/*
 * kmail: KDE mail client
 * SPDX-FileCopyrightText: 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#pragma once

#include "kmail_export.h"

#include <kxmlguiwindow.h>

class KMMainWidget;
class KToggleAction;
class QLabel;
class KHamburgerMenu;
namespace KPIM
{
class ProgressStatusBarWidget;
}
class KMAIL_EXPORT KMMainWin : public KXmlGuiWindow
{
    Q_OBJECT

public:
    // the main window needs to have a name since else restoring the window
    // settings by kwin doesn't work
    explicit KMMainWin(QWidget *parent = nullptr);
    ~KMMainWin() override;
    KMMainWidget *mainKMWidget() const;

    /// Same as KMMainWin::restore(), except that it also restores the docked state,
    /// which we have saved in saveProperties().
    /// TODO: KDE5: Move to kdelibs, see http://reviewboard.kde.org/r/504
    Q_REQUIRED_RESULT bool restoreDockedState(int number);

    void showAndActivateWindow();
public Q_SLOTS:
    void displayStatusMessage(const QString &);
    void slotEditToolbars();
    void slotUpdateGui();
    void setupStatusBar();

protected:
    /// Reimplemented to save the docked state
    void saveProperties(KConfigGroup &) override;

    bool queryClose() override;

protected Q_SLOTS:
    void slotQuit();

private:
    void slotConfigureShortcuts();
    void slotToggleMenubar(bool dontShowWarning);
    void updateHamburgerMenu();
    void slotShortcutSaved();
    KPIM::ProgressStatusBarWidget *mProgressBar = nullptr;
    KMMainWidget *mKMMainWidget = nullptr;
    KToggleAction *mShowMenuBarAction = nullptr;
    QLabel *mMessageLabel = nullptr;
    KHamburgerMenu *mHamburgerMenu = nullptr;
    bool mReallyClose = false;
};

