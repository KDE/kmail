/*
 * kmail: KDE mail client
 * SPDX-FileCopyrightText: 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "tag/tagactionmanager.h"
#include <Libkdepim/ProgressStatusBarWidget>
#include <Libkdepim/StatusbarProgressWidget>
#include <PimCommon/BroadcastStatus>
#include <PimCommon/NeedUpdateVersionUtils>
#include <PimCommon/NeedUpdateVersionWidget>

#include <KConfigGroup>
#include <KToolBar>
#include <QApplication>
#include <QStatusBar>
#include <QTimer>

#include <KAboutData>
#include <KConfig>
#include <KConfigGui>
#include <KEditToolBar>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>
#include <KXMLGUIFactory>
#include <QMenuBar>
#include <QWindow>

#include <QLabel>
#include <QVBoxLayout>

// signal handler for SIGINT & SIGTERM
#ifdef Q_OS_UNIX
#include <KSignalHandler>
#include <signal.h>
#include <unistd.h>
#endif

#include <chrono>

using namespace std::chrono_literals;

KMMainWin::KMMainWin(QWidget *)
    : KXmlGuiWindow(nullptr)
    , mMessageLabel(new QLabel(i18n("Starting...")))

{
#ifdef Q_OS_UNIX
    /**
     * Set up signal handler for SIGINT and SIGTERM
     */
    KSignalHandler::self()->watchSignal(SIGINT);
    KSignalHandler::self()->watchSignal(SIGTERM);
    connect(KSignalHandler::self(), &KSignalHandler::signalReceived, this, [this](int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            // Intercept console.
            printf("Shutting down...\n");
            slotQuit();
        }
    });
#endif
    setObjectName(QLatin1StringView("kmail-mainwindow#"));

    resize(700, 500); // The default size

    auto mainWidget = new QWidget(this);
    auto mainWidgetLayout = new QVBoxLayout(mainWidget);
    mainWidgetLayout->setContentsMargins({});
    if (PimCommon::NeedUpdateVersionUtils::checkVersion()) {
        const auto status = PimCommon::NeedUpdateVersionUtils::obsoleteVersionStatus(KAboutData::applicationData().version(), QDate::currentDate());
        if (status != PimCommon::NeedUpdateVersionUtils::ObsoleteVersion::NotObsoleteYet) {
            auto needUpdateVersionWidget = new PimCommon::NeedUpdateVersionWidget(this);
            mainWidgetLayout->addWidget(needUpdateVersionWidget);
            qDebug() << " KAboutData::applicationData().version() " << KAboutData::applicationData().version();
            needUpdateVersionWidget->setObsoleteVersion(status);
        }
    }

    mKMMainWidget = new KMMainWidget(this, this, actionCollection());
    mainWidgetLayout->addWidget(mKMMainWidget);

    // Don't initialize in constructor. We need this statusbar created
    // Bug 460289
    mProgressBar = new KPIM::ProgressStatusBarWidget(statusBar(), this);
    connect(mKMMainWidget, &KMMainWidget::recreateGui, this, &KMMainWin::slotUpdateGui);
    setCentralWidget(mainWidget);
    setupStatusBar();
    if (!kmkernel->xmlGuiInstanceName().isEmpty()) {
        setComponentName(kmkernel->xmlGuiInstanceName(), i18n("KMail2"));
    }
    setStandardToolBarMenuEnabled(true);

    KStandardAction::configureToolbars(this, &KMMainWin::slotEditToolbars, actionCollection());

    KStandardAction::keyBindings(this, &KMMainWin::slotConfigureShortcuts, actionCollection());

    mShowMenuBarAction = KStandardAction::showMenubar(this, &KMMainWin::slotToggleMenubar, actionCollection());
    if (menuBar()) {
        mHamburgerMenu = KStandardAction::hamburgerMenu(nullptr, nullptr, actionCollection());
        mHamburgerMenu->setShowMenuBarAction(mShowMenuBarAction);
        mHamburgerMenu->setMenuBar(menuBar());
        connect(mHamburgerMenu, &KHamburgerMenu::aboutToShowMenu, this, [this]() {
            updateHamburgerMenu();
            // Immediately disconnect. We only need to run this once, but on demand.
            // NOTE: The nullptr at the end disconnects all connections between
            // q and mHamburgerMenu's aboutToShowMenu signal.
            disconnect(mHamburgerMenu, &KHamburgerMenu::aboutToShowMenu, this, nullptr);
        });
    }

    KStandardAction::quit(this, &KMMainWin::slotQuit, actionCollection());
    createGUI(QStringLiteral("kmmainwin.rc"));

    // must be after createGUI, otherwise e.g toolbar settings are not loaded
    setAutoSaveSettings(KMKernel::self()->config()->group(QStringLiteral("Main Window")));

    connect(PimCommon::BroadcastStatus::instance(), &PimCommon::BroadcastStatus::statusMsg, this, &KMMainWin::displayStatusMessage);

    connect(mKMMainWidget, &KMMainWidget::captionChangeRequest, this, qOverload<const QString &>(&KMainWindow::setCaption));

    mKMMainWidget->updateQuickSearchLineText();
    mShowMenuBarAction->setChecked(KMailSettings::self()->showMenuBar());
    slotToggleMenubar(true);
    connect(guiFactory(), &KXMLGUIFactory::shortcutsSaved, this, &KMMainWin::slotShortcutSaved);

    mShowFullScreenAction = KStandardAction::fullScreen(nullptr, nullptr, this, actionCollection());
    actionCollection()->setDefaultShortcut(mShowFullScreenAction, Qt::Key_F11);
    connect(mShowFullScreenAction, &QAction::toggled, this, &KMMainWin::slotFullScreen);
    KMKernel::self()->setSystemTryAssociatedWindow(windowHandle());
}

