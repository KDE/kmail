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

#ifndef GROUPWAREADAPTOR_H
#define GROUPWAREADAPTOR_H

#include <QObject>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusArgument>
#include <QByteArray>
#include <QString>
#include <QMap>
#include <qdbusmetatype.h>
#include <QtDBus/qdbusextratypes.h>
#include <QtDBus/QtDBus>
class KMailICalIfaceImpl;

typedef QMap<QByteArray, QString> ByteArrayStringMap;
Q_DECLARE_METATYPE(ByteArrayStringMap)
namespace KMail {

class GroupwareAdaptor : public QDBusAbstractAdaptor
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.kmail.groupware")
public:
  GroupwareAdaptor(KMailICalIfaceImpl* impl);
  //qDBusRegisterMetaType< QMap<QByteArray, QString> >();

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
  };

  /// The format of the mails containing other contents than actual mail
  /// (like contacts, calendar etc.)
  /// This is currently either ical/vcard, or XML.
  /// For actual mail folders this simply to know which resource handles it
  /// This enum matches the one defined in kmail.kcfg
  enum StorageFormat { StorageIcalVcard, StorageXML };

  /// This bitfield indicates which changes have been made in a folder, at syncing time.
  enum FolderChanges { NoChange = 0, Contents = 1, ACL = 2 };

public Q_SLOTS:
  Q_SCRIPTABLE bool isWritableFolder( const QString& type, const QString& resource );
  // Return the number of mails that need to be looked at by incidencesKolab.
  // This allows to call incidencesKolab in chunks.
  Q_SCRIPTABLE int incidencesKolabCount( const QString& mimetype /*ignored*/,
                                         const QString& resource );
  /**
   * Causes all resource folders of the given type to be synced with the server.
   */
  Q_SCRIPTABLE bool triggerSync( const QString & );

  Q_SCRIPTABLE bool deleteIncidenceKolab( const QString& resource, quint32 sernum );
  Q_SCRIPTABLE GroupwareAdaptor::StorageFormat storageFormat( const QString& resource );

  Q_SCRIPTABLE QString getAttachment( const QString& resource, quint32 sernum, const QString& filename );
  Q_SCRIPTABLE quint32 update( const QString& resource,
                               quint32 sernum,
                               const QString& subject,
                               const QString& plainTextBody,
                               const QMap<QByteArray, QString>& customHeaders,
                               const QStringList& attachmentURLs,
                               const QStringList& attachmentMimetypes,
                               const QStringList& attachmentNames,
                               const QStringList& deletedAttachments );
  Q_SCRIPTABLE QMap<quint32, QString> incidencesKolab( const QString& mimetype,
                                                       const QString& resource,
                                                       int startIndex,
                                                       int nbMessages );
  /**
   * Return list of subresources. @p contentsType is
   * Mail, Calendar, Contact, Note, Task or Journal
   */
  Q_SCRIPTABLE QList<GroupwareAdaptor::SubResource> subresourcesKolab( const QString& contentsType );


Q_SIGNALS:
  Q_SCRIPTABLE  void incidenceAdded( const QString& type, const QString& folder,
                       quint32 sernum, int format, const QString& entry );
  Q_SCRIPTABLE void asyncLoadResult( const QMap<quint32, QString>, const QString& type,
                        const QString& folder );
  Q_SCRIPTABLE void incidenceDeleted( const QString& type, const QString& folder,
                         const QString& uid );
  Q_SCRIPTABLE void signalRefresh( const QString& type, const QString& folder );
  Q_SCRIPTABLE void subresourceAdded( const QString& type, const QString& resource,
                         const QString& label, bool writable, bool alarmRelevant );
  Q_SCRIPTABLE void subresourceDeleted( const QString& type, const QString& resource );

protected:
  KMailICalIfaceImpl *mIcalImpl;

};
}// namespace KMail
#endif  /*groupwareadaptor*/
