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

#include "make_ptr.h"

#include <utility>

namespace Devices::TurboSound
{
  template<uint_t Count>
  class PSG
  {
  public:
    explicit PSG(const AYM::MultiVolumeTable& table)
      : Chips{{AYM::PSG(table), AYM::PSG(table), AYM::PSG(table)}}
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

  template<uint_t Count>
  struct Traits
  {
    using DataChunkType = DataChunk;
    using PSGType = PSG<Count>;
    using ChipBaseType = Chip;
    static const uint_t VOICES = TurboSound::VOICES;
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

  Chip::Ptr CreateChip(uint_t chips, ChipParameters::Ptr params, MixerType::Ptr mixer)
  {
    if (chips == MAX_CHIPS)
    {
      auto scaledMixer = MakePtr<ScaledLevelMixer>(std::move(mixer), MAX_CHIPS);
      return MakePtr<AYM::SoundChip<Traits<MAX_CHIPS>>>(std::move(params), std::move(scaledMixer));
    }
    auto scaledMixer = MakePtr<ScaledLevelMixer>(std::move(mixer), CHIPS);
    return MakePtr<AYM::SoundChip<Traits<CHIPS>>>(std::move(params), std::move(scaledMixer));
  }
}  // namespace Devices::TurboSound
