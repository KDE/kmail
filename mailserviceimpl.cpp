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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mailserviceimpl.h"

#include "kmcomposewin.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmkernel.h"

#include <dcopobject.h>
#include <kurl.h>
#include <qcstring.h>

namespace KMail {


MailServiceImpl::MailServiceImpl()
  : DCOPObject( "MailTransportServiceIface" )
{
}

bool MailServiceImpl::sendMessage( const QString& to, const QString& cc, const QString& bcc,
                                   const QString& subject, const QString& body,
                                   const KURL::List& attachments )
{
  if ( to.isEmpty() && cc.isEmpty() && bcc.isEmpty() )
    return false;

  KMMessage *msg = new KMMessage;
  msg->initHeader();

  msg->setCharset( "utf-8" );

  if ( !to.isEmpty() )      msg->setTo( to );
  if ( !cc.isEmpty() )      msg->setCc( cc );
  if ( !bcc.isEmpty() )     msg->setBcc( bcc );
  if ( !subject.isEmpty() ) msg->setSubject( subject );
  if ( !body.isEmpty() )    msg->setBody( body.utf8() );

  KMComposeWin *cWin = new KMComposeWin( msg );
  cWin->setCharset("", TRUE);

  for( KURL::List::ConstIterator itr = attachments.begin();
       itr != attachments.end(); ++itr ) {
    cWin->addAttachment( *itr, "" );
  }

  cWin->send( 1 );//send now
  return true;
}

bool MailServiceImpl::sendMessage( const QString& to, const QString& cc, const QString& bcc,
                                   const QString& subject, const QString& body,
                                   const QByteArray& attachment )
{
  if ( to.isEmpty() && cc.isEmpty() && bcc.isEmpty() )
    return false;

  KMMessage *msg = new KMMessage;
  msg->initHeader();

  msg->setCharset( "utf-8" );

  if ( !to.isEmpty() )      msg->setTo( to );
  if ( !cc.isEmpty() )      msg->setCc( cc );
  if ( !bcc.isEmpty() )     msg->setBcc( bcc );
  if ( !subject.isEmpty() ) msg->setSubject( subject );
  if ( !body.isEmpty() )    msg->setBody( body.utf8() );

  KMMessagePart *part = new KMMessagePart;
  part->setCteStr( "base64" );
  part->setBodyEncodedBinary( attachment );
  msg->addBodyPart( part );

  KMComposeWin *cWin = new KMComposeWin( msg );
  cWin->setCharset("", TRUE);
  return true;
}

}//end namespace KMail

