/*  -*- c++ -*-
    callback.cpp

    This file is used by KMail's plugin interface.
    Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#include "callback.h"
#include "kmkernel.h"
#include "kmmessage.h"
#include <libemailfunctions/email.h>
#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>
#include "kmmainwin.h"
#include "composer.h"
#include "kmreaderwin.h"
#include "secondarywindow.h"

#include <mimelib/enum.h>

#include <kinputdialog.h>
#include <klocale.h>
#include <kdebug.h>

using namespace KMail;


Callback::Callback( KMMessage* msg, KMReaderWin* readerWin ) 
  : mMsg( msg ), mReaderWin( readerWin ), mReceiverSet( false )
{
}

bool Callback::mailICal( const QString& to, const QString iCal,
                         const QString& subject ) const
{
  kdDebug(5006) << "Mailing message:\n" << iCal << endl;

  KMMessage *msg = new KMMessage;
  msg->initHeader();
  msg->setHeaderField( "Content-Type",
                       "text/calendar; method=reply; charset=\"utf-8\"" );
  msg->setSubject( subject );
  msg->setTo( to );
  msg->setBody( iCal.utf8() );
  msg->setFrom( receiver() );
  /* We want the triggering mail to be moved to the trash once this one
   * has been sent successfully. Set a link header which accomplishes that. */
  msg->link( mMsg, KMMsgStatusDeleted );

  // Outlook will only understand the reply if the From: header is the
  // same as the To: header of the invitation message.
  KConfigGroup options( KMKernel::config(), "Groupware" );
  if( !options.readBoolEntry( "LegacyMangleFromToHeaders", true ) ) {
    // Try and match the receiver with an identity
    const KPIM::Identity& identity =
      kmkernel->identityManager()->identityForAddress( receiver() );
    if( identity != KPIM::Identity::null() )
      // Identity found. Use this
      msg->setFrom( identity.fullEmailAddr() );
      msg->setHeaderField("X-KMail-Identity", QString::number( identity.uoid() ));
      // Remove BCC from identity on ical invitations (https://intevation.de/roundup/kolab/issue474)
      msg->setBcc( "" );
  }

  KMail::Composer * cWin = KMail::makeComposer();
  cWin->setMsg( msg, false /* mayAutoSign */ );
  // cWin->setCharset( "", true );
  cWin->slotWordWrapToggled( false );
  cWin->setSigningAndEncryptionDisabled( true );

  if ( options.readBoolEntry( "AutomaticSending", true ) ) {
    cWin->setAutoDeleteWindow(  true );
    cWin->slotSendNow();
  } else {
    cWin->show();
  }

  return true;
}

QString Callback::receiver() const
{
  if ( mReceiverSet )
    // Already figured this out
    return mReceiver;

  mReceiverSet = true;

  QStringList addrs = KPIM::splitEmailAddrList( mMsg->to() );
  int found = 0;
  for( QStringList::Iterator it = addrs.begin(); it != addrs.end(); ++it ) {
    if( kmkernel->identityManager()->identityForAddress( *it ) !=
        KPIM::Identity::null() ) {
      // Ok, this could be us
      ++found;
      mReceiver = *it;
    }
  }
  QStringList ccaddrs = KPIM::splitEmailAddrList( mMsg->cc() );
  for( QStringList::Iterator it = ccaddrs.begin(); it != ccaddrs.end(); ++it ) {
    if( kmkernel->identityManager()->identityForAddress( *it ) !=
        KPIM::Identity::null() ) {
      // Ok, this could be us
      ++found;
      mReceiver = *it;
    }
  }
  if( found != 1 ) {
    bool ok;
    QString selectMessage;
    if (found == 0) {
      selectMessage = i18n("<qt>None of your identities match the "
          "receiver of this message,<br>please "
          "choose which of the following addresses "
          "is yours, if any:");
    } else {
      selectMessage = i18n("<qt>Several of your identities match the "
          "receiver of this message,<br>please "
          "choose which of the following addresses "
          "is yours:");
    }

    mReceiver =
      KInputDialog::getItem( i18n( "Select Address" ),
          selectMessage,
          addrs, 0, FALSE, &ok, kmkernel->mainWin() );
    if( !ok )
      mReceiver = QString::null;
  }

  return mReceiver;
}

void Callback::closeIfSecondaryWindow() const
{
  KMail::SecondaryWindow *window = dynamic_cast<KMail::SecondaryWindow*>( mReaderWin->mainWindow() );
  if ( window )
    window->close();
}
