/**
 *
 * @file
 *
 * @brief  AY/YM device implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "devices/aym/chip.h"
#include "devices/aym/src/generators.h"

namespace Devices::AYM
{
  class AYMDevice
  {
  public:
    void SetDutyCycle(uint_t value, uint_t mask)
    {
      GenA.SetDutyCycle(0 != (mask & CHANNEL_MASK_A) ? value : NO_DUTYCYCLE);
      GenB.SetDutyCycle(0 != (mask & CHANNEL_MASK_B) ? value : NO_DUTYCYCLE);
      GenC.SetDutyCycle(0 != (mask & CHANNEL_MASK_C) ? value : NO_DUTYCYCLE);
    }

    void SetMuteMask(uint_t mask)
    {
      MuteMask = mask;
      MutedLevels = ((mask & CHANNEL_MASK_A) ? HIGH_LEVEL_A : LOW_LEVEL)
                    | ((mask & CHANNEL_MASK_B) ? HIGH_LEVEL_B : LOW_LEVEL)
                    | ((mask & CHANNEL_MASK_C) ? HIGH_LEVEL_C : LOW_LEVEL);
    }

    void SetMixer(uint_t mixer)
    {
      GenA.SetMasked(0 != (mixer & Registers::MASK_TONEA));
      GenB.SetMasked(0 != (mixer & Registers::MASK_TONEB));
      GenC.SetMasked(0 != (mixer & Registers::MASK_TONEC));
      ToneOffMask = ((mixer & Registers::MASK_TONEA) ? HIGH_LEVEL_A : LOW_LEVEL)
                    | ((mixer & Registers::MASK_TONEB) ? HIGH_LEVEL_B : LOW_LEVEL)
                    | ((mixer & Registers::MASK_TONEC) ? HIGH_LEVEL_C : LOW_LEVEL);
      NoiseMask = HIGH_LEVEL;
      if (0 == (mixer & Registers::MASK_NOISEA))
      {
        NoiseMask ^= HIGH_LEVEL_A;
      }
      if (0 == (mixer & Registers::MASK_NOISEB))
      {
        NoiseMask ^= HIGH_LEVEL_B;
      }
      if (0 == (mixer & Registers::MASK_NOISEC))
      {
        NoiseMask ^= HIGH_LEVEL_C;
      }
    }

    void SetToneA(uint_t toneA)
    {
      GenA.SetPeriod(toneA);
    }

    void SetToneB(uint_t toneB)
    {
      GenB.SetPeriod(toneB);
    }

    void SetToneC(uint_t toneC)
    {
      GenC.SetPeriod(toneC);
    }

    void SetToneN(uint_t toneN)
    {
      GenN.SetPeriod(toneN);
    }

    void SetToneE(uint_t toneE)
    {
      GenE.SetPeriod(toneE);
    }

    void SetEnvType(uint_t type)
    {
      GenE.SetType(type);
    }

    void SetLevel(uint_t levelA, uint_t levelB, uint_t levelC)
    {
      Levels = 0;
      EnvelopeMask = 0;

      SetLevel(0, levelA);
      SetLevel(1, levelB);
      SetLevel(2, levelC);
    }

    void Reset()
    {
      GenA.Reset();
      GenB.Reset();
      GenC.Reset();
      GenN.Reset();
      GenE.Reset();
      Levels = 0;
      NoiseMask = HIGH_LEVEL;
      EnvelopeMask = LOW_LEVEL;
      ToneOffMask = HIGH_LEVEL;
      MuteMask = LOW_LEVEL;
      MutedLevels = LOW_LEVEL;
    }

    void Tick(uint_t ticks)
    {
      GenA.Tick(ticks);
      GenB.Tick(ticks);
      GenC.Tick(ticks);
      GenN.Tick(ticks);
      GenE.Tick(ticks);
    }

    uint_t GetLevels() const
    {
      // muted envelope silences channels using it, others keep their own levels
      const uint_t level = (MuteMask & CHANNEL_MASK_E) || !EnvelopeMask
                               ? Levels
                               : (EnvelopeMask * GenE.GetLevel()) | Levels;
      // muted noise: channels playing noise only would give a constant level (audible clicks on volume changes),
      // so they are silent, channels playing tone and noise keep the tone
      const uint_t noise = (MuteMask & CHANNEL_MASK_N)
                               ? HIGH_LEVEL ^ (ToneOffMask & ~NoiseMask)
                               : (NoiseMask != HIGH_LEVEL ? NoiseMask | GenN.GetLevel() : NoiseMask);
      const uint_t toneA = GenA.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_A, HIGH_LEVEL>();
      const uint_t toneB = GenB.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_B, HIGH_LEVEL>();
      const uint_t toneC = GenC.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_C, HIGH_LEVEL>();

      // muted channels are silent (level 0), not just without tone
      return level & toneA & toneB & toneC & noise & ~MutedLevels;
    }

  private:
    void SetLevel(uint_t chan, uint_t reg)
    {
      const uint_t shift = chan * BITS_PER_LEVEL;
      if (0 != (reg & Registers::MASK_ENV))
      {
        EnvelopeMask |= 1 << shift;
      }
      else
      {
        Levels |= ((reg << 1) + 1) << shift;
      }
    }

  private:
    ToneGenerator GenA;
    ToneGenerator GenB;
    ToneGenerator GenC;
    NoiseGenerator GenN;
    EnvelopeGenerator GenE;
    uint_t Levels = 0;
    uint_t NoiseMask = HIGH_LEVEL;
    uint_t EnvelopeMask = LOW_LEVEL;
    uint_t ToneOffMask = HIGH_LEVEL;
    uint_t MuteMask = LOW_LEVEL;
    uint_t MutedLevels = LOW_LEVEL;
  };
}  // namespace Devices::AYM
