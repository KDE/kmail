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
#include "editor/composer.h"
#include "kmkernel.h"

// kdepim includes
#include "messagecomposer/helper/messagehelper.h"

#include <kurl.h>
#include <kdebug.h>

#include <QDBusConnection>

namespace KMail {


MailServiceImpl::MailServiceImpl()
{
  new ServiceAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String("/MailTransportService"), this );
}

bool MailServiceImpl::sendMessage( const QString& from, const QString& to,
                                   const QString& cc, const QString& bcc,
                                   const QString& subject, const QString& body,
                                   const QStringList& attachments )
{
  if ( to.isEmpty() && cc.isEmpty() && bcc.isEmpty() )
    return false;

  KMime::Message::Ptr msg( new KMime::Message );
  MessageHelper::initHeader( msg, KMKernel::self()->identityManager() );

  msg->contentType()->setCharset( "utf-8" );

  if ( !from.isEmpty() )    msg->from()->fromUnicodeString( from, "utf-8" );
  if ( !to.isEmpty() )      msg->to()->fromUnicodeString( to, "utf-8" );
  if ( !cc.isEmpty() )      msg->cc()->fromUnicodeString( cc, "utf-8" );
  if ( !bcc.isEmpty() )     msg->bcc()->fromUnicodeString( bcc, "utf-8" );
  if ( !subject.isEmpty() ) msg->subject()->fromUnicodeString( subject, "utf-8" );
  if ( !body.isEmpty() )    msg->setBody( body.toUtf8() );

  KMail::Composer * cWin = KMail::makeComposer( msg );

  KUrl::List attachUrls;
  const int nbAttachments = attachments.count();
  for ( int i = 0; i < nbAttachments; ++i ) {
    attachUrls += KUrl( attachments[i] );
  }

  cWin->addAttachmentsAndSend( attachUrls, QString(), 1 );//send now
  return true;
}


bool MailServiceImpl::sendMessage( const QString& from, const QString& to,
                                   const QString& cc, const QString& bcc,
                                   const QString& subject, const QString& body,
                                   const QByteArray& attachment )
{
  if ( to.isEmpty() && cc.isEmpty() && bcc.isEmpty() )
    return false;

  KMime::Message::Ptr msg( new KMime::Message );
  MessageHelper::initHeader( msg, KMKernel::self()->identityManager() );

  msg->contentType()->setCharset( "utf-8" );

  if ( !from.isEmpty() )    msg->from()->fromUnicodeString( from, "utf-8" );
  if ( !to.isEmpty() )      msg->to()->fromUnicodeString( to, "utf-8" );
  if ( !cc.isEmpty() )      msg->cc()->fromUnicodeString( cc, "utf-8" );
  if ( !bcc.isEmpty() )     msg->bcc()->fromUnicodeString( bcc, "utf-8" );
  if ( !subject.isEmpty() ) msg->subject()->fromUnicodeString( subject, "utf-8" );
  if ( !body.isEmpty() )    msg->setBody( body.toUtf8() );

  KMime::Content *part = new KMime::Content;
  part->contentTransferEncoding()->from7BitString( "base64" );
  part->setBody( attachment ); //TODO: check it!
  msg->addContent( part );

  KMail::makeComposer( msg, false, false );
  return true;
}



}//end namespace KMail

