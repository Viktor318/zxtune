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

#include <QtGui/QCloseEvent>
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
      Categories->setTabEnabled(std::find(pages, std::end(pages), soundSettingsPage) - pages, !state.Item);
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

    // QWidgets virtuals
    void closeEvent(QCloseEvent* event) override
    {
      State->Save();
      event->accept();
    }

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
  void ShowPreferencesDialog(QWidget& parent, PlaybackSupport& playback, const PlaybackState& state)
  {
    PreferencesDialog dialog(parent, playback, state);
    dialog.exec();
  }

  void ShowMixerDialog(QWidget& parent, PlaybackSupport& playback, const PlaybackState& state)
  {
    PreferencesDialog dialog(parent, playback, state);
    dialog.ShowMixer(state);
    dialog.exec();
  }
}  // namespace UI
