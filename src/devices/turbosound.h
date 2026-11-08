/**
 *
 * @file
 *
 * @brief  TurboSound support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "devices/aym/chip.h"

#include <array>
#include <vector>

namespace Devices::TurboSound
{
  // Classic TurboSound
  const uint_t CHIPS = 2;
  // ZX Spectrum Next (TurboSound Next)
  const uint_t MAX_CHIPS = 3;
  const uint_t VOICES = AYM::VOICES * MAX_CHIPS;

  using AYM::TimeUnit;
  using AYM::Stamp;

  // Registers of unused chips are left untouched
  using Registers = std::array<AYM::Registers, MAX_CHIPS>;

  struct DataChunk
  {
    DataChunk() = default;

    Stamp TimeStamp;
    Registers Data;
  };

  class Device
  {
  public:
    using Ptr = std::unique_ptr<Device>;
    virtual ~Device() = default;

    virtual void RenderData(DataChunk src) = 0;
    virtual void Reset() = 0;
  };

  class Chip : public Device
  {
  public:
    using Ptr = std::unique_ptr<Chip>;

    virtual Sound::Chunk RenderTill(Stamp till) = 0;
  };

  using AYM::ChipParameters;
  using AYM::MixerType;

  // One mixer per chip, 2 or 3 chips
  using Mixers = std::vector<MixerType::Ptr>;

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Mixers mixers);
}  // namespace Devices::TurboSound
