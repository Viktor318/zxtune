/**
 *
 * @file
 *
 * @brief Mixing settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/mixing.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/supp/playback_supp.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "mixing.ui.h"

#include "module/attributes.h"
#include "parameters/accessor.h"
#include "sound/mixer_parameters.h"

#include "contract.h"
#include "string_view.h"

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

namespace
{
  const char* INPUT_CHANNEL_NAMES[4] = {"A", "B", "C", "D"};
  // AY mixer has 3 rows per chip
  const unsigned CHIP_CHANNELS = 3;
  const unsigned MAX_CHIPS = Parameters::ZXTune::Sound::Mixer::AYM_CHANNELS / CHIP_CHANNELS;
  // Channels names of AY/YM modules are A, B, C, Noise and Envelope per chip
  const std::size_t AYM_CHANNELS_NAMES_PER_CHIP = 5;

  // Only these DAC players render via mixer matrix (others mix by themselves)
  const StringView DAC_3CHANNELS_TYPES[] = {"DMM"sv, "DST"sv, "STR"sv};
  const StringView DAC_4CHANNELS_TYPES[] = {"PDT"sv, "CHI"sv, "SQD"sv, "ET1"sv};

  struct MixerTarget
  {
    bool Valid = false;
    UI::MixingSettingsWidget::Kind Kind = UI::MixingSettingsWidget::AYM;
    // chips for AYM, channels for DAC
    unsigned Count = 0;
  };

  template<std::size_t N>
  bool IsOneOf(StringView type, const StringView (&types)[N])
  {
    return std::find(std::begin(types), std::end(types), type) != std::end(types);
  }

  std::size_t GetChannelsNamesCount(const Playlist::Item::Data& item)
  {
    const auto names = Parameters::GetString(*item.GetModuleProperties(), Module::ATTR_CHANNELS_NAMES);
    return names.empty() ? 0 : 1 + std::count(names.begin(), names.end(), '\n');
  }

  MixerTarget GetMixerTarget(const Playlist::Item::Data& item)
  {
    MixerTarget res;
    const auto type = item.GetType();
    if (item.GetCapabilities().IsAYM())
    {
      res.Valid = true;
      res.Kind = UI::MixingSettingsWidget::AYM;
      const auto chips = GetChannelsNamesCount(item) / AYM_CHANNELS_NAMES_PER_CHIP;
      res.Count = std::clamp<unsigned>(static_cast<unsigned>(chips), 1, MAX_CHIPS);
    }
    else if (item.GetCapabilities().IsDAC())
    {
      if (IsOneOf(type, DAC_3CHANNELS_TYPES))
      {
        res = {true, UI::MixingSettingsWidget::DAC, 3};
      }
      else if (IsOneOf(type, DAC_4CHANNELS_TYPES))
      {
        res = {true, UI::MixingSettingsWidget::DAC, 4};
      }
    }
    return res;
  }

  // Parameters store levels of left and right output for each input channel.
  // UI shows them as volume and panning, the same way as Fuse and ZEsarUX mixers do:
  // the side panning goes to keeps full volume, the other side is attenuated.
  struct VolumePan
  {
    int Volume = 0;
    int Pan = 0;  //-100 (left) .. 0 (center) .. 100 (right)
  };

  VolumePan FromLevels(int left, int right)
  {
    VolumePan res;
    res.Volume = std::max(left, right);
    if (res.Volume != 0 && left != right)
    {
      const auto weaker = std::min(left, right);
      const auto pan = (100 * (res.Volume - weaker) + res.Volume / 2) / res.Volume;
      res.Pan = left > right ? -pan : pan;
    }
    return res;
  }

  int Attenuated(int volume, int pan)
  {
    return (volume * (100 - pan) + 50) / 100;
  }

  std::pair<int, int> ToLevels(VolumePan vp)
  {
    const auto left = vp.Pan > 0 ? Attenuated(vp.Volume, vp.Pan) : vp.Volume;
    const auto right = vp.Pan < 0 ? Attenuated(vp.Volume, -vp.Pan) : vp.Volume;
    return {left, right};
  }

  class ChannelControls : public QObject
  {
  public:
    ChannelControls(QWidget& parent, Parameters::Container& storage, unsigned channels, unsigned channel)
      : QObject(&parent)
      , Storage(storage)
      , Left(Parameters::ZXTune::Sound::Mixer::Level::For(channels, channel, 0))
      , Right(Parameters::ZXTune::Sound::Mixer::Level::For(channels, channel, 1))
      , VolumeSlider(new QSlider(Qt::Horizontal, &parent))
      , VolumeValue(new QSpinBox(&parent))
      , PanSlider(new QSlider(Qt::Horizontal, &parent))
      , PanValue(new QLabel(&parent))
    {
      VolumeSlider->setRange(0, 100);
      VolumeValue->setRange(0, 100);
      VolumeValue->setSuffix(QLatin1String(" %"));
      PanSlider->setRange(-100, 100);
      PanSlider->setTickPosition(QSlider::TicksBelow);
      PanSlider->setTickInterval(100);
      PanValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      PanValue->setMinimumWidth(PanValue->fontMetrics().horizontalAdvance(QLatin1String("M 100")));
      Load();
      Require(connect(VolumeSlider, &QSlider::valueChanged, this, &ChannelControls::OnVolumeSlider));
      Require(connect(VolumeValue, qOverload<int>(&QSpinBox::valueChanged), this, &ChannelControls::OnVolumeValue));
      Require(connect(PanSlider, &QSlider::valueChanged, this, &ChannelControls::OnPan));
    }

    QLabel* AddTo(QGridLayout& layout, int row, const QString& name)
    {
      auto* label = new QLabel(name, VolumeSlider->parentWidget());
      layout.addWidget(label, row, 0);
      layout.addWidget(VolumeSlider, row, 1);
      layout.addWidget(VolumeValue, row, 2);
      layout.addWidget(PanSlider, row, 3);
      layout.addWidget(PanValue, row, 4);
      return label;
    }

    void Reset()
    {
      Store(Left.Default, Right.Default);
      Load();
    }

    void UpdateTexts()
    {
      PanValue->setText(PanText(PanSlider->value()));
    }

  private:
    void Load()
    {
      const auto left = Parameters::GetInteger<int>(Storage, Left.Key, Left.Default);
      const auto right = Parameters::GetInteger<int>(Storage, Right.Key, Right.Default);
      const auto vp = FromLevels(left, right);
      const QSignalBlocker blockVolume(VolumeSlider);
      const QSignalBlocker blockValue(VolumeValue);
      const QSignalBlocker blockPan(PanSlider);
      VolumeSlider->setValue(vp.Volume);
      VolumeValue->setValue(vp.Volume);
      // panning is not stored for muted channel, keep the slider where it was
      if (vp.Volume != 0)
      {
        PanSlider->setValue(vp.Pan);
      }
      UpdateTexts();
    }

    void OnVolumeSlider(int value)
    {
      const QSignalBlocker block(VolumeValue);
      VolumeValue->setValue(value);
      Save();
    }

    void OnVolumeValue(int value)
    {
      const QSignalBlocker block(VolumeSlider);
      VolumeSlider->setValue(value);
      Save();
    }

    void OnPan(int /*value*/)
    {
      UpdateTexts();
      Save();
    }

    void Save()
    {
      const auto levels = ToLevels({VolumeSlider->value(), PanSlider->value()});
      Store(levels.first, levels.second);
    }

    void Store(int left, int right)
    {
      Storage.SetValue(Left.Key, left);
      Storage.SetValue(Right.Key, right);
    }

    static QString PanText(int pan)
    {
      if (pan == 0)
      {
        return UI::MixingSettingsWidget::tr("C", "panning");
      }
      return pan < 0 ? UI::MixingSettingsWidget::tr("L %1", "panning").arg(-pan)
                     : UI::MixingSettingsWidget::tr("R %1", "panning").arg(pan);
    }

  private:
    Parameters::Container& Storage;
    const Parameters::ZXTune::Sound::Mixer::Level Left;
    const Parameters::ZXTune::Sound::Mixer::Level Right;
    QSlider* const VolumeSlider;
    QSpinBox* const VolumeValue;
    QSlider* const PanSlider;
    QLabel* const PanValue;
  };

  // Channels group: TurboSound chip on AY page, 3- or 4-channel mixer on DAC page
  struct ChannelsGroup
  {
    QLabel* Header = nullptr;
    std::vector<QLabel*> Names;
  };

  class MixingOptionsWidget
    : public UI::MixingSettingsWidget
    , public UI::Ui_MixingSettingsWidget
  {
  public:
    MixingOptionsWidget(QWidget& parent, Kind kind, PlaybackSupport* playback, const UI::PlaybackState& state)
      : UI::MixingSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
      , PageKind(kind)
      , State(state)
    {
      // setup self
      setupUi(this);

      Description = new QLabel(this);
      Description->setWordWrap(true);
      static_cast<QVBoxLayout*>(layout())->insertWidget(0, Description);
      // playback status is shown next to the reset button
      Status = new QLabel(this);
      Status->setWordWrap(true);
      bottomLayout->addWidget(Status, 1);

      int row = 0;
      AddHeader(row++);
      AddSeparator(row++);
      if (PageKind == AYM)
      {
        using Parameters::ZXTune::Sound::Mixer::AYM_CHANNELS;
        for (unsigned chip = 0; chip != MAX_CHIPS; ++chip)
        {
          auto& group = AddGroup(row++);
          for (unsigned chan = 0; chan != CHIP_CHANNELS; ++chan)
          {
            const auto name = QString::fromLatin1("%1%2").arg(QLatin1Char(static_cast<char>('A' + chan))).arg(chip + 1);
            AddChannel(group, row++, name, AYM_CHANNELS, chip * CHIP_CHANNELS + chan);
          }
        }
      }
      else
      {
        for (unsigned channels = 3; channels <= 4; ++channels)
        {
          auto& group = AddGroup(row++);
          for (unsigned chan = 0; chan != channels; ++chan)
          {
            AddChannel(group, row++, QString::fromLatin1(INPUT_CHANNEL_NAMES[chan]), channels, chan);
          }
        }
      }
      channelsLayout->setColumnStretch(1, 1);
      channelsLayout->setColumnStretch(3, 1);
      Require(connect(resetButton, &QPushButton::clicked, this, &MixingOptionsWidget::Reset));
      if (playback)
      {
        Require(connect(playback, &PlaybackSupport::OnStartModule, this,
                        [this](Sound::Backend::Ptr, Playlist::Item::Data::Ptr item) {
                          State = {std::move(item), false};
                          UpdateStatus();
                        }));
        Require(connect(playback, &PlaybackSupport::OnPauseModule, this, [this]() {
          State.Paused = true;
          UpdateStatus();
        }));
        Require(connect(playback, &PlaybackSupport::OnResumeModule, this, [this]() {
          State.Paused = false;
          UpdateStatus();
        }));
        Require(connect(playback, &PlaybackSupport::OnStopModule, this, [this]() {
          State = {};
          UpdateStatus();
        }));
      }
      SetTexts();
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
        SetTexts();
      }
      UI::MixingSettingsWidget::changeEvent(event);
    }

  private:
    void AddHeader(int row)
    {
      VolumeHeader = new QLabel(this);
      VolumeHeader->setAlignment(Qt::AlignCenter);
      channelsLayout->addWidget(VolumeHeader, row, 1, 1, 2);
      auto* panHeader = new QWidget(this);
      auto* panLayout = new QHBoxLayout(panHeader);
      panLayout->setContentsMargins(0, 0, 0, 0);
      LeftHeader = new QLabel(panHeader);
      PanHeader = new QLabel(panHeader);
      PanHeader->setAlignment(Qt::AlignCenter);
      RightHeader = new QLabel(panHeader);
      panLayout->addWidget(LeftHeader);
      panLayout->addWidget(PanHeader, 1);
      panLayout->addWidget(RightHeader);
      channelsLayout->addWidget(panHeader, row, 3);
    }

    void AddSeparator(int row)
    {
      auto* line = new QFrame(this);
      line->setFrameShape(QFrame::HLine);
      line->setFrameShadow(QFrame::Sunken);
      channelsLayout->addWidget(line, row, 0, 1, 5);
    }

    ChannelsGroup& AddGroup(int row)
    {
      auto* label = new QLabel(this);
      auto font = label->font();
      font.setBold(true);
      label->setFont(font);
      channelsLayout->addWidget(label, row, 0, 1, 5);
      Groups.emplace_back();
      Groups.back().Header = label;
      return Groups.back();
    }

    void AddChannel(ChannelsGroup& group, int row, const QString& name, unsigned channels, unsigned channel)
    {
      auto* controls = new ChannelControls(*this, *Options, channels, channel);
      group.Names.push_back(controls->AddTo(*channelsLayout, row, name));
      Controls.push_back(controls);
    }

    void SetTexts()
    {
      if (PageKind == AYM)
      {
        setWindowTitle(UI::MixingSettingsWidget::tr("AY mixer"));
        Description->setText(UI::MixingSettingsWidget::tr(
            "For AY/YM and TurboSound tunes (PT2, PT3, STC, ASC, SQT, PSG, YM, AY, TS...). "
            "A single chip tune uses AY 1, TurboSound uses AY 1-2 or AY 1-3."));
        for (std::size_t idx = 0; idx != Groups.size(); ++idx)
        {
          Groups[idx].Header->setText(QString::fromLatin1("AY %1").arg(idx + 1));
        }
      }
      else
      {
        setWindowTitle(UI::MixingSettingsWidget::tr("DAC mixer"));
        Description->setText(UI::MixingSettingsWidget::tr(
            "For digital (Covox, SounDrive) tunes. 3 channels: Digital Music Maker, Digital Studio, Sample Tracker. "
            "4 channels: ProDigiTracker, Chip Tracker, SQ-Digital Tracker, Extreme Tracker."));
        Groups[0].Header->setText(UI::MixingSettingsWidget::tr("3 channels"));
        Groups[1].Header->setText(UI::MixingSettingsWidget::tr("4 channels"));
      }
      VolumeHeader->setText(UI::MixingSettingsWidget::tr("Volume"));
      LeftHeader->setText(UI::MixingSettingsWidget::tr("Left"));
      PanHeader->setText(UI::MixingSettingsWidget::tr("Panning"));
      RightHeader->setText(UI::MixingSettingsWidget::tr("Right"));
      for (auto* ctrl : Controls)
      {
        ctrl->UpdateTexts();
      }
      UpdateStatus();
    }

    void UpdateStatus()
    {
      const auto target = State.Item ? GetMixerTarget(*State.Item) : MixerTarget();
      const bool applies = target.Valid && target.Kind == PageKind;
      Status->setText(GetStatusText(target, applies));
      // groups used by the current tune are highlighted, all are editable anyway
      for (std::size_t idx = 0; idx != Groups.size(); ++idx)
      {
        const bool used = !applies || IsGroupUsed(idx, target.Count);
        Groups[idx].Header->setEnabled(used);
        for (auto* name : Groups[idx].Names)
        {
          name->setEnabled(used);
        }
      }
    }

    bool IsGroupUsed(std::size_t group, unsigned count) const
    {
      return PageKind == AYM ? group < count : group + 3 == count;
    }

    QString GetStatusText(const MixerTarget& target, bool applies) const
    {
      if (!State.Item)
      {
        return UI::MixingSettingsWidget::tr("Nothing is playing.");
      }
      const auto type = ToQString(State.Item->GetType());
      if (!applies)
      {
        const auto head = State.Paused ? UI::MixingSettingsWidget::tr("Paused: %1.").arg(type)
                                       : UI::MixingSettingsWidget::tr("Now playing: %1.").arg(type);
        if (!target.Valid)
        {
          return head + QLatin1Char(' ') + UI::MixingSettingsWidget::tr("No mixer page applies to it.");
        }
        return head + QLatin1Char(' ')
               + (target.Kind == AYM ? UI::MixingSettingsWidget::tr("Not this page but the AY mixer applies to it.")
                                     : UI::MixingSettingsWidget::tr("Not this page but the DAC mixer applies to it."));
      }
      const auto count = static_cast<int>(target.Count);
      QString head;
      QString tail;
      if (PageKind == AYM)
      {
        head = State.Paused ? UI::MixingSettingsWidget::tr("Paused: %1, %n AY chip(s).", nullptr, count).arg(type)
                            : UI::MixingSettingsWidget::tr("Now playing: %1, %n AY chip(s).", nullptr, count).arg(type);
        tail = count == 1   ? UI::MixingSettingsWidget::tr("AY 1 of this page applies to it.")
               : count == 2 ? UI::MixingSettingsWidget::tr("AY 1 and AY 2 of this page apply to it.")
                            : UI::MixingSettingsWidget::tr("All three chips of this page apply to it.");
      }
      else
      {
        head = State.Paused ? UI::MixingSettingsWidget::tr("Paused: %1, %n channel(s).", nullptr, count).arg(type)
                            : UI::MixingSettingsWidget::tr("Now playing: %1, %n channel(s).", nullptr, count).arg(type);
        tail = count == 3 ? UI::MixingSettingsWidget::tr("The '3 channels' group of this page applies to it.")
                          : UI::MixingSettingsWidget::tr("The '4 channels' group of this page applies to it.");
      }
      return head + QLatin1Char(' ') + tail;
    }

    void Reset()
    {
      for (auto* ctrl : Controls)
      {
        ctrl->Reset();
      }
    }

  private:
    const Parameters::Container::Ptr Options;
    const Kind PageKind;
    UI::PlaybackState State;
    std::vector<ChannelControls*> Controls;
    std::vector<ChannelsGroup> Groups;
    QLabel* Description = nullptr;
    QLabel* Status = nullptr;
    QLabel* VolumeHeader = nullptr;
    QLabel* LeftHeader = nullptr;
    QLabel* PanHeader = nullptr;
    QLabel* RightHeader = nullptr;
  };
}  // namespace

namespace UI
{
  MixingSettingsWidget::MixingSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  MixingSettingsWidget* MixingSettingsWidget::Create(QWidget& parent, Kind kind, PlaybackSupport* playback,
                                                     const PlaybackState& state)
  {
    return new MixingOptionsWidget(parent, kind, playback, state);
  }

  bool MixingSettingsWidget::GetKind(const Playlist::Item::Data& item, Kind& kind)
  {
    const auto target = GetMixerTarget(item);
    if (target.Valid)
    {
      kind = target.Kind;
    }
    return target.Valid;
  }
}  // namespace UI
