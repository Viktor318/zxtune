/**
 *
 * @file
 *
 * @brief  Parameters::ZXTune::Sound and tested
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/parameters.h"

#include "sound/backends/parameters.h"

#include "sound/gain.h"
#include "sound/mixer_parameters.h"

namespace Parameters::ZXTune::Sound::Mixer
{
  static_assert(::Sound::Gain::CHANNELS == 2, "Incompatible sound channels count");

  const Level MIXERS[4][4][2] = {
      // 1-channel
      {
          {{Prefix + "1.0_0"_id, 100}, {Prefix + "1.0_1"_id, 100}},
      },
      // 2-channel
      {
          {{Prefix + "2.0_0"_id, 100}, {Prefix + "2.0_1"_id, 0}},
          {{Prefix + "2.1_0"_id, 0}, {Prefix + "2.1_1"_id, 100}},
      },
      // 3-channel
      {
          {{Prefix + "3.0_0"_id, 100}, {Prefix + "3.0_1"_id, 0}},
          {{Prefix + "3.1_0"_id, 60}, {Prefix + "3.1_1"_id, 60}},
          {{Prefix + "3.2_0"_id, 0}, {Prefix + "3.2_1"_id, 100}},
      },
      // 4-channel
      {
          {{Prefix + "4.0_0"_id, 100}, {Prefix + "4.0_1"_id, 0}},
          {{Prefix + "4.1_0"_id, 100}, {Prefix + "4.1_1"_id, 0}},
          {{Prefix + "4.2_0"_id, 0}, {Prefix + "4.2_1"_id, 100}},
          {{Prefix + "4.3_0"_id, 0}, {Prefix + "4.3_1"_id, 100}},
      },
  };

  // AY/YM chips (single or TurboSound): chips' A/B/C in a row, each chip defaults to 3-channel mixer
  const Level MIXERS_9[9][2] = {
      {{Prefix + "9.0_0"_id, 100}, {Prefix + "9.0_1"_id, 0}}, {{Prefix + "9.1_0"_id, 60}, {Prefix + "9.1_1"_id, 60}},
      {{Prefix + "9.2_0"_id, 0}, {Prefix + "9.2_1"_id, 100}}, {{Prefix + "9.3_0"_id, 100}, {Prefix + "9.3_1"_id, 0}},
      {{Prefix + "9.4_0"_id, 60}, {Prefix + "9.4_1"_id, 60}}, {{Prefix + "9.5_0"_id, 0}, {Prefix + "9.5_1"_id, 100}},
      {{Prefix + "9.6_0"_id, 100}, {Prefix + "9.6_1"_id, 0}}, {{Prefix + "9.7_0"_id, 60}, {Prefix + "9.7_1"_id, 60}},
      {{Prefix + "9.8_0"_id, 0}, {Prefix + "9.8_1"_id, 100}},
  };

  Level Level::For(uint_t totalChannels, uint_t inChannel, uint_t outChannel)
  {
    switch (totalChannels)
    {
    case 9:
      return MIXERS_9[inChannel][outChannel];
    default:
      return MIXERS[totalChannels - 1][inChannel][outChannel];
    }
  }
}  // namespace Parameters::ZXTune::Sound::Mixer