KMMainWin::~KMMainWin()
{
    // Avoids a crash if there are any Akonadi jobs running, which may
    // attempt to display a status message when they are killed.
    disconnect(PimCommon::BroadcastStatus::instance(), &PimCommon::BroadcastStatus::statusMsg, this, nullptr);
}

void KMMainWin::slotFullScreen(bool t)
{
    KToggleFullScreenAction::setFullScreen(this, t);
    QMenuBar *mb = menuBar();
    if (t) {
        auto b = new QToolButton(mb);
        b->setDefaultAction(mShowFullScreenAction);
        b->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored));
        b->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
        mb->setCornerWidget(b, Qt::TopRightCorner);
        b->setVisible(true);
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    } else {
        QWidget *w = mb->cornerWidget(Qt::TopRightCorner);
        if (w) {
            w->deleteLater();
        }
    }
}

void KMMainWin::updateHamburgerMenu()
{
    delete mHamburgerMenu->menu();
    auto menu = new QMenu(this);
    menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Open)));
    menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::SaveAs)));
    menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Print)));
    menu->addSeparator();
    menu->addAction(actionCollection()->action(QStringLiteral("check_mail")));
    menu->addAction(actionCollection()->action(QStringLiteral("check_mail_in")));
    menu->addAction(actionCollection()->action(QStringLiteral("send_queued")));
    menu->addAction(actionCollection()->action(QStringLiteral("send_queued_via")));
    menu->addSeparator();

    menu->addAction(actionCollection()->action(QStringLiteral("kmail_configure_kmail")));
    menu->addAction(actionCollection()->action(QStringLiteral("kmail_configure_notifications")));
    menu->addSeparator();

    menu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Quit)));
    mHamburgerMenu->setMenu(menu);
}

KMMainWidget *KMMainWin::mainKMWidget() const
{
    return mKMMainWidget;
}

void KMMainWin::displayStatusMessage(const QString &aText)
{
    if (!statusBar() || !mProgressBar->littleProgress()) {
        return;
    }
    const int statusWidth = statusBar()->width() - mProgressBar->littleProgress()->width() - fontMetrics().maxWidth();

    const QString text = fontMetrics().elidedText(QLatin1Char(' ') + aText, Qt::ElideRight, statusWidth);

    // ### FIXME: We should disable richtext/HTML (to avoid possible denial of service attacks),
    // but this code would double the size of the status bar if the user hovers
    // over an <foo@bar.com>-style email address :-(
    //  text.replace("&", "&amp;");
    //  text.replace("<", "&lt;");
    //  text.replace(">", "&gt;");
    mMessageLabel->setText(text);
    mMessageLabel->setToolTip(aText);
}

