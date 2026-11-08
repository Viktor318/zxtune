/**
 *
 * @file
 *
 * @brief  TurboSound containers support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/ts.h"

#include "formats/chiptune/aym/turbosound.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/aym/turbosound.h"
#include "module/players/properties_helper.h"

#include "debug/log.h"
#include "module/attributes.h"
#include "parameters/accessor.h"
#include "parameters/identifiers_set.h"

#include "make_ptr.h"

#include <vector>

namespace Module::TS
{
  const Debug::Stream Dbg("Module::TS");

  class DataBuilder : public Formats::Chiptune::TurboSound::Builder
  {
  public:
    DataBuilder(const Parameters::Accessor& params, const Binary::Container& data, const Factory& delegate)
      : Params(params)
      , Data(data)
      , Delegate(delegate)
    {}

    void AddSubmoduleLocation(std::size_t offset, std::size_t size) override
    {
      if (!LoadChiptune(offset, size))
      {
        Dbg("Failed to create submodule {}", Locations);
        Failed = true;
      }
      ++Locations;
    }

    bool HasResult() const
    {
      return !Failed && Chiptunes.size() == Locations && Locations >= Devices::TurboSound::CHIPS
             && Locations <= Devices::TurboSound::MAX_CHIPS;
    }

    std::vector<AYM::Chiptune::Ptr> CaptureChiptunes()
    {
      return std::move(Chiptunes);
    }

    const std::vector<Parameters::Accessor::Ptr>& GetProperties() const
    {
      return Properties;
    }

  private:
    bool LoadChiptune(std::size_t offset, std::size_t size)
    {
      const auto content = Data.GetSubcontainer(offset, size);
      if (const auto holder = std::dynamic_pointer_cast<const AYM::Holder>(TryOpenAYModule(*content)))
      {
        Chiptunes.emplace_back(holder->GetChiptune());
        Properties.emplace_back(holder->GetModuleProperties());
        return true;
      }
      return false;
    }

    Module::Holder::Ptr TryOpenAYModule(const Binary::Container& data) const
    {
      const auto initialProperties = Parameters::Container::Create();
      return Delegate.CreateModule(Params, data, initialProperties);
    }

  private:
    const Parameters::Accessor& Params;
    const Binary::Container& Data;
    const Factory& Delegate;
    std::size_t Locations = 0;
    bool Failed = false;
    std::vector<AYM::Chiptune::Ptr> Chiptunes;
    std::vector<Parameters::Accessor::Ptr> Properties;
  };

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(Ptr delegate)
      : Delegate(std::move(delegate))
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& data,
                                     Parameters::Container::Ptr properties) const override
    {
      DataBuilder dataBuilder(params, data, *Delegate);
      if (const auto container = Formats::Chiptune::TurboSound::Parse(data, dataBuilder))
      {
        if (dataBuilder.HasResult())
        {
          const auto& subProperties = dataBuilder.GetProperties();
          AYM::PropertiesHelper props(*properties);
          props.SetSource(*container);
          props.SetChipsCount(static_cast<uint_t>(subProperties.size()));
          // A composite supports everything its parts do
          // Sets view into these strings, so keep them alive till properties are stored
          std::vector<String> subIds;
          for (const auto& sub : subProperties)
          {
            subIds.emplace_back(Parameters::GetString(*sub, ATTR_SUPPORTED_PARAMETERS));
          }
          Parameters::IdentifiersSet supported;
          for (const auto& ids : subIds)
          {
            supported = supported | Parameters::IdentifiersSet::Parse(ids);
          }
          props.SetSupportedParameters(supported);
          auto moduleProperties = TurboSound::CreateModuleProperties(properties, subProperties);
          auto chiptune = TurboSound::CreateChiptune(dataBuilder.CaptureChiptunes());
          return TurboSound::CreateHolder(std::move(chiptune), std::move(moduleProperties));
        }
      }
      return {};
    }

  private:
    const Ptr Delegate;
  };

  Factory::Ptr CreateFactory(Factory::Ptr delegate)
  {
    return MakePtr<Factory>(std::move(delegate));
  }
}  // namespace Module::TS
