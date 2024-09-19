/*
    secondarywindow.cpp

    This file is part of KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2003 Ingo Kloecker <kloecker@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#include "secondarywindow.h"
#include "kmkernel.h"

#include <KLocalizedString>

#include <QCloseEvent>

using namespace KMail;
//---------------------------------------------------------------------------
SecondaryWindow::SecondaryWindow(const QString &name)
    : KXmlGuiWindow(nullptr)
{
    setObjectName(name);
}

//---------------------------------------------------------------------------
SecondaryWindow::~SecondaryWindow() = default;

//---------------------------------------------------------------------------
void SecondaryWindow::closeEvent(QCloseEvent *e)
{
    // if there's a system tray applet then just do what needs to be done if a
    // window is closed.
    if (kmkernel->haveSystemTrayApplet()) {
        // BEGIN of code borrowed from KMainWindow::closeEvent

        // Clear the flag that tells KMainWindow to not restore the window
        // position for the next window to be opened, see KMainWindow::closeEvent()
        // and KMainWindow::applyMainWindowSettings().  We are not going to pass
        // the event on to KMainWindow so this needs to be done here.
        KConfigGroup grp = stateConfigGroup();
        if (grp.isValid()) {
            grp.deleteEntry("RestorePositionForNextInstance");
        }

        // Save settings if auto-save is enabled.  We want to save the window
        // position as well as the size, so save the settings regardless of
        // whether they have changed.  Otherwise, the applyMainWindowSettings()
        // done when opening the window will have set the state to "clean" and
        // the window position will not be saved even if the window has been
        // moved.  On Unix platforms the state is set to "dirty" if the window
        // is resized but not if it is simply moved, see KMainWindow::event().
        //
        // saveAutoSaveSettings() saves the settings to the autoSaveGroup().
        // This will have been set at construction time to be the same as
        // the stateConfigGroup();  the settings must be saved to that group
        // because applyMainWindowSettings() always restores from there.
        if (autoSaveSettings()) {
            saveAutoSaveSettings();
        }

        if (!queryClose()) {
            e->ignore();
        }
        // END of code borrowed from KMainWindow::closeEvent
    } else {
        KMainWindow::closeEvent(e);
    }
}

void SecondaryWindow::setCaption(const QString &userCaption)
{
    const QString caption = QGuiApplication::applicationDisplayName();
    QString captionString = userCaption.isEmpty() ? caption : userCaption;
    if (!userCaption.isEmpty()) {
        // Add the application name if:
        // User asked for it, it's not a duplication  and the app name (caption()) is not empty
        if (!caption.isEmpty()) {
            captionString += i18nc("Document/application separator in titlebar", " – ") + caption;
        }
    }

    setWindowTitle(captionString);
}

#include "moc_secondarywindow.cpp"
