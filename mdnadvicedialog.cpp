/*
  Copyright (c) 2009 Michael Leupold <lemma@confuego.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "mdnadvicedialog.h"

#include <KMessageBox>
#include <KLocalizedString>
#include <QtCore/QPointer>
#include <KDebug>
#include <messagecomposer/messagefactory.h>
#include "kmkernel.h"
#include <messagecomposer/messageinfo.h>
#include <messageviewer/kcursorsaver.h>
#include <boost/shared_ptr.hpp>

static const struct {
  const char * dontAskAgainID;
  bool         canDeny;
  const char * text;
} mdnMessageBoxes[] = {
  { "mdnNormalAsk", true,
    I18N_NOOP("This message contains a request to return a notification "
              "about your reception of the message.\n"
              "You can either ignore the request or let KMail send a "
              "\"denied\" or normal response.") },
  { "mdnUnknownOption", false,
    I18N_NOOP("This message contains a request to send a notification "
              "about your reception of the message.\n"
              "It contains a processing instruction that is marked as "
              "\"required\", but which is unknown to KMail.\n"
              "You can either ignore the request or let KMail send a "
              "\"failed\" response.") },
  { "mdnMultipleAddressesInReceiptTo", true,
    I18N_NOOP("This message contains a request to send a notification "
              "about your reception of the message,\n"
              "but it is requested to send the notification to more "
              "than one address.\n"
              "You can either ignore the request or let KMail send a "
              "\"denied\" or normal response.") },
  { "mdnReturnPathEmpty", true,
    I18N_NOOP("This message contains a request to send a notification "
              "about your reception of the message,\n"
              "but there is no return-path set.\n"
              "You can either ignore the request or let KMail send a "
              "\"denied\" or normal response.") },
  { "mdnReturnPathNotInReceiptTo", true,
    I18N_NOOP("This message contains a request to send a notification "
              "about your reception of the message,\n"
              "but the return-path address differs from the address "
              "the notification was requested to be sent to.\n"
              "You can either ignore the request or let KMail send a "
              "\"denied\" or normal response.") },
};

static const int numMdnMessageBoxes
      = sizeof mdnMessageBoxes / sizeof *mdnMessageBoxes;

MDNAdviceDialog::MDNAdviceDialog( const QString &text, bool canDeny, QWidget *parent )
  : KDialog( parent ), m_result(MessageComposer::MDNIgnore)
{
  setCaption( i18n( "Message Disposition Notification Request" ) );
  if ( canDeny ) {
    setButtons( KDialog::Yes | KDialog::User1 | KDialog::User2 );
    setButtonText( KDialog::User2, i18n("Send \"&denied\"") );
  } else {
    setButtons( KDialog::Yes | KDialog::User1 );
  }
  setButtonText( KDialog::Yes, i18n("&Ignore") );
  setButtonText( KDialog::User1, i18n("&Send") );
  setEscapeButton( KDialog::Yes );
  KMessageBox::createKMessageBox( this, QMessageBox::Question, text,
                                  QStringList(), QString(), 0, KMessageBox::NoExec );
}

MDNAdviceDialog::~MDNAdviceDialog()
{
}

MessageComposer::MDNAdvice MDNAdviceDialog::questionIgnoreSend( const QString &text, bool canDeny )
{
  MessageComposer::MDNAdvice rc = MessageComposer::MDNIgnore;
  QPointer<MDNAdviceDialog> dlg( new MDNAdviceDialog( text, canDeny ) );
  dlg->exec();
  if ( dlg ) {
    rc = dlg->m_result;
  }
  delete dlg;
  return rc;
}

KMime::MDN::SendingMode MDNAdviceDialog::checkMDNHeaders(KMime::Message::Ptr msg)
{
  KConfigGroup mdnConfig( KMKernel::config(), "MDN" );

  // default:
  int mode = mdnConfig.readEntry( "default-policy", 0 );
  if ( !mode || mode < 0 || mode > 3 ) {
    // early out for ignore:
    MessageInfo::instance()->setMDNSentState( msg.get(), KMMsgMDNIgnore );
    return KMime::MDN::SentManually;
  }

  KMime::MDN::SendingMode s = KMime::MDN::SentAutomatically; // set to manual if asked user

  if( MessageFactory::MDNMDNUnknownOption( msg ) ) {
    mode = requestAdviceOnMDN( "mdnUnknownOption" );
    s = KMime::MDN::SentManually;
    // TODO set type to Failed as well
    //      and clear modifiers
  }
  
  if( MessageFactory::MDNConfirmMultipleRecipients( msg ) ) {
    mode = requestAdviceOnMDN( "mdnMultipleAddressesInReceiptTo" );
    s = KMime::MDN::SentManually;
  }

  if( MessageFactory::MDNReturnPathEmpty( msg ) ) {
    mode = requestAdviceOnMDN( "mdnReturnPathEmpty" );
    s = KMime::MDN::SentManually;
  }

  if( MessageFactory::MDNReturnPathNotInRecieptTo( msg ) ) {
    mode = requestAdviceOnMDN( "mdnReturnPathNotInReceiptTo" );
    s = KMime::MDN::SentManually;
  }

  // if mode is still 1, we don't have to ask but the loaded default says to always ask.
  if( mode == 1 ) {
    mode = requestAdviceOnMDN( "mdnNormalAsk" );
    s = KMime::MDN::SentManually; // asked user
  }
  //TODO: Andras: somebody should check if this is correct. :)

  if( mode == 0 ) // ignore
      MessageInfo::instance()->setMDNSentState( msg.get(), KMMsgMDNIgnore );

  return s;
/*
  switch ( mode ) {
    case 0: // ignore:
      MessageInfo::instance()->setMDNSentState( msg.get(), KMMsgMDNIgnore );
      return MessageComposer::MDNIgnore;
    default:
    case 1:
      kFatal() << "The \"ask\" mode should never appear here!";
      break;
    case 2: // deny
      return MessageComposer::MDNSendDenied;
    case 3:
      return MessageComposer::MDNSend;
  } */
}



int MDNAdviceDialog::MDNAdviceDialog::requestAdviceOnMDN(const char* what)
{
 for ( int i = 0 ; i < numMdnMessageBoxes ; ++i )
    if ( !qstrcmp( what, mdnMessageBoxes[i].dontAskAgainID ) ) {
      const MessageViewer::KCursorSaver saver( Qt::ArrowCursor );
      MessageComposer::MDNAdvice answer;
      answer = MDNAdviceDialog::questionIgnoreSend( mdnMessageBoxes[i].text,
                                                    mdnMessageBoxes[i].canDeny );
      switch ( answer ) {
        case MessageComposer::MDNSend:
          return 3;

        case MessageComposer::MDNSendDenied:
          return 2;
      // don't use 1, as that's used for 'default ask" in checkMDNHeaders
        default:
        case MessageComposer::MDNIgnore:
          return 0;
      }
    }
  kWarning() << "didn't find data for message box \""  << what << "\"";
  return MessageComposer::MDNIgnore;
}


void MDNAdviceDialog::slotButtonClicked( int button )
{
  switch ( button ) {

    case KDialog::User1:
      m_result = MessageComposer::MDNSend;
      accept();
      break;

    case KDialog::User2:
      m_result = MessageComposer::MDNSendDenied;
      accept();
      break;

    case KDialog::Yes:
    default:
      m_result = MessageComposer::MDNIgnore;
      accept();
      break;

  }
  reject();
}

#include "mdnadvicedialog.moc"
