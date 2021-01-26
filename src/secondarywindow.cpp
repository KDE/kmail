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
    // Set this to be the group leader for all subdialogs - this means
    // modal subdialogs will only affect this window, not the other windows
    setAttribute(Qt::WA_GroupLeader);
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
        // Save settings if auto-save is enabled, and settings have changed
        if (settingsDirty() && autoSaveSettings()) {
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