void KMMainWin::slotToggleMenubar(bool dontShowWarning)
{
    if (menuBar()) {
        if (mShowMenuBarAction->isChecked()) {
            menuBar()->show();
        } else {
            if (!dontShowWarning && (!toolBar()->isVisible() || !toolBar()->actions().contains(mHamburgerMenu))) {
                const QString accel = mShowMenuBarAction->shortcut().toString();
                KMessageBox::information(this,
                                         i18n("<qt>This will hide the menu bar completely."
                                              " You can show it again by typing %1.</qt>",
                                              accel),
                                         i18nc("@title:window", "Hide menu bar"),
                                         QStringLiteral("HideMenuBarWarning"));
            }
            menuBar()->hide();
        }
        KMailSettings::self()->setShowMenuBar(mShowMenuBarAction->isChecked());
    }
}

void KMMainWin::slotEditToolbars()
{
    KConfigGroup grp = KMKernel::self()->config()->group(QStringLiteral("Main Window"));
    saveMainWindowSettings(grp);
    QPointer<KEditToolBar> dlg = new KEditToolBar(guiFactory(), this);
    connect(dlg.data(), &KEditToolBar::newToolBarConfig, this, &KMMainWin::slotUpdateGui);

    dlg->exec();
    delete dlg;
}

void KMMainWin::slotUpdateGui()
{
    // remove dynamically created actions before editing
    mKMMainWidget->clearFilterActions();
    mKMMainWidget->tagActionManager()->clearActions();
    mKMMainWidget->clearPluginActions();

    createGUI(QStringLiteral("kmmainwin.rc"));
    applyMainWindowSettings(KMKernel::self()->config()->group(QStringLiteral("Main Window")));

    // plug dynamically created actions again
    mKMMainWidget->initializeFilterActions(false);
    mKMMainWidget->tagActionManager()->createActions();
    // FIXME mKMMainWidget->initializePluginActions();
}

void KMMainWin::setupStatusBar()
{
    /* Create a progress dialog and hide it. */
    mMessageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusBar()->addWidget(mMessageLabel);

    QTimer::singleShot(2s, PimCommon::BroadcastStatus::instance(), &PimCommon::BroadcastStatus::reset);
    statusBar()->addPermanentWidget(mKMMainWidget->dkimWidgetInfo());
    statusBar()->addPermanentWidget(mKMMainWidget->zoomLabelIndicator());
    statusBar()->addPermanentWidget(mKMMainWidget->vacationScriptIndicator());
    statusBar()->addPermanentWidget(mProgressBar->littleProgress());
}

void KMMainWin::slotQuit()
{
    mReallyClose = true;
    close();
}

//-----------------------------------------------------------------------------
bool KMMainWin::restoreDockedState(int n)
{
    // Default restore behavior is to show the window once it is restored.
    // Override this if the main window was hidden in the system tray
    // when the session was saved.
    KConfigGroup config(KConfigGui::sessionConfig(), QString::number(n));
    const bool show = !config.readEntry("docked", false);

    return KMainWindow::restore(n, show);
}

void KMMainWin::showAndActivateWindow()
{
    show();
    raise();
    activateWindow();
}

void KMMainWin::saveProperties(KConfigGroup &config)
{
    // This is called by the session manager on log-off
    // Save the shown/hidden status so we can restore to the same state.
    KMainWindow::saveProperties(config);
    config.writeEntry("docked", isHidden());
}

bool KMMainWin::queryClose()
{
    if (kmkernel->shuttingDown() || qApp->isSavingSession() || mReallyClose) {
        return true;
    }
    return kmkernel->canQueryClose();
}

void KMMainWin::slotConfigureShortcuts()
{
    guiFactory()->showConfigureShortcutsDialog();
}

void KMMainWin::slotShortcutSaved()
{
    mKMMainWidget->updateQuickSearchLineText();
}

#include "moc_kmmainwin.cpp"
