/*  -*- mode: C++; c-file-style: "gnu" -*-
    secondarywindow.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2003 Ingo Kloecker <kloecker@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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
#ifndef __KMAIL_SECONDARYWINDOW_H__
#define __KMAIL_SECONDARYWINDOW_H__

#include <kmainwindow.h>

class QCloseEvent;

namespace KMail {

  /**
   *  Window class for secondary KMail window like the composer window and
   *  the separate message window.
   */
  class SecondaryWindow : public KMainWindow
  {
    Q_OBJECT

  public:
    SecondaryWindow( const char * name = 0 );
    ~SecondaryWindow();

  protected:
    /**
     *  Reimplemented because we don't want the application to quit when the
     *  last _visible_ secondary window is closed in case a system tray applet
     *  exists.
     */
    virtual void closeEvent( QCloseEvent * );
  };

} // namespace KMail

#endif // __KMAIL_SECONDARYWINDOW_H__
