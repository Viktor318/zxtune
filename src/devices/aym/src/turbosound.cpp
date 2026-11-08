/**
 *
 * @file
 *
 * @brief  TurboSound chip implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "devices/turbosound.h"

#include "devices/aym/src/psg.h"
#include "devices/aym/src/soundchip.h"

#include "contract.h"
#include "make_ptr.h"

#include <utility>

namespace Devices::TurboSound
{
  // Every chip has its own volume table built from its own mixer
  using VolumeTables = std::array<AYM::MultiVolumeTable, MAX_CHIPS>;

  template<uint_t Count>
  class PSG
  {
  public:
    explicit PSG(const VolumeTables& tables)
      : Chips{{AYM::PSG(tables[0]), AYM::PSG(tables[1]), AYM::PSG(tables[2])}}
    {}

    void SetDutyCycle(uint_t value, uint_t mask)
    {
      for (auto& chip : Chips)
      {
        chip.SetDutyCycle(value, mask);
      }
    }

    void SetMuteMask(uint_t mask)
    {
      for (auto& chip : Chips)
      {
        chip.SetMuteMask(mask);
        mask >>= AYM::VOICES;
      }
    }

    void Reset()
    {
      for (auto& chip : Chips)
      {
        chip.Reset();
      }
    }

    void SetNewData(const Registers& data)
    {
      for (uint_t idx = 0; idx < Count; ++idx)
      {
        Chips[idx].SetNewData(data[idx]);
      }
    }

    void Tick(uint_t ticks)
    {
      for (uint_t idx = 0; idx < Count; ++idx)
      {
        Chips[idx].Tick(ticks);
      }
    }

    Sound::Sample GetLevels() const
    {
      Sound::Sample result = Chips[0].GetLevels();
      for (uint_t idx = 1; idx < Count; ++idx)
      {
        result = Sound::Sample::FastAdd(result, Chips[idx].GetLevels());
      }
      return result;
    }

  private:
    std::array<AYM::PSG, MAX_CHIPS> Chips;
  };

  // Each chip's output is scaled to keep the sum of all chips in range
  class ScaledLevelMixer : public MixerType
  {
  public:
    ScaledLevelMixer(MixerType::Ptr delegate, uint_t divisor)
      : Delegate(std::move(delegate))
      , DelegateRef(*Delegate)
      , Divisor(divisor)
    {}

    Sound::Sample ApplyData(const MixerType::InDataType& in) const override
    {
      const Sound::Sample out = DelegateRef.ApplyData(in);
      return {out.Left() / Divisor, out.Right() / Divisor};
    }

  private:
    const MixerType::Ptr Delegate;
    const MixerType& DelegateRef;
    const int_t Divisor;
  };

  template<uint_t Count>
  class SoundChip : public Chip
  {
  public:
    SoundChip(ChipParameters::Ptr params, Mixers mixers)
      : Params(std::move(params))
      , Psg(VolTables)
      , Renderers(Clock, Psg)
    {
      for (uint_t idx = 0; idx < Count; ++idx)
      {
        ChipMixers[idx] = MakePtr<ScaledLevelMixer>(std::move(mixers[idx]), Count);
      }
      SoundChip::Reset();
    }

    void RenderData(DataChunk src) override
    {
      Chunks.Add(std::move(src));
    }

    void Reset() override
    {
      Params.Reset();
      Psg.Reset();
      Renderers.Reset();
      Chunks = {};
      SynchronizeParameters();
    }

    Sound::Chunk RenderTill(Stamp till) override
    {
      SynchronizeParameters();
      Sound::Chunk result;
      for (const auto& event : Chunks.ConsumeBefore(till))
      {
        Renderers.Render(event.TimeStamp, &result);
        Psg.SetNewData(event.Data);
      }
      Renderers.Render(till, &result);
      return result;
    }

  private:
    void SynchronizeParameters()
    {
      const uint_t AYM_CLOCK_DIVISOR = 8;

      if (Params.IsChanged())
      {
        Psg.SetMuteMask(Params->MuteMask());
        Psg.SetDutyCycle(Params->DutyCycleValue(), Params->DutyCycleMask());
        const uint_t clock = Params->ClockFreq() / AYM_CLOCK_DIVISOR;
        const uint_t sndFreq = Params->SoundFreq();
        Renderers.SetFrequency(clock, sndFreq);
        Renderers.SetInterpolation(Params->Interpolation());
        for (uint_t idx = 0; idx < Count; ++idx)
        {
          VolTables[idx].SetParameters(Params->Type(), Params->Layout(), *ChipMixers[idx]);
        }
      }
    }

  private:
    Parameters::TrackingHelper<ChipParameters> Params;
    std::array<MixerType::Ptr, Count> ChipMixers;
    VolumeTables VolTables;
    PSG<Count> Psg;
    AYM::ClockSource Clock;
    AYM::RenderersSet<PSG<Count>> Renderers;
    Details::ChunksCache<DataChunk, Stamp> Chunks;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Mixers mixers)
  {
    if (mixers.size() == MAX_CHIPS)
    {
      return MakePtr<SoundChip<MAX_CHIPS>>(std::move(params), std::move(mixers));
    }
    Require(mixers.size() == CHIPS);
    return MakePtr<SoundChip<CHIPS>>(std::move(params), std::move(mixers));
  }
}  // namespace Devices::TurboSound
