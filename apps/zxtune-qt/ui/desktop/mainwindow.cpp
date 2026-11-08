/**
 *
 * @file
 *
 * @brief Main window implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/desktop/mainwindow.h"

#include "apps/zxtune-qt/playlist/ui/container_view.h"
#include "apps/zxtune-qt/supp/playback_supp.h"
#include "apps/zxtune-qt/ui/controls/analyzer_control.h"
#include "apps/zxtune-qt/ui/controls/playback_controls.h"
#include "apps/zxtune-qt/ui/controls/playback_options.h"
#include "apps/zxtune-qt/ui/controls/seek_controls.h"
#include "apps/zxtune-qt/ui/controls/status_control.h"
#include "apps/zxtune-qt/ui/controls/volume_control.h"
#include "apps/zxtune-qt/ui/desktop/language.h"
#include "apps/zxtune-qt/ui/format.h"
#include "apps/zxtune-qt/ui/informational/aboutdialog.h"
#include "apps/zxtune-qt/ui/informational/componentsdialog.h"
#include "apps/zxtune-qt/ui/parameters.h"
#include "apps/zxtune-qt/ui/preferences/preferencesdialog.h"
#include "apps/zxtune-qt/ui/state.h"
#include "apps/zxtune-qt/ui/tools/errordialog.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "apps/zxtune-qt/update/check.h"
#include "apps/zxtune-qt/urls.h"
#include "mainwindow.ui.h"

#include "debug/log.h"
#include "platform/version/api.h"
#include "strings/format.h"

#include "contract.h"
#include "make_ptr.h"

#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtGui/QCloseEvent>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolBar>

#include <utility>

namespace
{
  const Debug::Stream Dbg("UI::MainWindow");

  UI::Language::Ptr CreateLanguage(const Parameters::Container& options)
  {
    auto res = UI::Language::Create();
    const auto lang =
        Parameters::Find<Parameters::ZXTuneQT::UI::Language>(options).value_or(FromQString(res->GetSystem()));
    res->Set(ToQString(lang));
    return res;
  }

  // Volume slider, its value and the mixer button
  const int VOLUME_TOOLBAR_MIN_WIDTH = 300;

  class DesktopMainWindowImpl
    : public DesktopMainWindow
    , public Ui::MainWindow
  {
  public:
    explicit DesktopMainWindowImpl(Parameters::Container::Ptr options)
      : Options(std::move(options))
      , Language(CreateLanguage(*Options))
      , Playback(PlaybackSupport::Create(*this, Options))
      , Controls(PlaybackControls::Create(*this, *Playback))
      , FastOptions(PlaybackOptions::Create(*this, *Playback, Options))
      , Volume(VolumeControl::Create(*this, *Playback))
      , Status(StatusControl::Create(*this, *Playback))
      , Seeking(SeekControls::Create(*this, *Playback))
      , Analyzer(AnalyzerControl::Create(*this, *Playback))
      , MultiPlaylist(Playlist::UI::ContainerView::Create(*this, Options))
    {
      setupUi(this);
      State = UI::State::Create(*this);
      // fill menu
      menubar->addMenu(Controls->GetActionsMenu());
      menubar->addMenu(MultiPlaylist->GetActionsMenu());
      menubar->addMenu(menuHelp);
      // fill toolbar and layout menu
      {
        Toolbars.push_back(AddWidgetOnToolbar(Controls, false));
        Toolbars.push_back(AddWidgetOnToolbar(FastOptions, false));
        Toolbars.push_back(AddWidgetOnToolbar(Volume, true));
        VolumeToolbar = Toolbars.back().second;
        Toolbars.push_back(AddWidgetOnToolbar(Status, false));
        Toolbars.push_back(AddWidgetOnToolbar(Seeking, true));
        Toolbars.push_back(AddWidgetOnToolbar(Analyzer, false));
        // playlist is mandatory and cannot be hidden
        AddWidgetOnLayout(MultiPlaylist);
        State->Load();
        FillLayoutMenu();
      }

      // connect root actions
      Require(connect(actionComponents, &QAction::triggered, this, &DesktopMainWindowImpl::ShowComponentsInformation));
      Require(connect(actionAbout, &QAction::triggered, this, &DesktopMainWindowImpl::ShowAboutProgram));
      Require(connect(actionOnlineHelp, &QAction::triggered, this, &DesktopMainWindowImpl::VisitHelp));
      Require(connect(actionWebSite, &QAction::triggered, this, &DesktopMainWindowImpl::VisitSite));
      Require(connect(actionOnlineFAQ, &QAction::triggered, this, &DesktopMainWindowImpl::VisitFAQ));
      Require(connect(actionReportBug, &QAction::triggered, this, &DesktopMainWindowImpl::ReportIssue));
      Require(connect(actionAboutQt, &QAction::triggered, this, &DesktopMainWindowImpl::ShowAboutQt));
      Require(connect(actionPreferences, &QAction::triggered, this, &DesktopMainWindowImpl::ShowPreferences));
      // mixer page applicable to the current tune, from menu and next to volume control
      ActionMixer = new QAction(this);
      ActionMixer->setMenuRole(QAction::NoRole);
      Controls->GetActionsMenu()->addSeparator();
      Controls->GetActionsMenu()->addAction(ActionMixer);
      Require(connect(ActionMixer, &QAction::triggered, this, &DesktopMainWindowImpl::ShowMixer));
      MixerButton = new QPushButton(this);
      VolumeToolbar->addWidget(MixerButton);
      VolumeToolbar->setMinimumWidth(VOLUME_TOOLBAR_MIN_WIDTH);
      Require(connect(MixerButton, &QPushButton::clicked, this, &DesktopMainWindowImpl::ShowMixer));
      SetMixerActionText();
      UpdateMinimumWidth();
      if (Update::CheckOperation* op = Update::CheckOperation::Create(*this))
      {
        Require(connect(actionCheckUpdates, &QAction::triggered, op, &Update::CheckOperation::Execute));
      }
      else
      {
        actionCheckUpdates->setEnabled(false);
      }

      Require(connect(Controls, &PlaybackControls::OnPrevious, MultiPlaylist, &Playlist::UI::ContainerView::Prev));
      Require(connect(Controls, &PlaybackControls::OnNext, MultiPlaylist, &Playlist::UI::ContainerView::Next));
      Require(
          connect(Playback, &PlaybackSupport::OnStartModule, MultiPlaylist,
                  [playlists = MultiPlaylist](Sound::Backend::Ptr, Playlist::Item::Data::Ptr) { playlists->Play(); }));
      Require(connect(Playback, &PlaybackSupport::OnResumeModule, MultiPlaylist, &Playlist::UI::ContainerView::Play));
      Require(connect(Playback, &PlaybackSupport::OnPauseModule, MultiPlaylist, &Playlist::UI::ContainerView::Pause));
      Require(connect(Playback, &PlaybackSupport::OnStopModule, MultiPlaylist, &Playlist::UI::ContainerView::Stop));
      Require(connect(Playback, &PlaybackSupport::OnFinishModule, MultiPlaylist, &Playlist::UI::ContainerView::Finish));
      Require(
          connect(MultiPlaylist, &Playlist::UI::ContainerView::Activated, Playback, &PlaybackSupport::SetDefaultItem));
      Require(connect(MultiPlaylist, &Playlist::UI::ContainerView::ItemActivated, Playback, &PlaybackSupport::SetItem));
      Require(connect(MultiPlaylist, &Playlist::UI::ContainerView::Deactivated, Playback, &PlaybackSupport::ResetItem));
      Require(connect(Playback, &PlaybackSupport::OnStartModule, this, &DesktopMainWindowImpl::StartModule));
      Require(connect(Playback, &PlaybackSupport::OnStopModule, this, &DesktopMainWindowImpl::StopModule));
      Require(connect(Playback, &PlaybackSupport::OnPauseModule, this, [this]() { PlayState.Paused = true; }));
      Require(connect(Playback, &PlaybackSupport::OnResumeModule, this, [this]() { PlayState.Paused = false; }));
      Require(connect(Playback, &PlaybackSupport::ErrorOccurred, this, &DesktopMainWindowImpl::ShowError));
      Require(connect(actionAddFiles, &QAction::triggered, MultiPlaylist, &Playlist::UI::ContainerView::AddFiles));
      Require(connect(actionAddFolder, &QAction::triggered, MultiPlaylist, &Playlist::UI::ContainerView::AddFolder));

      StopModule();

      MultiPlaylist->Setup();
    }

    void SetCmdline(const QStringList& args) override
    {
      if (this->sender())
      {
        setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        raise();
        activateWindow();
      }
      if (!args.empty())
      {
        MultiPlaylist->Open(args);
      }
    }

    // QWidgets virtuals
    void closeEvent(QCloseEvent* event) override
    {
      Playback->Stop();
      State->Save();
      MultiPlaylist->Teardown();
      event->accept();
    }

    bool event(QEvent* event) override
    {
      const bool res = ::MainWindow::event(event);
      if (event && QEvent::LanguageChange == event->type())
      {
        FillLayoutMenu();
      }
      return res;
    }

    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        Dbg("Retranslate main UI");
        retranslateUi(this);
        SetMixerActionText();
      }
      ::MainWindow::changeEvent(event);
    }

  private:
    using WidgetOnToolbar = std::pair<QWidget*, QToolBar*>;

    WidgetOnToolbar AddWidgetOnToolbar(QWidget* widget, bool lastInRow)
    {
      auto* const toolBar = new QToolBar(this);
      toolBar->setObjectName(widget->objectName());
      QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
      sizePolicy.setHorizontalStretch(0);
      sizePolicy.setVerticalStretch(0);
      sizePolicy.setHeightForWidth(toolBar->sizePolicy().hasHeightForWidth());
      toolBar->setSizePolicy(sizePolicy);
      toolBar->setAllowedAreas(Qt::TopToolBarArea);
      toolBar->setFloatable(false);
      toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
      toolBar->addWidget(widget);

      addToolBar(Qt::TopToolBarArea, toolBar);
      if (lastInRow)
      {
        addToolBarBreak();
      }
      return {widget, toolBar};
    }

    QWidget* AddWidgetOnLayout(QWidget* widget)
    {
      centralWidget()->layout()->addWidget(widget);
      return widget;
    }

    void FillLayoutMenu()
    {
      menuLayout->clear();
      for (const auto& toolbar : Toolbars)
      {
        toolbar.second->setWindowTitle(toolbar.first->windowTitle());
        auto* action = toolbar.second->toggleViewAction();
        action->setMenuRole(QAction::NoRole);
        menuLayout->addAction(action);
      }
    }

    void StartModule(Sound::Backend::Ptr /*player*/, Playlist::Item::Data::Ptr item)
    {
      setWindowTitle(
          ToQString(Strings::Format("{1} [{0}]", Platform::Version::GetProgramTitle(), item->GetDisplayName())));
      PlayState = {std::move(item), false};
      // channel buttons are changed by playback options, wait for their layout
      QTimer::singleShot(0, this, &DesktopMainWindowImpl::UpdateMinimumWidth);
    }

    // Tunes with more channels (e.g. TurboSound) have more channel buttons on the toolbars row,
    // the window may grow to keep the volume toolbar wide enough
    void UpdateMinimumWidth()
    {
      int rowWidth = 0;
      for (const auto& toolbar : Toolbars)
      {
        if (toolbar.second == VolumeToolbar)
        {
          break;
        }
        if (toolbar.second->isVisible())
        {
          // size hints are cached by layouts and are not reduced when buttons get hidden
          for (auto* layout : toolbar.second->findChildren<QLayout*>())
          {
            layout->invalidate();
          }
          rowWidth += toolbar.second->sizeHint().width();
        }
      }
      // toolbar handles, spacing and window frame
      const int MARGINS = 16;
      setMinimumWidth(rowWidth + VOLUME_TOOLBAR_MIN_WIDTH + MARGINS);
    }

    void StopModule()
    {
      PlayState = {};
      setWindowTitle(ToQString(Platform::Version::GetProgramTitle()));
    }

    void ShowPreferences()
    {
      UI::ShowPreferencesDialog(*this, *Playback, PlayState);
    }

    void ShowMixer()
    {
      UI::ShowMixerDialog(*this, *Playback, PlayState);
    }

    void SetMixerActionText()
    {
      ActionMixer->setText(UI::MixingSettingsWidget::tr("Mixer..."));
      MixerButton->setText(UI::MixingSettingsWidget::tr("Mixer"));
      MixerButton->setToolTip(UI::MixingSettingsWidget::tr("Mixer settings of the current tune"));
    }

    void ShowComponentsInformation()
    {
      UI::ShowComponentsInformation(*this);
    }

    void ShowAboutProgram()
    {
      UI::ShowProgramInformation(*this);
    }

    void ShowAboutQt()
    {
      QMessageBox::aboutQt(this);
    }

    void VisitHelp()
    {
      QDesktopServices::openUrl(ToQString(Urls::Help()));
    }

    void VisitSite()
    {
      QDesktopServices::openUrl(ToQString(Urls::Site()));
    }

    void VisitFAQ()
    {
      QDesktopServices::openUrl(ToQString(Urls::Faq()));
    }

    void ReportIssue()
    {
      QDesktopServices::openUrl(ToQString(Urls::Bugreport()));
    }

    void ShowError(const Error& err)
    {
      ShowErrorMessage(QString(), err);
    }

  private:
    const Parameters::Container::Ptr Options;
    const UI::Language::Ptr Language;
    UI::State::Ptr State;
    PlaybackSupport* const Playback;
    PlaybackControls* const Controls;
    PlaybackOptions* const FastOptions;
    VolumeControl* const Volume;
    StatusControl* const Status;
    SeekControls* const Seeking;
    AnalyzerControl* const Analyzer;
    Playlist::UI::ContainerView* const MultiPlaylist;
    UI::PlaybackState PlayState;
    QAction* ActionMixer = nullptr;
    QToolBar* VolumeToolbar = nullptr;
    QPushButton* MixerButton = nullptr;
    std::vector<WidgetOnToolbar> Toolbars;
  };
}  // namespace

MainWindow::Ptr DesktopMainWindow::Create(Parameters::Container::Ptr options)
{
  auto res = MakePtr<DesktopMainWindowImpl>(std::move(options));
  res->show();
  return res;
}
