/**
 *
 * @file
 *
 * @brief Preferences dialog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/preferencesdialog.h"

#include "apps/zxtune-qt/supp/playback_supp.h"
#include "apps/zxtune-qt/ui/preferences/aym.h"
#include "apps/zxtune-qt/ui/preferences/interface.h"
#include "apps/zxtune-qt/ui/preferences/mixing.h"
#include "apps/zxtune-qt/ui/preferences/plugins.h"
#include "apps/zxtune-qt/ui/preferences/saa.h"
#include "apps/zxtune-qt/ui/preferences/sid.h"
#include "apps/zxtune-qt/ui/preferences/sound.h"
#include "apps/zxtune-qt/ui/preferences/z80.h"
#include "apps/zxtune-qt/ui/state.h"
#include "preferencesdialog.ui.h"

#include "contract.h"

#include <QtCore/QPointer>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>
#include <utility>

namespace
{
  class PreferencesDialog
    : public QDialog
    , public Ui::PreferencesDialog
  {
  public:
    PreferencesDialog(QWidget& parent, PlaybackSupport& playback, const UI::PlaybackState& state)
      : QDialog(&parent)
    {
      setupUi(this);
      State = UI::State::Create(*this);
      // fill
      QWidget* const soundSettingsPage = UI::SoundSettingsWidget::Create(*Categories);
      AYMixerPage = UI::MixingSettingsWidget::Create(*Categories, UI::MixingSettingsWidget::AYM, &playback, state);
      DACMixerPage = UI::MixingSettingsWidget::Create(*Categories, UI::MixingSettingsWidget::DAC, &playback, state);
      QWidget* const pages[] = {UI::AYMSettingsWidget::Create(*Categories),
                                UI::SAASettingsWidget::Create(*Categories),
                                UI::SIDSettingsWidget::Create(*Categories),
                                UI::Z80SettingsWidget::Create(*Categories),
                                soundSettingsPage,
                                AYMixerPage,
                                DACMixerPage,
                                UI::PluginsSettingsWidget::Create(*Categories),
                                UI::InterfaceSettingsWidget::Create(*Categories)};
      for (auto* p : pages)
      {
        Categories->addTab(p, p->windowTitle());
      }
      const auto soundPageIndex = static_cast<int>(std::find(pages, std::end(pages), soundSettingsPage) - pages);
      Categories->setTabEnabled(soundPageIndex, !state.Item);
      // dialog is modeless, so playback may start or stop while it's opened
      Require(connect(&playback, &PlaybackSupport::OnStartModule, this,
                      [this, soundPageIndex](Sound::Backend::Ptr, Playlist::Item::Data::Ptr) {
                        Categories->setTabEnabled(soundPageIndex, false);
                      }));
      Require(connect(&playback, &PlaybackSupport::OnStopModule, this,
                      [this, soundPageIndex]() { Categories->setTabEnabled(soundPageIndex, true); }));
      Require(connect(this, &QDialog::finished, this, &PreferencesDialog::OnFinished));
      State->AddWidget(*Categories);
      State->Load();
    }

    void ShowMixer(const UI::PlaybackState& state)
    {
      auto kind = UI::MixingSettingsWidget::AYM;
      if (state.Item)
      {
        UI::MixingSettingsWidget::GetKind(*state.Item, kind);
      }
      Categories->setCurrentWidget(kind == UI::MixingSettingsWidget::AYM ? AYMixerPage : DACMixerPage);
    }

    void Activate()
    {
      show();
      raise();
      activateWindow();
    }

  private:
    void OnFinished()
    {
      State->Save();
      deleteLater();
    }

  public:
    // QWidgets virtuals

    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
        for (int idx = 0, lim = Categories->count(); idx != lim; ++idx)
        {
          QWidget* const tab = Categories->widget(idx);
          // tab->changeEvent(event);
          Categories->setTabText(idx, tab->windowTitle());
        }
      }
      else
      {
        QDialog::changeEvent(event);
      }
    }

  private:
    UI::State::Ptr State;
    QWidget* AYMixerPage = nullptr;
    QWidget* DACMixerPage = nullptr;
  };
}  // namespace

namespace UI
{
  // Modeless and single: playback can be controlled while settings are opened
  PreferencesDialog& GetPreferencesDialog(QWidget& parent, PlaybackSupport& playback, const PlaybackState& state)
  {
    static QPointer<PreferencesDialog> instance;
    if (!instance)
    {
      instance = new PreferencesDialog(parent, playback, state);
    }
    return *instance;
  }

  void ShowPreferencesDialog(QWidget& parent, PlaybackSupport& playback, const PlaybackState& state)
  {
    GetPreferencesDialog(parent, playback, state).Activate();
  }

  void ShowMixerDialog(QWidget& parent, PlaybackSupport& playback, const PlaybackState& state)
  {
    auto& dialog = GetPreferencesDialog(parent, playback, state);
    dialog.ShowMixer(state);
    dialog.Activate();
  }
}  // namespace UI
