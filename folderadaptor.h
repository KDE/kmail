/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2003 Zack Rusin <zack@kde.org>
 *
 *  KMail is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  KMail is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#ifndef FOLDERADAPTOR_H
#define FOLDERADAPTOR_H

#include <QObject>

class KMFolder;

namespace KMail {

  class FolderAdaptor : public QObject
  {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kmail.folder")

  public:
    FolderAdaptor( const QString& vpath );

  public Q_SLOTS:
    Q_SCRIPTABLE QString path() const;
    Q_SCRIPTABLE QString displayName() const;
    Q_SCRIPTABLE QString displayPath() const;
    Q_SCRIPTABLE bool usesCustomIcons() const;
    Q_SCRIPTABLE QString normalIconPath() const;
    Q_SCRIPTABLE QString unreadIconPath() const;
    Q_SCRIPTABLE int messages();
    Q_SCRIPTABLE int unreadMessages();
    Q_SCRIPTABLE int unreadRecursiveMessages();

    //not yet
    //virtual QValueList<DCOPRef> messageRefs();
  protected:
    KMFolder* mFolder;
    QString   mPath;
  };

}

#endif
