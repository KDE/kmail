/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2009 Philip Van Hoof <philip@codeminded.be>
 *
 *  KMail is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License,
 *  or (at your option) any later version.
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
#include "mailmanagerimpl.h"
#include <manageradaptor.h>

#include "kmmessage.h"
#include "kmfolder.h"

#include <KUrl>
#include <KDebug>

#include <QString>
#include <QDBusConnection>

static const uint max_per_dbus_call = 1000;

namespace KMail {


MailManagerImpl::MailManagerImpl()
{
  new ManagerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( "/org/freedesktop/email/metadata/Manager", this );
}

void MailManagerImpl::processMsgBase( const KMMsgBase *msg, QStringList &subjects, QVector<QStringList> &predicatesArray, QVector<QStringList> &valuesArray )
{
  QStringList values;
  QStringList predicates;

  predicates.append( "EMailMeta:MessageSubject" );
  values.append( msg->subject() );

  predicates.append( "EMailMeta:MessageFrom" );
  values.append( msg->fromStrip () );

  predicates.append( "EMailMeta:MessageTo" );
  values.append( msg->toStrip () );

  predicates.append( "KMail:MessageIdMD5" );
  values.append( msg->msgIdMD5() );

  predicates.append( "KMail:MessageSerNum" );
  values.append( QString::number( msg->getMsgSerNum() ) );

  predicates.append( "EMailMeta:MessageSent" );
  values.append( msg->dateStr () );

  predicates.append( "EMailMeta:MessageSent" );
  values.append( msg->dateStr () );

  predicates.append( "EMailMeta:MessageSize" );
  values.append( QString::number( static_cast<uint> ( msg->msgSizeServer() ) ) );

  predicates.append( "KMail:MessageUID" );
  values.append( QString::number(static_cast<uint> ( msg->UID() ) ) );

  for ( KMMessageTagList::const_iterator it = msg->tagList()->constBegin(); it != msg->tagList()->constEnd(); ++it ) {
    QString tag = (*it);
    predicates.append( "KMail:MessageTag" );
    values.append( tag );
  }

  const MessageStatus &stat = msg->messageStatus();

  predicates.append( "EMailMeta:MessageSeen" );
  if (stat.isRead())
    values.append( "True" );
  else
    values.append( "False" );

  predicates.append( "KMail:MessageSpam" );
  if ( stat.isSpam() )
    values.append( "True" );
  else
    values.append( "False" );

  predicates.append( "KMail:MessageHam" );
  if ( stat.isHam() )
    values.append( "True" );
  else
    values.append( "False" );

  predicates.append( "EMailMeta:MessageAnswered" );
  if ( stat.isReplied() )
    values.append( "True" );
  else
    values.append( "False" );

  predicates.append( "EMailMeta:MessageForwarded" );
  if ( stat.isForwarded() )
    values.append( "True" );
  else
    values.append( "False" );

  subjects.append( "kmail://" + QString::number( msg->getMsgSerNum() ) );
  predicatesArray.append( predicates );
  valuesArray.append( values );
}

void MailManagerImpl::Register( const QDBusObjectPath &registrarPath, uint lastModseq )
{
  Q_UNUSED( lastModseq );
  registrars.append( registrarPath );

  QDBusMessage m = QDBusMessage::createMethodCall( message().service(),
                     registrarPath.path(),
                     "org.freedesktop.email.metadata.Registrar", "Cleanup" );
  QList<QVariant> args;
  args.append ( static_cast<uint> ( time( 0 ) ) );
  m.setArguments( args );
  QDBusConnection::sessionBus().send( m );

  QList< QPointer< KMFolder > > folders = kmkernel->allFolders();
  QList< QPointer< KMFolder > >::Iterator it = folders.begin();

  while ( it != folders.end() ) {
    uint i = 0;

    KMFolder *folder = (*it);
    ++it;

    folder->open("DBus");

    uint fcount = folder->count();

    while ( i < fcount ) {

      QVector<QStringList> predicatesArray;
      QVector<QStringList> valuesArray;
      QStringList subjects;

      uint processed = 0;

      for ( ; i < fcount; ++i ) {
        const KMMsgBase *msg = folder->getMsgBase( i );

        processMsgBase( msg, subjects, predicatesArray, valuesArray );
        processed++;

        if ( processed >= max_per_dbus_call )
          break;
      }

      if (!subjects.isEmpty()) {
        QDBusMessage m = QDBusMessage::createMethodCall( message().service(),
                                registrarPath.path(),
                                "org.freedesktop.email.metadata.Registrar", "SetMany" );
        QList<QVariant> args;

        args.append( subjects );
        args.append( qVariantFromValue( predicatesArray ) );
        args.append( qVariantFromValue( valuesArray ) );
        args.append( static_cast<uint> ( time( 0 ) ) );

        m.setArguments( args );

        QDBusConnection::sessionBus().send( m );
      }
    }

    folder->close( "DBus", false );
  }
}


}//end namespace KMail

