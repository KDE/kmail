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

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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
#include "kmmsgpart.h"
#include "kmmainwin.h"
#include "composer.h"
#include "kmreaderwin.h"
#include "secondarywindow.h"
#include "kmcommands.h"

#include <kpimutils/email.h>

#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>

#include <mimelib/enum.h>

#include <kinputdialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfiggroup.h>

using namespace KMail;

Callback::Callback( KMMessage *msg, KMReaderWin *readerWin )
  : mMsg( msg ), mReaderWin( readerWin ), mReceiverSet( false )
{
}

bool Callback::mailICal( const QString &to, const QString &iCal,
                         const QString &subject, const QString &status,
                         bool delMessage ) const
{
  kDebug() << "Mailing message:" << iCal;

  KMMessage *msg = new KMMessage;
  msg->initHeader();
  if ( GlobalSettings::self()->exchangeCompatibleInvitations() ) {
    msg->setSubject( status );
    QString tsubject = subject;
    tsubject.remove( i18n( "Answer: " ) );
    if ( status == QLatin1String( "cancel" ) ) {
      msg->setSubject( i18nc( "Not able to attend.", "Declined: %1", tsubject ) );
    } else if ( status == QLatin1String("tentative") ) {
      msg->setSubject( i18nc( "Unsure if it is possible to attend.", "Tentative: %1", tsubject ) );
    } else if ( status == QLatin1String("accepted") ) {
      msg->setSubject( i18nc( "Accepted the invitation.", "Accepted: %1", tsubject ) );
    } else {
      msg->setSubject( subject );
    }
  } else {
    msg->setSubject( subject );
  }
  msg->setTo( to );
  msg->setFrom( receiver() );
  if ( !GlobalSettings::self()->exchangeCompatibleInvitations() ) {
    msg->setHeaderField( "Content-Type",
                         "text/calendar; method=reply; charset=\"utf-8\"" );
    msg->setBody( iCal.toUtf8() );
  }

  if ( delMessage && deleteInvitationAfterReply() )
    /* We want the triggering mail to be moved to the trash once this one
    * has been sent successfully. Set a link header which accomplishes that. */
    msg->link( mMsg, MessageStatus::statusDeleted() );

  // Try and match the receiver with an identity.
  // Setting the identity here is important, as that is used to select the correct
  // transport later
  const KPIMIdentities::Identity &identity = kmkernel->identityManager()->identityForAddress( receiver() );
  if ( identity != KPIMIdentities::Identity::null() ) {
    msg->setHeaderField( "X-KMail-Identity", QString::number( identity.uoid() ) );
  }

  // Outlook will only understand the reply if the From: header is the
  // same as the To: header of the invitation message.
  if ( !GlobalSettings::self()->legacyMangleFromToHeaders() ) {
    if ( identity != KPIMIdentities::Identity::null() ) {
      msg->setFrom( identity.fullEmailAddr() );
    }
    // Remove BCC from identity on ical invitations (https://intevation.de/roundup/kolab/issue474)
    msg->setBcc( "" );
  }

  KMail::Composer *cWin = KMail::makeComposer();
  cWin->setMsg( msg, false /* mayAutoSign */ );
  // cWin->setCharset( "", true );
  cWin->disableWordWrap();
  cWin->setSigningAndEncryptionDisabled( true );
  if ( GlobalSettings::self()->exchangeCompatibleInvitations() ) {
    // For Exchange, send ical as attachment, with proper
    // parameters
    msg->setSubject( status );
    msg->setCharset( "utf-8" );
    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setName( "cal.ics" );
    // msgPart->setCteStr( attachCte ); // "base64" ?
    msgPart->setBodyEncoded( iCal.toUtf8() );
    msgPart->setTypeStr( "text" );
    msgPart->setSubtypeStr( "calendar" );
    msgPart->setParameter( "method", "reply" );
    cWin->addAttach( msgPart );
  }

  cWin->forceDisableHtml();
  cWin->disableRecipientNumberCheck();
  if ( GlobalSettings::self()->automaticSending() ) {
    cWin->setAttribute( Qt::WA_DeleteOnClose );
    cWin->slotSendNow();
  } else {
    cWin->show();
  }

  return true;
}

