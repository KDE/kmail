/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef __KMMAINWIN
#define __KMMAINWIN

#include "kmail_export.h"

#include <kxmlguiwindow.h>

class KMMainWidget;
class KToggleAction;
class QLabel;
namespace KPIM {
class ProgressStatusBarWidget;
}
class KMAIL_EXPORT KMMainWin : public KXmlGuiWindow
{
    Q_OBJECT

public:
    // the main window needs to have a name since else restoring the window
    // settings by kwin doesn't work
    explicit KMMainWin(QWidget *parent = nullptr);
    virtual ~KMMainWin();
    KMMainWidget *mainKMWidget() const;

    /// Same as KMMainWin::restore(), except that it also restores the docked state,
    /// which we have saved in saveProperties().
    /// TODO: KDE5: Move to kdelibs, see http://reviewboard.kde.org/r/504
    bool restoreDockedState(int number);

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
    KPIM::ProgressStatusBarWidget *mProgressBar = nullptr;
    KMMainWidget *mKMMainWidget = nullptr;
    KToggleAction *mHideMenuBarAction = nullptr;
    QLabel *mMessageLabel = nullptr;
    bool mReallyClose = false;
};

#endif
