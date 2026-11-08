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
#include "mixing.ui.h"

#include "sound/mixer_parameters.h"

#include "contract.h"

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>

#include <algorithm>
#include <utility>
#include <vector>

namespace
{
  const char* INPUT_CHANNEL_NAMES[4] = {"A", "B", "C", "D"};
  // TurboSound mixers have 3 channels per chip
  const unsigned CHIP_CHANNELS = 3;

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

    void AddTo(QGridLayout& layout, int row, const QString& name)
    {
      auto* label = new QLabel(name, VolumeSlider->parentWidget());
      layout.addWidget(label, row, 0);
      layout.addWidget(VolumeSlider, row, 1);
      layout.addWidget(VolumeValue, row, 2);
      layout.addWidget(PanSlider, row, 3);
      layout.addWidget(PanValue, row, 4);
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

  class MixingOptionsWidget
    : public UI::MixingSettingsWidget
    , public UI::Ui_MixingSettingsWidget
  {
  public:
    MixingOptionsWidget(QWidget& parent, unsigned channels)
      : UI::MixingSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
      , Channels(channels)
    {
      // setup self
      setupUi(this);

      int row = 0;
      AddHeader(row++);
      AddSeparator(row++);
      const bool byChips = Channels > 4;
      for (unsigned inChan = 0; inChan != Channels; ++inChan)
      {
        const auto chip = inChan / CHIP_CHANNELS;
        if (byChips && inChan % CHIP_CHANNELS == 0)
        {
          AddChipHeader(row++, chip);
        }
        auto* controls = new ChannelControls(*this, *Options, Channels, inChan);
        const auto name = byChips ? QString::fromLatin1("%1%2")
                                        .arg(QLatin1Char(static_cast<char>('A' + inChan % CHIP_CHANNELS)))
                                        .arg(chip + 1)
                                  : QString::fromLatin1(INPUT_CHANNEL_NAMES[inChan]);
        controls->AddTo(*channelsLayout, row++, name);
        Controls.push_back(controls);
      }
      channelsLayout->setColumnStretch(1, 1);
      channelsLayout->setColumnStretch(3, 1);
      Require(connect(resetButton, &QPushButton::clicked, this, &MixingOptionsWidget::Reset));
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

    void AddChipHeader(int row, unsigned chip)
    {
      auto* label = new QLabel(QString::fromLatin1("AY %1").arg(chip + 1), this);
      auto font = label->font();
      font.setBold(true);
      label->setFont(font);
      channelsLayout->addWidget(label, row, 0, 1, 5);
    }

    void SetTexts()
    {
      setWindowTitle(UI::MixingSettingsWidget::tr("%1-channels mixer").arg(Channels));
      VolumeHeader->setText(UI::MixingSettingsWidget::tr("Volume"));
      LeftHeader->setText(UI::MixingSettingsWidget::tr("Left"));
      PanHeader->setText(UI::MixingSettingsWidget::tr("Panning"));
      RightHeader->setText(UI::MixingSettingsWidget::tr("Right"));
      for (auto* ctrl : Controls)
      {
        ctrl->UpdateTexts();
      }
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
    const unsigned Channels;
    std::vector<ChannelControls*> Controls;
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

  MixingSettingsWidget* MixingSettingsWidget::Create(QWidget& parent, unsigned channels)
  {
    return new MixingOptionsWidget(parent, channels);
  }
}  // namespace UI