QString Callback::receiver() const
{
  if ( mReceiverSet ) {
    // Already figured this out
    return mReceiver;
  }

  mReceiverSet = true;

  QStringList addrs = KPIMUtils::splitAddressList( mMsg->to() );
  int found = 0;
  for ( QStringList::ConstIterator it = addrs.constBegin(); it != addrs.constEnd(); ++it ) {
    if ( kmkernel->identityManager()->identityForAddress( *it ) !=
         KPIMIdentities::Identity::null() ) {
      // Ok, this could be us
      ++found;
      mReceiver = *it;
    }
  }

  QStringList ccaddrs = KPIMUtils::splitAddressList( mMsg->cc() );
  for ( QStringList::ConstIterator it = ccaddrs.constBegin(); it != ccaddrs.constEnd(); ++it ) {
    if ( kmkernel->identityManager()->identityForAddress( *it ) !=
         KPIMIdentities::Identity::null() ) {
      // Ok, this could be us
      ++found;
      mReceiver = *it;
    }
  }
  if ( found != 1 ) {
    bool ok;
    QString selectMessage;
    if ( found == 0 ) {
      selectMessage = i18n("<qt>None of your identities match the "
                           "receiver of this message,<br />please "
                           "choose which of the following addresses "
                           "is yours, if any, or select one of your "
                           "identities to use in the reply:</qt>");
      addrs += kmkernel->identityManager()->allEmails();
    } else {
      selectMessage = i18n("<qt>Several of your identities match the "
                           "receiver of this message,<br />please "
                           "choose which of the following addresses "
                           "is yours:</qt>");
    }

    // select default identity by default
    const QString defaultAddr = kmkernel->identityManager()->defaultIdentity().emailAddr();
    const int defaultIndex = qMax( 0, addrs.indexOf( defaultAddr ) );

    mReceiver = KInputDialog::getItem(
      i18n( "Select Address" ),
      selectMessage,
      addrs + ccaddrs, defaultIndex, false, &ok, kmkernel->mainWin() );
    if ( !ok ) {
      mReceiver.clear();
    }
  }

  return mReceiver;
}

void Callback::closeIfSecondaryWindow() const
{
  KMail::SecondaryWindow *window =
    dynamic_cast<KMail::SecondaryWindow*>( mReaderWin->mainWindow() );
  if ( window ) {
    window->close();
  }
}

bool Callback::askForComment( KCal::Attendee::PartStat status ) const
{
  if ( ( status != KCal::Attendee::Accepted
          && GlobalSettings::self()->askForCommentWhenReactingToInvitation()
          == GlobalSettings:: EnumAskForCommentWhenReactingToInvitation::AskForAllButAcceptance )
      || GlobalSettings::self()->askForCommentWhenReactingToInvitation()
      == GlobalSettings:: EnumAskForCommentWhenReactingToInvitation::AlwaysAsk )
      return true;
  return false;
}

bool Callback::deleteInvitationAfterReply() const
{
  return GlobalSettings::self()->deleteInvitationEmailsAfterSendingReply();
}


void Callback::deleteInvitation() const
{
  const unsigned long sernum = getMsg()->getMsgSerNum();
  ( new KMTrashMsgCommand( sernum ) )->start();
}

bool Callback::exchangeCompatibleInvitations() const
{
  return GlobalSettings::self()->exchangeCompatibleInvitations();
}

bool Callback::outlookCompatibleInvitationReplyComments() const
{
  return GlobalSettings::self()->outlookCompatibleInvitationReplyComments();
}

QString Callback::sender() const
{
  return mMsg->from();
}
