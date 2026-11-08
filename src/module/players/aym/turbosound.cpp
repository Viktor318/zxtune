/**
 *
 * @file
 *
 * @brief  TurboSound-based chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/turbosound.h"

#include "module/players/parameters.h"
#include "module/players/streaming.h"
#include "parameters/src/names_set.h"

#include "module/attributes.h"
#include "parameters/merged_accessor.h"
#include "parameters/visitor.h"
#include "sound/mixer_factory.h"
#include "sound/mixer_parameters.h"
#include "tools/iterators.h"

#include "error.h"
#include "make_ptr.h"
#include "string_view.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace Module::TurboSound
{
  // Same values are merged into one, different ones are joined with '/' in chips order
  // (see AYM::TrackParameters for per-chip frequency tables)
  class MergedStrings
  {
  public:
    void Add(StringView val)
    {
      Values.emplace_back(val);
    }

    String Get() const
    {
      if (std::all_of(Values.begin(), Values.end(), [this](const auto& val) { return val == Values.front(); }))
      {
        return Values.front();
      }
      String result;
      for (const auto& val : Values)
      {
        if (!result.empty())
        {
          result += '/';
        }
        result += val;
      }
      return result;
    }

  private:
    std::vector<String> Values;
  };

  class MergedModuleProperties : public Parameters::Accessor
  {
    class MergedStringsVisitor : public Parameters::Visitor
    {
    public:
      explicit MergedStringsVisitor(Visitor& delegate)
        : Delegate(delegate)
      {}

      void SetValue(Parameters::Identifier name, Parameters::IntType val) override
      {
        if (DoneIntegers.Insert(name))
        {
          return Delegate.SetValue(name, val);
        }
      }

      void SetValue(Parameters::Identifier name, StringView val) override
      {
        Strings[name.AsString()].Add(val);
      }

      void SetValue(Parameters::Identifier name, Binary::View val) override
      {
        if (DoneDatas.Insert(name))
        {
          return Delegate.SetValue(name, val);
        }
      }

      void ProcessRestStrings() const
      {
        for (const auto& str : Strings)
        {
          Delegate.SetValue(str.first, str.second.Get());
        }
      }

    private:
      Parameters::Visitor& Delegate;
      std::map<String, MergedStrings, std::less<>> Strings;
      Parameters::NamesSet DoneIntegers;
      Parameters::NamesSet DoneDatas;
    };

  public:
    explicit MergedModuleProperties(std::vector<Parameters::Accessor::Ptr> submodules)
      : Submodules(std::move(submodules))
    {}

    uint_t Version() const override
    {
      return 1;
    }

    std::optional<Parameters::IntType> FindInteger(Parameters::Identifier name) const override
    {
      for (const auto& sub : Submodules)
      {
        if (auto val = sub->FindInteger(name))
        {
          return val;
        }
      }
      return {};
    }

    std::optional<Parameters::StringType> FindString(Parameters::Identifier name) const override
    {
      MergedStrings merged;
      bool found = false;
      for (const auto& sub : Submodules)
      {
        if (auto val = sub->FindString(name))
        {
          merged.Add(*val);
          found = true;
        }
      }
      if (found)
      {
        return merged.Get();
      }
      return {};
    }

    Binary::Data::Ptr FindData(Parameters::Identifier name) const override
    {
      for (const auto& sub : Submodules)
      {
        if (auto val = sub->FindData(name))
        {
          return val;
        }
      }
      return {};
    }

    void Process(Parameters::Visitor& visitor) const override
    {
      MergedStringsVisitor mergedVisitor(visitor);
      for (const auto& sub : Submodules)
      {
        sub->Process(mergedVisitor);
      }
      mergedVisitor.ProcessRestStrings();
    }

  private:
    const std::vector<Parameters::Accessor::Ptr> Submodules;
  };

  class MergedSource : public DataIterator
  {
  public:
    explicit MergedSource(std::vector<AYM::DataIterator::Ptr> sources)
      : Sources(std::move(sources))
    {}

    FramedState GetState() const override
    {
      auto result = Sources.front()->GetState();
      for (auto it = Sources.begin() + 1, lim = Sources.end(); it != lim && result.Track; ++it)
      {
        if (const auto state = (*it)->GetState(); state.Track)
        {
          result.Track->Channels += state.Track->Channels;
        }
      }
      return result;
    }

    Devices::TurboSound::Registers Advance() override
    {
      Devices::TurboSound::Registers result;
      for (std::size_t idx = 0; idx < Sources.size(); ++idx)
      {
        result[idx] = Sources[idx]->Advance();
      }
      return result;
    }

    void Reset() override
    {
      for (const auto& src : Sources)
      {
        src->Reset();
      }
    }

  private:
    const std::vector<AYM::DataIterator::Ptr> Sources;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(FrameDurationParameter frameDuration, DataIterator::Ptr iterator, Devices::TurboSound::Chip::Ptr device)
      : FrameDuration(std::move(frameDuration))
      , Iterator(std::move(iterator))
      , Device(std::move(device))
    {}

    State GetState() const override
    {
      return ToState(Iterator->GetState(), FrameDuration.Get());
    }

    Sound::Chunk Render() override
    {
      TransferChunk();
      FrameDuration.Synchronize();
      LastChunk.TimeStamp += FrameDuration.Get<Devices::TurboSound::TimeUnit>();
      return Device->RenderTill(LastChunk.TimeStamp);
    }

    void Reset() override
    {
      Iterator->Reset();
      Device->Reset();
      LastChunk.TimeStamp = {};
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      if (request < GetState().At)
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = {};
      }
      while (GetState().At < request)
      {
        TransferChunk();
      }
    }

  private:
    void TransferChunk()
    {
      LastChunk.Data = Iterator->Advance();
      Device->RenderData(LastChunk);
    }

  private:
    FrameDurationParameter FrameDuration;
    const DataIterator::Ptr Iterator;
    const Devices::TurboSound::Chip::Ptr Device;
    Devices::TurboSound::DataChunk LastChunk;
  };

  class MergedChiptune : public Chiptune
  {
  public:
    explicit MergedChiptune(std::vector<AYM::Chiptune::Ptr> submodules)
      : Submodules(std::move(submodules))
    {}

    Time::Microseconds GetFrameDuration() const override
    {
      return Submodules.front()->GetFrameDuration();
    }

    FramedInformation GetInformation() const override
    {
      auto info = Submodules.front()->GetInformation();
      if (info.Track)
      {
        info.Track->ChannelsCount = AYM::TRACK_CHANNELS * GetChipsCount();
      }
      return info;
    }

    uint_t GetChipsCount() const override
    {
      return static_cast<uint_t>(Submodules.size());
    }

    DataIterator::Ptr CreateDataIterator(TrackParametersList params) const override
    {
      std::vector<AYM::DataIterator::Ptr> sources;
      for (std::size_t idx = 0; idx < Submodules.size(); ++idx)
      {
        sources.emplace_back(Submodules[idx]->CreateDataIterator(std::move(params[idx])));
      }
      return MakePtr<MergedSource>(std::move(sources));
    }

  private:
    const std::vector<AYM::Chiptune::Ptr> Submodules;
  };

  Devices::TurboSound::Chip::Ptr CreateChip(uint_t chips, uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    // per-chip mixers are set up by the rows of AY mixer parameters
    using MixerType = Sound::ThreeChannelsMatrixMixer;
    std::vector<MixerType::Ptr> mixers;
    for (uint_t idx = 0; idx < chips; ++idx)
    {
      mixers.emplace_back(MixerType::Create());
    }
    auto pollParams = Sound::CreateMixerNotificationParameters(std::move(params), mixers,
                                                               Parameters::ZXTune::Sound::Mixer::AYM_CHANNELS);
    auto chipParams = AYM::ChipParameters::Create(samplerate, std::move(pollParams));
    return Devices::TurboSound::CreateChip(std::move(chipParams), {mixers.begin(), mixers.end()});
  }

  class Holder : public Module::Holder
  {
  public:
    Holder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties, Time::Microseconds frameDuration)
      : Tune(std::move(chiptune))
      , Properties(std::move(properties))
      , FrameDuration(frameDuration)
    {}

    Information GetModuleInformation() const override
    {
      return ToInformation(Tune->GetInformation(), FrameDuration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto rendererParams = params;
      const auto chips = Tune->GetChipsCount();
      TrackParametersList trackParams;
      for (uint_t idx = 0; idx < chips; ++idx)
      {
        trackParams.emplace_back(AYM::TrackParameters::Create(params, idx));
      }
      auto iterator = Tune->CreateDataIterator(std::move(trackParams));
      auto chip = CreateChip(chips, samplerate, std::move(params));
      return MakePtr<Renderer>(FrameDurationParameter(std::move(rendererParams), AYM::BASE_FRAME_DURATION),
                               std::move(iterator), std::move(chip));
    }

  private:
    const Chiptune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
    const Time::Microseconds FrameDuration;
  };

  Chiptune::Ptr CreateChiptune(std::vector<AYM::Chiptune::Ptr> submodules)
  {
    /* TODO: think about it
    if (first->GetInformation()->FramesCount() < second->GetInformation()->FramesCount())
    {
      std::swap(first, second);
    }
    */
    return MakePtr<MergedChiptune>(std::move(submodules));
  }

  Parameters::Accessor::Ptr CreateModuleProperties(Parameters::Accessor::Ptr outer,
                                                   std::vector<Parameters::Accessor::Ptr> submodules)
  {
    auto mixProps = MakePtr<MergedModuleProperties>(std::move(submodules));
    return Parameters::CreateMergedAccessor(std::move(outer), std::move(mixProps));
  }

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
  {
    const FrameDurationParameter frameDuration(properties, AYM::BASE_FRAME_DURATION);
    return MakePtr<Holder>(std::move(chiptune), std::move(properties), frameDuration.Get());
  }
}  // namespace Module::TurboSound
