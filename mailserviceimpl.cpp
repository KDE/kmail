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

#include "mailserviceimpl.h"
#include "mailserviceimpl.moc"
#include <serviceadaptor.h>
#include "composer.h"

#include "kmmessage.h"
#include "kmmsgpart.h"

#include <kurl.h>
#include <kdebug.h>
#include <QString>
#include <QDBusConnection>

namespace KMail {


MailServiceImpl::MailServiceImpl()
{
  new ServiceAdaptor( this );
  QDBusConnection::sessionBus().registerObject( "/MailTransportService", this );
}

bool MailServiceImpl::sendMessage( const QString& from, const QString& to,
                                   const QString& cc, const QString& bcc,
                                   const QString& subject, const QString& body,
                                   const QStringList& attachments )
{
  if ( to.isEmpty() && cc.isEmpty() && bcc.isEmpty() )
    return false;

  KMMessage *msg = new KMMessage;
  msg->initHeader();

  msg->setCharset( "utf-8" );

  if ( !from.isEmpty() )    msg->setFrom( from );
  if ( !to.isEmpty() )      msg->setTo( to );
  if ( !cc.isEmpty() )      msg->setCc( cc );
  if ( !bcc.isEmpty() )     msg->setBcc( bcc );
  if ( !subject.isEmpty() ) msg->setSubject( subject );
  if ( !body.isEmpty() )    msg->setBody( body.toUtf8() );

  KMail::Composer * cWin = KMail::makeComposer( msg );
  cWin->setCharset("", true);

  KUrl::List attachUrls;
  for ( int i = 0, count = attachments.count(); i < count; ++i ) {
    attachUrls += KUrl( attachments[i] );
  }

  cWin->addAttachmentsAndSend( attachUrls, "", 1 );//send now
  return true;
}


bool MailServiceImpl::sendMessage( const QString& from, const QString& to,
                                   const QString& cc, const QString& bcc,
                                   const QString& subject, const QString& body,
                                   const QByteArray& attachment )
{
  if ( to.isEmpty() && cc.isEmpty() && bcc.isEmpty() )
    return false;

  KMMessage *msg = new KMMessage;
  msg->initHeader();

  msg->setCharset( "utf-8" );

  if ( !from.isEmpty() )    msg->setFrom( from );
  if ( !to.isEmpty() )      msg->setTo( to );
  if ( !cc.isEmpty() )      msg->setCc( cc );
  if ( !bcc.isEmpty() )     msg->setBcc( bcc );
  if ( !subject.isEmpty() ) msg->setSubject( subject );
  if ( !body.isEmpty() )    msg->setBody( body.toUtf8() );

  KMMessagePart *part = new KMMessagePart;
  part->setCteStr( "base64" );
  part->setBodyEncodedBinary( attachment );
  msg->addBodyPart( part );

  KMail::Composer * cWin = KMail::makeComposer( msg );
  cWin->setCharset("", true);
  return true;
}



}//end namespace KMail

