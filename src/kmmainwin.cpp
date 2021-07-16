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
#include "util.h"
#include <Libkdepim/ProgressStatusBarWidget>
#include <Libkdepim/StatusbarProgressWidget>
#include <PimCommon/BroadcastStatus>
#include <kxmlgui_version.h>

#include <KConfigGroup>
#include <KToolBar>
#include <QApplication>
#include <QStatusBar>
#include <QTimer>

#include "kmail_debug.h"
#include <KConfig>
#include <KConfigGui>
#include <KEditToolBar>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>
#include <KXMLGUIFactory>
#include <QMenuBar>
#include <ktip.h>

#include <QLabel>
#include <chrono>

using namespace std::chrono_literals;

KMMainWin::KMMainWin(QWidget *)
    : KXmlGuiWindow(nullptr)
{
    setObjectName(QStringLiteral("kmail-mainwindow#"));
    // Set this to be the group leader for all subdialogs - this means
    // modal subdialogs will only affect this dialog, not the other windows
    setAttribute(Qt::WA_GroupLeader);

    resize(700, 500); // The default size

    mKMMainWidget = new KMMainWidget(this, this, actionCollection());
    connect(mKMMainWidget, &KMMainWidget::recreateGui, this, &KMMainWin::slotUpdateGui);
    setCentralWidget(mKMMainWidget);
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
    setAutoSaveSettings(KMKernel::self()->config()->group("Main Window"));

    connect(PimCommon::BroadcastStatus::instance(), &PimCommon::BroadcastStatus::statusMsg, this, &KMMainWin::displayStatusMessage);

    connect(mKMMainWidget, &KMMainWidget::captionChangeRequest, this, qOverload<const QString &>(&KMainWindow::setCaption));

    mKMMainWidget->updateQuickSearchLineText();
    mShowMenuBarAction->setChecked(KMailSettings::self()->showMenuBar());
    slotToggleMenubar(true);
#if KXMLGUI_VERSION >= QT_VERSION_CHECK(5, 84, 0)
    connect(guiFactory(), &KXMLGUIFactory::shortcutsSaved, this, &KMMainWin::slotShortcutSaved);
#endif
}

KMMainWin::~KMMainWin()
{
    // Avoids a crash if there are any Akonadi jobs running, which may
    // attempt to display a status message when they are killed.
    disconnect(PimCommon::BroadcastStatus::instance(), &PimCommon::BroadcastStatus::statusMsg, this, nullptr);
}

void KMMainWin::updateHamburgerMenu()
{
    QMenu *menu = new QMenu(this);
    menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::Open))));
    menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::SaveAs))));
    menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::Print))));
    menu->addSeparator();
    menu->addAction(actionCollection()->action(QStringLiteral("check_mail")));
    menu->addAction(actionCollection()->action(QStringLiteral("check_mail_in")));
    menu->addAction(actionCollection()->action(QStringLiteral("send_queued")));
    menu->addAction(actionCollection()->action(QStringLiteral("send_queued_via")));
    menu->addSeparator();

    menu->addAction(actionCollection()->action(QStringLiteral("kmail_configure_kmail")));
    menu->addAction(actionCollection()->action(QStringLiteral("kmail_configure_notifications")));
    menu->addSeparator();

    menu->addAction(actionCollection()->action(QLatin1String(KStandardAction::name(KStandardAction::Quit))));
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
                                         i18n("Hide menu bar"),
                                         QStringLiteral("HideMenuBarWarning"));
            }
            menuBar()->hide();
        }
        KMailSettings::self()->setShowMenuBar(mShowMenuBarAction->isChecked());
    }
}

void KMMainWin::slotEditToolbars()
{
    KConfigGroup grp = KMKernel::self()->config()->group("Main Window");
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
    applyMainWindowSettings(KMKernel::self()->config()->group("Main Window"));

    // plug dynamically created actions again
    mKMMainWidget->initializeFilterActions(false);
    mKMMainWidget->tagActionManager()->createActions();
    // FIXME mKMMainWidget->initializePluginActions();
}

void KMMainWin::setupStatusBar()
{
    /* Create a progress dialog and hide it. */
    mProgressBar = new KPIM::ProgressStatusBarWidget(statusBar(), this);
    mMessageLabel = new QLabel(i18n("Starting..."));
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
    bool show = !config.readEntry("docked", false);

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
#if KXMLGUI_VERSION < QT_VERSION_CHECK(5, 84, 0)
    if (guiFactory()->configureShortcuts()) {
        mKMMainWidget->updateQuickSearchLineText();
    }
#else
    guiFactory()->showConfigureShortcutsDialog();
#endif
}

void KMMainWin::slotShortcutSaved()
{
    mKMMainWidget->updateQuickSearchLineText();
}
