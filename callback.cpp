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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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
#include <libkdepim/email.h>
#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>
#include "kmmainwin.h"
#include "kmcomposewin.h"

#include <mimelib/enum.h>

#include <kinputdialog.h>
#include <klocale.h>
#include <kdebug.h>

using namespace KMail;


Callback::Callback( KMMessage* msg ) : mMsg( msg ),  mReceiverSet( false )
{
}

bool Callback::mailICal( const QString& to, const QString iCal,
                         const QString& subject ) const
{
  kdDebug(5006) << "Mailing message:\n" << iCal << endl;

  if ( receiver().isEmpty() )
    // This can't work
    return false;

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
    if( identity != KPIM::Identity::null ) {
      // Identity found. Use this
      msg->setFrom( identity.fullEmailAddr() );
      msg->setHeaderField( "X-KMail-Identity",
                           QString::number( identity.uoid() ) );

      // Remove BCC from all replies on ical invitations
      msg->setBcc( "" );
    }
  }

  KMComposeWin *cWin = new KMComposeWin();
  cWin->setMsg( msg, false /* mayAutoSign */ );
  // cWin->setCharset( "", true );
  cWin->slotWordWrapToggled( false );
  cWin->setSigningAndEncryptionDisabled( true );

  if ( options.readBoolEntry( "AutomaticSending", true ) ) {
    cWin->setAutoDeleteWindow( true );
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

  // Don't ask again
  mReceiverSet = true;

  QStringList addrs = KPIM::splitEmailAddrList( mMsg->to() );
  int found = 0;
  for( QStringList::Iterator it = addrs.begin(); it != addrs.end(); ++it )
    if( kmkernel->identityManager()->identityForAddress( *it )
        != KPIM::Identity::null ) {
      // Ok, this could be us
      ++found;
      mReceiver = *it;
    }

  if( found != 1 ) {
    bool ok;
    const QString message =
      i18n( "<qt>If you received this personally, please choose which of<br>"
            "the following addresses is yours.<br><br>"
            "If you received this from a distribution list, then choose<br>"
            "which of your own identities to reply with." );

    // Make the list of addresses
    QStringList toShow = addrs;

    // Remember how many original receivers we had
    const int receivers = addrs.count();

    // Add identities to the list
    KPIM::IdentityManager::ConstIterator it =
      kmkernel->identityManager()->begin();
    for ( ; it != kmkernel->identityManager()->end(); ++it ) {
      const QString addr = (*it).fullEmailAddr();
      addrs << addr;
      toShow << i18n( "Identity: %1" ).arg( addr );
    }

    // Choose
    mReceiver =
      KInputDialog::getItem( i18n( "Select Address" ), message,
                             toShow, 0, false, &ok, kmkernel->mainWin() );
    if( !ok ) {
      mReceiver = QString::null;

      // Since the user cancelled the dialog, we do want to ask again
      mReceiverSet = false;
    } else {
      // If this is an identity, we need to set it to the right address
      const int i = toShow.findIndex( mReceiver );
      if ( i >= receivers )
        // Do the correction
        mReceiver = addrs[ i ];
    }
  }

  kdDebug(5006) << "Receiver: " << mReceiver << endl;

  return mReceiver;
}

