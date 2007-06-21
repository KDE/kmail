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


#include "groupwareadaptor.h"
#include "kmailicalifaceimpl.h"
#include "kmfolder.h"
#include <qdbusconnection.h>
using namespace KMail;

typedef QList<SubResource> QListKmailSubResource;
Q_DECLARE_METATYPE(SubResource )
Q_DECLARE_METATYPE(QListKmailSubResource )
Q_DECLARE_METATYPE(StorageFormat )

const QDBusArgument &operator<<(QDBusArgument &arg, const SubResource &subResource)
{
    arg.beginStructure();
    arg << subResource.location << subResource.label << subResource.writable << subResource.alarmRelevant;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, SubResource &subResource)
{
    arg.beginStructure();
    arg >> subResource.location >> subResource.label >> subResource.writable >> subResource.alarmRelevant;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator<<(QDBusArgument &arg, const StorageFormat &format)
{
    arg.beginStructure();
    quint32 foo = format;
    arg << foo;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, StorageFormat &format)
{
    arg.beginStructure();
    quint32 foo = format;
    arg >> foo;
    arg.endStructure();
    return arg;
}


typedef QMap<quint32, QString> Quint32StringMap;
Q_DECLARE_METATYPE(Quint32StringMap)


static void registerTypes()
{
    static bool registered = false;
    if (!registered) {
        qDBusRegisterMetaType<QListKmailSubResource>();
        qDBusRegisterMetaType<Quint32StringMap>();
        registered = true;
    }
}

GroupwareAdaptor::GroupwareAdaptor(KMailICalIfaceImpl* impl)
  :QDBusAbstractAdaptor( impl ), mIcalImpl( impl )
{
  setAutoRelaySignals(true);
  QDBusConnection::sessionBus().registerObject( "/GroupWare", this,QDBusConnection::ExportScriptableSlots|QDBusConnection::ExportScriptableSignals );
}


bool GroupwareAdaptor::isWritableFolder( const QString& type, const QString& resource )
{
  return mIcalImpl->isWritableFolder( type, resource );
}

int GroupwareAdaptor::incidencesKolabCount( const QString& mimetype /*ignored*/, const QString& resource )
{
  return mIcalImpl->incidencesKolabCount( mimetype, resource );
}

bool GroupwareAdaptor::triggerSync( const QString & str)
{
  return mIcalImpl->triggerSync( str );
}

bool GroupwareAdaptor::deleteIncidenceKolab( const QString& resource, quint32 sernum )
{
  return mIcalImpl->deleteIncidenceKolab( resource, sernum );
}

StorageFormat GroupwareAdaptor::storageFormat( const QString& resource )
{
  return mIcalImpl->storageFormat(resource );
}

QString GroupwareAdaptor::getAttachment( const QString& resource, quint32 sernum, const QString& filename )
{
  KUrl url = mIcalImpl->getAttachment( resource, sernum, filename );
  return url.path();
}

quint32 GroupwareAdaptor::update( const QString& resource,
                                  quint32 sernum,
                                  const QString& subject,
                                  const QString& plainTextBody,
                                  const QMap<QByteArray, QString>& customHeaders,
                                  const QStringList& attachmentURLs,
                                  const QStringList& attachmentMimetypes,
                                  const QStringList& attachmentNames,
                                  const QStringList& deletedAttachments )
{
  //qDBusRegisterMetaType< QMap<QByteArray, QString> >();
  return mIcalImpl->update( resource, sernum, subject, plainTextBody, customHeaders, attachmentURLs, attachmentMimetypes, attachmentNames, deletedAttachments );
}

QMap<quint32, QString> GroupwareAdaptor::incidencesKolab( const QString& mimetype,
                                                          const QString& resource,
                                                          int startIndex,
                                                          int nbMessages )
{
  registerTypes();
  return mIcalImpl->incidencesKolab( mimetype, resource, startIndex, nbMessages );
}

QList<SubResource> GroupwareAdaptor::subresourcesKolab( const QString& contentsType )
{
  registerTypes();
  return mIcalImpl->subresourcesKolab(contentsType );
}

#include "groupwareadaptor.moc"
