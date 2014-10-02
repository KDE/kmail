/* -*- mode: C++; c-file-style: "gnu" -*-
    secondarywindow.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2003 Ingo Kloecker <kloecker@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/


#include "secondarywindow.h"
#include "kmkernel.h"

#include <QApplication>
#include <KLocalizedString>

#include <QCloseEvent>

namespace KMail {

//---------------------------------------------------------------------------
SecondaryWindow::SecondaryWindow( const char * name )
    : KXmlGuiWindow( 0 )
{
    setObjectName( QLatin1String(name) );
    // Set this to be the group leader for all subdialogs - this means
    // modal subdialogs will only affect this window, not the other windows
    setAttribute( Qt::WA_GroupLeader );
}


//---------------------------------------------------------------------------
SecondaryWindow::~SecondaryWindow()
{
}


//---------------------------------------------------------------------------
void SecondaryWindow::closeEvent( QCloseEvent * e )
{
    // if there's a system tray applet then just do what needs to be done if a
    // window is closed.
    if ( kmkernel->haveSystemTrayApplet() ) {
        // BEGIN of code borrowed from KMainWindow::closeEvent
        // Save settings if auto-save is enabled, and settings have changed
        if ( settingsDirty() && autoSaveSettings() )
            saveAutoSaveSettings();

        if ( !queryClose() ) {
            e->ignore();
        }
        // END of code borrowed from KMainWindow::closeEvent
    } else {
        KMainWindow::closeEvent( e );
    }
}

void SecondaryWindow::setCaption( const QString &userCaption )
{
    QString caption = QGuiApplication::applicationDisplayName();
    QString captionString = userCaption.isEmpty() ? caption : userCaption;
    if ( !userCaption.isEmpty() ) {
        // Add the application name if:
        // User asked for it, it's not a duplication  and the app name (caption()) is not empty
        if ( !caption.isEmpty() ) {
            captionString += i18nc("Document/application separator in titlebar", " – ") + caption;
        }
    }

    setWindowTitle(captionString);
}

} // namespace KMail

