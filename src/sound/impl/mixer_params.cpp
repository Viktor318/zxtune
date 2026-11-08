/**
 *
 * @file
 *
 * @brief  Parameters for mixer
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "parameters/accessor.h"
#include "parameters/delegated.h"
#include "sound/matrix_mixer.h"
#include "sound/mixer_parameters.h"
#include "sound/parameters.h"

#include "make_ptr.h"

#include <array>
#include <utility>
#include <vector>

namespace Sound
{
  using MultiConfigValue = std::array<Gain::Type, Sample::CHANNELS>;

  void GetMatrixRow(const Parameters::Accessor& params, uint_t channels, uint_t inChan, MultiConfigValue& out)
  {
    for (uint_t outChan = 0; outChan != out.size(); ++outChan)
    {
      using namespace Parameters::ZXTune::Sound;
      const auto level = Mixer::Level::For(channels, inChan, outChan);
      const auto val = Parameters::GetInteger<int_t>(params, level.Key, level.Default);
      out[outChan] = Gain::Type(val, Parameters::ZXTune::Sound::Gain::Precision);
    }
  }

  template<unsigned Channels>
  void GetMatrix(const Parameters::Accessor& params, typename FixedChannelsMatrixMixer<Channels>::Matrix& res)
  {
    MultiConfigValue row;
    for (uint_t inChan = 0; inChan != res.size(); ++inChan)
    {
      GetMatrixRow(params, Channels, inChan, row);
      res[inChan] = Gain(row[0], row[1]);
    }
  }

  template<unsigned Channels>
  void FillMixerInternal(const Parameters::Accessor& params, FixedChannelsMatrixMixer<Channels>& mixer)
  {
    typename FixedChannelsMatrixMixer<Channels>::Matrix res;
    GetMatrix<Channels>(params, res);
    mixer.SetMatrix(res);
  }

  void FillMixerPart(const Parameters::Accessor& params, uint_t totalChannels, uint_t firstChannel,
                     ThreeChannelsMatrixMixer& mixer)
  {
    ThreeChannelsMatrixMixer::Matrix res;
    MultiConfigValue row;
    for (uint_t inChan = 0; inChan != res.size(); ++inChan)
    {
      GetMatrixRow(params, totalChannels, firstChannel + inChan, row);
      res[inChan] = Gain(row[0], row[1]);
    }
    mixer.SetMatrix(res);
  }

  // Several 3-channel mixers (e.g. TurboSound chips) filled from consecutive rows of one bigger matrix
  class MultiMixerNotificationParameters : public Parameters::DelegatedAccessor
  {
  public:
    MultiMixerNotificationParameters(Parameters::Accessor::Ptr params,
                                     std::vector<ThreeChannelsMatrixMixer::Ptr> mixers)
      : Parameters::DelegatedAccessor(std::move(params))
      , Mixers(std::move(mixers))
      , LastVersion(~Delegate->Version())
    {}

    uint_t Version() const override
    {
      const uint_t newVers = Delegate->Version();
      if (newVers != LastVersion)
      {
        const auto totalChannels = static_cast<uint_t>(Mixers.size() * 3);
        for (std::size_t idx = 0; idx < Mixers.size(); ++idx)
        {
          FillMixerPart(*Delegate, totalChannels, static_cast<uint_t>(idx * 3), *Mixers[idx]);
        }
        LastVersion = newVers;
      }
      return newVers;
    }

  private:
    const std::vector<ThreeChannelsMatrixMixer::Ptr> Mixers;
    mutable uint_t LastVersion;
  };

  template<class MixerType>
  class MixerNotificationParameters : public Parameters::DelegatedAccessor
  {
  public:
    MixerNotificationParameters(Parameters::Accessor::Ptr params, typename MixerType::Ptr mixer)
      : Parameters::DelegatedAccessor(std::move(params))
      , Mixer(std::move(mixer))
      , LastVersion(~Delegate->Version())
    {}

    uint_t Version() const override
    {
      const uint_t newVers = Delegate->Version();
      if (newVers != LastVersion)
      {
        FillMixer(*Delegate, *Mixer);
        LastVersion = newVers;
      }
      return newVers;
    }

  private:
    const typename MixerType::Ptr Mixer;
    mutable uint_t LastVersion;
  };

  template<unsigned Channels>
  Parameters::Accessor::Ptr
      CreateMixerNotificationParametersInternal(Parameters::Accessor::Ptr params,
                                                typename FixedChannelsMatrixMixer<Channels>::Ptr mixer)
  {
    return MakePtr<MixerNotificationParameters<FixedChannelsMatrixMixer<Channels> > >(params, mixer);
  }
}  // namespace Sound

namespace Sound
{
  void FillMixer(const Parameters::Accessor& params, ThreeChannelsMatrixMixer& mixer)
  {
    FillMixerInternal<3>(params, mixer);
  }

  void FillMixer(const Parameters::Accessor& params, FourChannelsMatrixMixer& mixer)
  {
    FillMixerInternal<4>(params, mixer);
  }

  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate,
                                                              ThreeChannelsMatrixMixer::Ptr mixer)
  {
    return CreateMixerNotificationParametersInternal<3>(std::move(delegate), std::move(mixer));
  }

  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate,
                                                              FourChannelsMatrixMixer::Ptr mixer)
  {
    return CreateMixerNotificationParametersInternal<4>(std::move(delegate), std::move(mixer));
  }

  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate,
                                                              std::vector<ThreeChannelsMatrixMixer::Ptr> mixers)
  {
    return MakePtr<MultiMixerNotificationParameters>(std::move(delegate), std::move(mixers));
  }
}  // namespace Sound
