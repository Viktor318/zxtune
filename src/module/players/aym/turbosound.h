/**
 *
 * @file
 *
 * @brief  TurboSound-based chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/aym/aym_base_track.h"

#include "devices/turbosound.h"
#include "module/holder.h"
#include "strings/array.h"

#include <vector>

namespace Module::TurboSound
{
  using DataIterator = Iterator<Devices::TurboSound::Registers>;

  // One per chip
  using TrackParametersList = std::vector<AYM::TrackParameters::Ptr>;

  class Chiptune
  {
  public:
    using Ptr = std::unique_ptr<const Chiptune>;
    virtual ~Chiptune() = default;

    virtual Time::Microseconds GetFrameDuration() const = 0;

    virtual FramedInformation GetInformation() const = 0;

    virtual uint_t GetChipsCount() const = 0;

    virtual DataIterator::Ptr CreateDataIterator(TrackParametersList params) const = 0;
  };

  // Submodules are played on chips in the order given
  Chiptune::Ptr CreateChiptune(std::vector<AYM::Chiptune::Ptr> submodules);

  Parameters::Accessor::Ptr CreateModuleProperties(Parameters::Accessor::Ptr outer,
                                                   std::vector<Parameters::Accessor::Ptr> submodules);

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties);
}  // namespace Module::TurboSound
