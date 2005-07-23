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

#ifndef FOLDERIFACE_H
#define FOLDERIFACE_H

#include <qobject.h>
#include <dcopobject.h>

class KMFolder;
class KMMainWidget;

namespace KMail {

  class FolderIface : public QObject,
                      public DCOPObject
  {
    Q_OBJECT
    K_DCOP

  public:
    FolderIface( const QString& vpath );

  k_dcop:
    virtual QString path() const;
    virtual QString displayName() const;
    virtual QString displayPath() const;
    virtual bool usesCustomIcons() const;
    virtual QString normalIconPath() const;
    virtual QString unreadIconPath() const;
    virtual int messages();
    virtual int unreadMessages();
    virtual int unreadRecursiveMessages();

    //not yet
    //virtual QValueList<DCOPRef> messageRefs();
  protected:
    KMFolder* mFolder;
    QString   mPath;
  };

}

#endif
