/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2007 Montel Laurent <montel@kde.org>
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

#ifndef KMAIL_GROUPWARETYPES_H
#define KMAIL_GROUPWARETYPES_H

#include <QMetaType>

#define KMAIL_DBUS_SERVICE "org.kde.kmail"
#define KMAIL_DBUS_GROUPWARE_PATH "/Groupware"
#define KMAIL_DBUS_GROUPWARE_INTERFACE "org.kde.kmail.groupware"

namespace KMail {

  struct SubResource {
    SubResource() {
      writable=false;
      alarmRelevant=false;
    }
    SubResource( const QString& loc, const QString& lab, bool rw, bool ar )
      : location( loc ), label( lab ), writable( rw ), alarmRelevant( ar ) {}
    QString location; // unique
    QString label;    // shown to the user
    bool writable;
    bool alarmRelevant;

    typedef QList<SubResource> List;
  };

  /// The format of the mails containing other contents than actual mail
  /// (like contacts, calendar etc.)
  /// This is currently either ical/vcard, or XML.
  /// For actual mail folders this simply to know which resource handles it
  /// This enum matches the one defined in kmail.kcfg
  enum StorageFormat { StorageIcalVcard, StorageXML };

  /// This bitfield indicates which changes have been made in a folder, at syncing time.
  enum FolderChanges { NoChange = 0, ContentsChanged = 1, ACLChanged = 2 };

  /** Custom header structure, consisting of the header name and value. */
  struct CustomHeader
  {
    CustomHeader() {}
    CustomHeader( const QByteArray &n, const QString &v ) :
        name( n ), value( v ) {}

    QByteArray name;
    QString value;

    typedef QList<CustomHeader> List;
  };

  /**
    Register D-Bus types.
  */
  void registerGroupwareTypes();
}

typedef QMap<quint32, QString> Quint32QStringMap;

Q_DECLARE_METATYPE( KMail::SubResource )
Q_DECLARE_METATYPE( KMail::SubResource::List )
Q_DECLARE_METATYPE( KMail::StorageFormat )
Q_DECLARE_METATYPE( Quint32QStringMap )
Q_DECLARE_METATYPE( KMail::CustomHeader )
Q_DECLARE_METATYPE( KMail::CustomHeader::List )

#endif
