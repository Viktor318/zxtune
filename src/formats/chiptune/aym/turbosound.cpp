/**
 *
 * @file
 *
 * @brief  TurboSound container support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/turbosound.h"

#include "formats/chiptune/common/container.h"

#include "binary/format_factories.h"
#include "math/numeric.h"

#include "byteorder.h"
#include "make_ptr.h"
#include "string_type.h"

#include <algorithm>
#include <vector>

namespace Formats::Chiptune
{
  namespace TurboSound
  {
    const std::size_t MIN_SIZE = 256;
    const std::size_t MAX_MODULE_SIZE = 32767;

    // Footer is a list of {ID[4], le_uint16_t Size} per submodule followed by signature
    // '02TS' - two chips (Vortex Tracker, ZXTune)
    // '03TS' - three chips (ZX Spectrum Next, MXTracker)
    struct SubmoduleInfo
    {
      uint8_t ID[4];  //'PT3!' or other type
      le_uint16_t Size;
    };

    static_assert(sizeof(SubmoduleInfo) * alignof(SubmoduleInfo) == 6, "Invalid layout");

    const std::size_t SIGNATURE_SIZE = 4;

    const auto DESCRIPTION = "TurboSound"sv;
    const auto SUBMODULE_INFO_FORMAT =
        "%0xxxxxxx%0xxxxxxx%0xxxxxxx21"  // uint8_t ID[4];//'PT3!' or other type
        "?%0xxxxxxx"                     // uint16_t Size;
        ""sv;

    String MakeFooterFormat(uint_t chips)
    {
      String result;
      for (uint_t idx = 0; idx < chips; ++idx)
      {
        result += SUBMODULE_INFO_FORMAT;
      }
      result += chips == 2 ? "'0'2'T'S"sv : "'0'3'T'S"sv;
      return result;
    }

    class StubBuilder : public Builder
    {
    public:
      void AddSubmoduleLocation(std::size_t /*offset*/, std::size_t /*size*/) override {}
    };

    class ModuleTraits
    {
    public:
      ModuleTraits(Binary::View data, std::size_t footerOffset, uint_t chips)
        : FooterOffset(footerOffset)
        , FooterSize(chips * sizeof(SubmoduleInfo) + SIGNATURE_SIZE)
      {
        const auto foot = data.SubView(footerOffset, FooterSize);
        if (foot.Size() == FooterSize)
        {
          const auto* infos = foot.As<SubmoduleInfo>();
          for (uint_t idx = 0; idx < chips; ++idx)
          {
            Sizes.push_back(infos[idx].Size);
          }
        }
      }

      bool Matched() const
      {
        return !Sizes.empty() && FooterOffset == GetModulesSize()
               && Math::InRange(FooterOffset, MIN_SIZE, MAX_MODULE_SIZE * Sizes.size());
      }

      std::size_t NextOffset() const
      {
        if (Sizes.empty())
        {
          return FooterOffset;
        }
        const std::size_t totalSize = GetModulesSize();
        if (totalSize < FooterOffset)
        {
          return FooterOffset - totalSize;
        }
        else
        {
          return FooterOffset + FooterSize;
        }
      }

      const std::vector<std::size_t>& GetModulesSizes() const
      {
        return Sizes;
      }

      std::size_t GetTotalSize() const
      {
        return FooterOffset + FooterSize;
      }

    private:
      std::size_t GetModulesSize() const
      {
        std::size_t res = 0;
        for (const auto size : Sizes)
        {
          res += size;
        }
        return res;
      }

    private:
      const std::size_t FooterOffset;
      const std::size_t FooterSize;
      std::vector<std::size_t> Sizes;
    };

    // TODO: rework avoiding Binary::Format creation here
    class FooterFormat : public Binary::ScanningFormat
    {
    public:
      using Ptr = std::shared_ptr<const FooterFormat>;

      FooterFormat()
        : TwoChips(Binary::CreateScanningFormat(MakeFooterFormat(2)))
        , ThreeChips(Binary::CreateScanningFormat(MakeFooterFormat(3)))
      {}

      bool Match(Binary::View data) const override
      {
        return GetTraits(data).Matched();
      }

      std::size_t NextMatchOffset(Binary::View data) const override
      {
        const ModuleTraits two(data, TwoChips->NextMatchOffset(data), 2);
        const ModuleTraits three(data, ThreeChips->NextMatchOffset(data), 3);
        return std::min(two.NextOffset(), three.NextOffset());
      }

      ModuleTraits GetTraits(Binary::View data) const
      {
        ModuleTraits two(data, TwoChips->NextMatchOffset(data), 2);
        if (two.Matched())
        {
          return two;
        }
        return {data, ThreeChips->NextMatchOffset(data), 3};
      }

    private:
      const Binary::ScanningFormat::Ptr TwoChips;
      const Binary::ScanningFormat::Ptr ThreeChips;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      const ModuleTraits& traits = FooterFormat().GetTraits(rawData);

      if (!traits.Matched())
      {
        return {};
      }

      std::size_t offset = 0;
      for (const auto size : traits.GetModulesSizes())
      {
        target.AddSubmoduleLocation(offset, size);
        offset += size;
      }

      const std::size_t usedSize = traits.GetTotalSize();
      auto subData = rawData.GetSubcontainer(0, usedSize);
      // use whole container as a fixed data
      return CreateCalculatingCrcContainer(std::move(subData), 0, usedSize);
    }

    class DecoderImpl : public Decoder
    {
    public:
      DecoderImpl()
        : Format(MakePtr<FooterFormat>())
      {}

      StringView GetDescription() const override
      {
        return DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }

    private:
      const FooterFormat::Ptr Format;
    };

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

    Decoder::Ptr CreateDecoder()
    {
      return MakePtr<DecoderImpl>();
    }
  }  // namespace TurboSound

  Decoder::Ptr CreateTurboSoundDecoder()
  {
    return TurboSound::CreateDecoder();
  }
}  // namespace Formats::Chiptune
