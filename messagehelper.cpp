/* -*- mode: C++; c-file-style: "gnu" -*-
  Copyright (C) 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Copyright (c) 2009 Andras Mantia <andras@kdab.net>

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

#include "kmkernel.h"
#include "messagehelper.h"
#include "stringutil.h"
#include "messageviewer/stringutil.h"
#include "version-kmail.h"
#include "kmversion.h"
#include "templateparser.h"
#include "messageinfo.h"
#include "mdnadvicedialog.h"
#include "mailinglist-magic.h"
#include "foldercollection.h"
#include "util.h"

#include <messageviewer/objecttreeparser.h>
#include <messageviewer/kcursorsaver.h>

#include <KDateTime>
#include <KProtocolManager>
#include <KMime/Message>
#include <kmime/kmime_mdn.h>
#include <kmime/kmime_dateformatter.h>
#include <kmime/kmime_headers.h>
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>
#include <KPIMUtils/Email>

namespace KMail {

namespace MessageHelper {

//TODO init them somewhere, now the init code is in KMMsgBase::readConfig()
static QStringList sReplySubjPrefixes, sForwardSubjPrefixes;
static bool sReplaceSubjPrefix, sReplaceForwSubjPrefix;

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

static int requestAdviceOnMDN( const char * what ) {
  for ( int i = 0 ; i < numMdnMessageBoxes ; ++i )
    if ( !qstrcmp( what, mdnMessageBoxes[i].dontAskAgainID ) ) {
      const KCursorSaver saver( Qt::ArrowCursor );
      MDNAdviceDialog::MDNAdvice answer;
      answer = MDNAdviceDialog::questionIgnoreSend( mdnMessageBoxes[i].text,
                                                    mdnMessageBoxes[i].canDeny );
      switch ( answer ) {
        case MDNAdviceDialog::MDNSend:
          return 3;

        case MDNAdviceDialog::MDNSendDenied:
          return 2;

        default:
        case MDNAdviceDialog::MDNIgnore:
          return 0;
      }
    }
  kWarning() <<"didn't find data for message box \""
                 << what << "\"";
  return 0;
}

QString replaceHeadersInString( const KMime::Message::Ptr &msg, const QString & s )
{
    QString result = s;
    QRegExp rx( "\\$\\{([a-z0-9-]+)\\}", Qt::CaseInsensitive );
    Q_ASSERT( rx.isValid() );

    QRegExp rxDate( "\\$\\{date\\}" );
    Q_ASSERT( rxDate.isValid() );

    QString sDate = KMime::DateFormatter::formatDate(
        KMime::DateFormatter::Localized, msg->date()->dateTime().dateTime().toTime_t() );

    int idx = 0;
    if( ( idx = rxDate.indexIn( result, idx ) ) != -1  ) {
      result.replace( idx, rxDate.matchedLength(), sDate );
    }

    idx = 0;
    while ( ( idx = rx.indexIn( result, idx ) ) != -1 ) {
      QString replacement = msg->headerByType( rx.cap(1).toLatin1() ) ? msg->headerByType( rx.cap(1).toLatin1() )->asUnicodeString() : "";
      result.replace( idx, rx.matchedLength(), replacement );
      idx += replacement.length();
    }
    return result;
}


void initHeader( const KMime::Message::Ptr &message, uint id )
{
  applyIdentity( message, id );
  message->to()->clear();
  message->subject()->clear();
  message->date()->setDateTime( KDateTime::currentLocalDateTime() );

  // user agent, e.g. KMail/1.9.50 (Windows/5.0; KDE/3.97.1; i686; svn-762186; 2008-01-15)
  QStringList extraInfo;
# if defined KMAIL_SVN_REVISION_STRING && defined KMAIL_SVN_LAST_CHANGE
  extraInfo << KMAIL_SVN_REVISION_STRING << KMAIL_SVN_LAST_CHANGE;
#else
#error forgot to include version-kmail.h
# endif

  message->userAgent()->fromUnicodeString( KProtocolManager::userAgentForApplication( "KMail", KMAIL_VERSION, extraInfo ), "utf-8" );

  // This will allow to change Content-Type:
  message->contentType()->setMimeType( "text/plain" );
}

void applyIdentity( const KMime::Message::Ptr &message, uint id )
{
  const KPIMIdentities::Identity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( id );

  if(ident.fullEmailAddr().isEmpty())
    message->from()->clear();
  else
    message->from()->addAddress(ident.fullEmailAddr().toUtf8());

  if(ident.replyToAddr().isEmpty())
    message->replyTo()->clear();
  else
    message->replyTo()->addAddress(ident.replyToAddr().toUtf8());

  if(ident.bcc().isEmpty())
    message->bcc()->clear();
  else
    message->bcc()->addAddress(ident.bcc().toUtf8());

  if (ident.organization().isEmpty())
    message->removeHeader("Organization");
  else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "Organization", message.get(), ident.organization(), "utf-8" );
    message->setHeader( header );
  }

  if (ident.isDefault())
    message->removeHeader("X-KMail-Identity");
  else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Identity", message.get(), QString::number( ident.uoid() ), "utf-8" );
    message->setHeader( header );
  }

  if (ident.transport().isEmpty())
    message->removeHeader("X-KMail-Transport");
  else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Transport", message.get(), ident.transport(), "utf-8" );
    message->setHeader( header );
  }

  if (ident.fcc().isEmpty())
    message->removeHeader("X-KMail-Fcc");
  else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Fcc", message.get(), ident.fcc(), "utf-8" );
    message->setHeader( header );
  }

  if (ident.drafts().isEmpty())
    message->removeHeader("X-KMail-Drafts");
  else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Drafts", message.get(), ident.drafts(), "utf-8" );
    message->setHeader( header );
  }

  if (ident.templates().isEmpty())
    message->removeHeader("X-KMail-Templates");
  else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Templates", message.get(), ident.templates(), "utf-8" );
    message->setHeader( header );
  }
}

MessageReply createReply( const Akonadi::Item &item,
                          const KMime::Message::Ptr &origMsg,
                          KMail::ReplyStrategy replyStrategy,
                          const QString &selection /*.clear() */,
                          bool noQuote /* = false */,
                          bool allowDecryption /* = true */,
                          bool selectionIsBody /* = false */,
                          const QString &tmpl /* = QString() */ )
{
  KMime::Message::Ptr msg( new KMime::Message );
  QString str, mailingListStr, replyToStr, toStr;
  QStringList mailingListAddresses;
  QByteArray refStr, headerName;
  bool replyAll = true;

  initFromMessage( item, msg, origMsg);
  MailingList::name( origMsg, headerName, mailingListStr );
  replyToStr = origMsg->replyTo()->asUnicodeString();

  msg->contentType()->setCharset("utf-8");
  // determine the mailing list posting address
  Akonadi::Collection parentCollection = item.parentCollection();
  QSharedPointer<FolderCollection> fd;
  if ( parentCollection.isValid() ) {
    fd = FolderCollection::forCollection( parentCollection );
    if ( fd->isMailingListEnabled() && !fd->mailingListPostAddress().isEmpty() ) {
      mailingListAddresses << fd->mailingListPostAddress();
    }
  }

  if ( origMsg->headerByType("List-Post") && origMsg->headerByType("List-Post")->asUnicodeString().contains( "mailto:", Qt::CaseInsensitive ) ) {
    QString listPost = origMsg->headerByType("List-Post")->asUnicodeString();
    QRegExp rx( "<mailto:([^@>]+)@([^>]+)>", Qt::CaseInsensitive );
    if ( rx.indexIn( listPost, 0 ) != -1 ) // matched
      mailingListAddresses << rx.cap(1) + '@' + rx.cap(2);
  }

  switch( replyStrategy ) {
  case ReplySmart : {
    if ( origMsg->headerByType( "Mail-Followup-To" ) ) {
      toStr = origMsg->headerByType( "Mail-Followup-To" )->asUnicodeString();
    }
    else if ( !replyToStr.isEmpty() ) {
      toStr = replyToStr;
      // use the ReplyAll template only when it's a reply to a mailing list
      if ( mailingListAddresses.isEmpty() )
        replyAll = false;
    }
    else if ( !mailingListAddresses.isEmpty() ) {
      toStr = mailingListAddresses[0];
    }
    else {

      // doesn't seem to be a mailing list, reply to From: address
      toStr = origMsg->from()->asUnicodeString();

      if( kmkernel->identityManager()->thatIsMe( KPIMUtils::extractEmailAddress( toStr ) ) ) {
        // sender seems to be one of our own identities, so we assume that this
        // is a reply to a "sent" mail where the users wants to add additional
        // information for the recipient.
        toStr = origMsg->to()->asUnicodeString();
      }

      replyAll = false;
    }
    // strip all my addresses from the list of recipients
    QStringList recipients = KPIMUtils::splitAddressList( toStr );
    toStr = StringUtil::stripMyAddressesFromAddressList( recipients ).join(", ");
    // ... unless the list contains only my addresses (reply to self)
    if ( toStr.isEmpty() && !recipients.isEmpty() )
      toStr = recipients[0];

    break;
  }
  case ReplyList : {
    if ( origMsg->headerByType( "Mail-Followup-To" ) ) {
      toStr = origMsg->headerByType( "Mail-Followup-To" )->asUnicodeString();
    }
    else if ( !mailingListAddresses.isEmpty() ) {
      toStr = mailingListAddresses[0];
    }
    else if ( !replyToStr.isEmpty() ) {
      // assume a Reply-To header mangling mailing list
      toStr = replyToStr;
    }
    // strip all my addresses from the list of recipients
    QStringList recipients = KPIMUtils::splitAddressList( toStr );
    toStr = StringUtil::stripMyAddressesFromAddressList( recipients ).join(", ");

    break;
  }
  case ReplyAll : {
    QStringList recipients;
    QStringList ccRecipients;

    // add addresses from the Reply-To header to the list of recipients
    if( !replyToStr.isEmpty() ) {
      recipients += KPIMUtils::splitAddressList( replyToStr );
      // strip all possible mailing list addresses from the list of Reply-To
      // addresses
      for ( QStringList::const_iterator it = mailingListAddresses.constBegin();
            it != mailingListAddresses.constEnd();
            ++it ) {
        recipients = MessageViewer::StringUtil::stripAddressFromAddressList( *it, recipients );
      }
    }

    if ( !mailingListAddresses.isEmpty() ) {
      // this is a mailing list message
      if ( recipients.isEmpty() && !origMsg->from()->asUnicodeString().isEmpty() ) {
        // The sender didn't set a Reply-to address, so we add the From
        // address to the list of CC recipients.
        ccRecipients += origMsg->from()->asUnicodeString();
        kDebug() << "Added" << origMsg->from()->asUnicodeString() <<"to the list of CC recipients";
      }
      // if it is a mailing list, add the posting address
      recipients.prepend( mailingListAddresses[0] );
    }
    else {
      // this is a normal message
      if ( recipients.isEmpty() && !origMsg->from()->asUnicodeString().isEmpty() ) {
        // in case of replying to a normal message only then add the From
        // address to the list of recipients if there was no Reply-to address
        recipients +=  origMsg->from()->asUnicodeString();
        kDebug() << "Added" <<  origMsg->from()->asUnicodeString() <<"to the list of recipients";
      }
    }

    // strip all my addresses from the list of recipients
    toStr = StringUtil::stripMyAddressesFromAddressList( recipients ).join(", ");

    // merge To header and CC header into a list of CC recipients
    if( !origMsg->cc()->asUnicodeString().isEmpty() || !origMsg->to()->asUnicodeString().isEmpty() ) {
      QStringList list;
      if (!origMsg->to()->asUnicodeString().isEmpty())
        list += KPIMUtils::splitAddressList(origMsg->to()->asUnicodeString());
      if (!origMsg->cc()->asUnicodeString().isEmpty())
        list += KPIMUtils::splitAddressList(origMsg->cc()->asUnicodeString());
      for( QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it ) {
        if(    !MessageViewer::StringUtil::addressIsInAddressList( *it, recipients )
            && !MessageViewer::StringUtil::addressIsInAddressList( *it, ccRecipients ) ) {
          ccRecipients += *it;
          kDebug() << "Added" << *it <<"to the list of CC recipients";
        }
      }
    }

    if ( !ccRecipients.isEmpty() ) {
      // strip all my addresses from the list of CC recipients
      ccRecipients = StringUtil::stripMyAddressesFromAddressList( ccRecipients );

      // in case of a reply to self toStr might be empty. if that's the case
      // then propagate a cc recipient to To: (if there is any).
      if ( toStr.isEmpty() && !ccRecipients.isEmpty() ) {
        toStr = ccRecipients[0];
        ccRecipients.pop_front();
      }

      msg->cc()->fromUnicodeString( ccRecipients.join(", "), "utf-8" );
    }

    if ( toStr.isEmpty() && !recipients.isEmpty() ) {
      // reply to self without other recipients
      toStr = recipients[0];
    }
    break;
  }
  case ReplyAuthor : {
    if ( !replyToStr.isEmpty() ) {
      QStringList recipients = KPIMUtils::splitAddressList( replyToStr );
      // strip the mailing list post address from the list of Reply-To
      // addresses since we want to reply in private
      for ( QStringList::const_iterator it = mailingListAddresses.constBegin();
            it != mailingListAddresses.constEnd();
            ++it ) {
        recipients = MessageViewer::StringUtil::stripAddressFromAddressList( *it, recipients );
      }
      if ( !recipients.isEmpty() ) {
        toStr = recipients.join(", ");
      }
      else {
        // there was only the mailing list post address in the Reply-To header,
        // so use the From address instead
        toStr =  origMsg->from()->asUnicodeString();
      }
    }
    else if ( ! origMsg->from()->asUnicodeString().isEmpty() ) {
      toStr = origMsg->from()->asUnicodeString();
    }
    replyAll = false;
    break;
  }
  case ReplyNone : {
    // the addressees will be set by the caller
  }
  }

  msg->to()->fromUnicodeString(toStr, "utf-8");

  refStr = getRefStr( origMsg );
  if (!refStr.isEmpty())
    msg->references()->fromUnicodeString (refStr, "utf-8" );
  //In-Reply-To = original msg-id
  msg->inReplyTo()->fromUnicodeString( origMsg->messageID()->asUnicodeString(), "utf-8" );

  msg->subject()->fromUnicodeString( replySubject( origMsg ), "utf-8" );

  // If the reply shouldn't be blank, apply the template to the message
  if ( !noQuote ) {
    TemplateParser parser( msg, (replyAll ? TemplateParser::ReplyAll : TemplateParser::Reply),
                           selection,kmkernel->smartQuote(), allowDecryption, selectionIsBody );
    if ( !tmpl.isEmpty() )
      parser.process( tmpl, origMsg );
    else
      parser.process( origMsg );
  }
  link( msg, item, KPIM::MessageStatus::statusReplied() );
  if ( parentCollection.isValid() && fd->putRepliesInSameFolder() ) {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Fcc", msg.get(), QString::number( parentCollection.id() ), "utf-8" );
    msg->setHeader( header );
  }
#if 0
  // replies to an encrypted message should be encrypted as well
  if ( encryptionState() == KMMsgPartiallyEncrypted ||
       encryptionState() == KMMsgFullyEncrypted ) {
    msg->setEncryptionState( KMMsgFullyEncrypted );
  }

#else //TODO port to akonadi
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif

  MessageReply reply;
  reply.msg = msg;
  reply.replyAll = replyAll;
  return reply;
}

void link( const KMime::Message::Ptr &msg, const Akonadi::Item & item,const KPIM::MessageStatus& aStatus )
{
  Q_ASSERT( aStatus.isReplied() || aStatus.isForwarded() || aStatus.isDeleted() );

  QString message = msg->headerByType( "X-KMail-Link-Message" ) ? msg->headerByType( "X-KMail-Link-Message" )->asUnicodeString() : QString();
  if ( !message.isEmpty() )
    message += ',';
  QString type = msg->headerByType( "X-KMail-Link-Type" ) ? msg->headerByType( "X-KMail-Link-Type" )->asUnicodeString(): QString();
  if ( !type.isEmpty() )
    type += ',';

  message += QString::number( item.id() );
  if ( aStatus.isReplied() )
    type += "reply";
  else if ( aStatus.isForwarded() )
    type += "forward";
  else if ( aStatus.isDeleted() )
    type += "deleted";

  KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Link-Message", msg.get(), message, "utf-8" );
  msg->setHeader( header );

  header = new KMime::Headers::Generic( "X-KMail-Link-Type", msg.get(), type, "utf-8" );
  msg->setHeader( header );
}



KMime::Message::Ptr createForward( const Akonadi::Item &item, const KMime::Message::Ptr &origMsg, const QString &tmpl /* = QString() */ )
{
  KMime::Message::Ptr msg( new KMime::Message );

  // This is a non-multipart, non-text mail (e.g. text/calendar). Construct
  // a multipart/mixed mail and add the original body as an attachment.
  if ( !origMsg->contentType()->isMultipart() &&
      ( !origMsg->contentType()->isText() ||
      ( origMsg->contentType()->isText() && origMsg->contentType()->subType() != "html"
        && origMsg->contentType()->subType() != "plain" ) ) ) {
    initFromMessage( item, msg, origMsg );
    msg->removeHeader("Content-Type");
    msg->removeHeader("Content-Transfer-Encoding");

    msg->contentType()->setMimeType( "multipart/mixed" );

  //TODO: Andras: somebody should check if this is correct. :)
    // empty text part
    KMime::Content *msgPart = new KMime::Content;
    msgPart->contentType()->setMimeType("text/plain");
    msg->addContent( msgPart );

    // the old contents of the mail
    KMime::Content *secondPart = new KMime::Content;
    secondPart->contentType()->setMimeType( origMsg->contentType()->mimeType() );
    secondPart->setBody( origMsg->body() );
    // use the headers of the original mail
    secondPart->setHead( origMsg->head() );
    msg->addContent( secondPart );
    msg->assemble();
    //TODO Port it msg->cleanupHeader();
  }

  // Normal message (multipart or text/plain|html)
  // Just copy the message, the template parser will do the hard work of
  // replacing the body text in TemplateParser::addProcessedBodyToMessage()
  else {
//TODO Check if this is ok
    msg->setHead( origMsg->head() );
    msg->setBody( origMsg->body() );
    QString oldContentType = msg->contentType()->asUnicodeString();
    initFromMessage(item, msg, origMsg );

    // restore the content type, initFromMessage() sets the contents type to
    // text/plain, via initHeader(), for unclear reasons
    msg->contentType()->fromUnicodeString( oldContentType, "utf-8" );
    msg->assemble();
  }

  msg->subject()->fromUnicodeString( forwardSubject( origMsg ), "utf-8" );

  TemplateParser parser( msg, TemplateParser::Forward,
                         QString(),
                         false, false, false);
  if ( !tmpl.isEmpty() )
    parser.process( tmpl, origMsg );
  else
    parser.process( origMsg );

  link( msg, item, KPIM::MessageStatus::statusForwarded() );
  return msg;
}


KMime::Message::Ptr createResend( const Akonadi::Item & item, const KMime::Message::Ptr &origMsg )
{
  KMime::Message::Ptr msg( new KMime::Message );
  initFromMessage( item, msg, origMsg);
  msg->setContent( origMsg->encodedContent() );
  msg->removeHeader( "Message-Id" );
  uint originalIdentity = identityUoid( item, origMsg);

  // Remove all unnecessary headers
  msg->removeHeader("Bcc");
  msg->removeHeader( "Cc" );
  msg->removeHeader( "To" );
  msg->removeHeader( "Subject" );
  // Set the identity from above
  KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Identity", msg.get(), QString::number( originalIdentity ), "utf-8" );
  msg->setHeader( header );

  // Restore the original bcc field as this is overwritten in applyIdentity
  msg->bcc( origMsg->bcc() );
  return msg;
}

KMime::Message::Ptr createRedirect( const Akonadi::Item & item, const QString &toStr )
{
  const KMime::Message::Ptr origMsg = KMail::Util::message( item );
  if ( !origMsg )
    return KMime::Message::Ptr();

  // copy the message 1:1
  KMime::Message::Ptr msg( new KMime::Message );
  msg->setContent( origMsg->encodedContent() );

  uint id = 0;
  QString strId = msg->headerByType( "X-KMail-Identity" ) ? msg->headerByType( "X-KMail-Identity" )->asUnicodeString().trimmed() : "";
  if ( !strId.isEmpty())
    id = strId.toUInt();
  const KPIMIdentities::Identity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( id );

  // X-KMail-Redirect-From: content
  QString strByWayOf = QString("%1 (by way of %2 <%3>)")
    .arg( origMsg->from()->asUnicodeString() )
    .arg( ident.fullName() )
    .arg( ident.emailAddr() );

  // Resent-From: content
  QString strFrom = QString("%1 <%2>")
    .arg( ident.fullName() )
    .arg( ident.emailAddr() );

  // format the current date to be used in Resent-Date:
  QString origDate = msg->date()->asUnicodeString();
  msg->date()->setDateTime( KDateTime::currentLocalDateTime() );
  QString newDate = msg->date()->asUnicodeString();

  // prepend Resent-*: headers (c.f. RFC2822 3.6.6)
  KMime::Headers::Generic *header = new KMime::Headers::Generic( "Resent-Message-ID", msg.get(), MessageViewer::StringUtil::generateMessageId( msg->sender()->asUnicodeString() ), "utf-8" );
  msg->setHeader( header );

  header = new KMime::Headers::Generic( "Resent-Date", msg.get(), newDate, "utf-8" );
  msg->setHeader( header );

  header = new KMime::Headers::Generic( "Resent-To", msg.get(), toStr, "utf-8" );
  msg->setHeader( header );
  header = new KMime::Headers::Generic( "Resent-To", msg.get(), strFrom, "utf-8" );
  msg->setHeader( header );

  header = new KMime::Headers::Generic( "X-KMail-Redirect-From", msg.get(), strByWayOf, "utf-8" );
  msg->setHeader( header );
  header = new KMime::Headers::Generic( "X-KMail-Recipients", msg.get(), toStr, "utf-8" );
  msg->setHeader( header );

  link( msg, item, KPIM::MessageStatus::statusForwarded() );
  return msg;
}


void initFromMessage(const Akonadi::Item & item, const KMime::Message::Ptr &msg, const KMime::Message::Ptr &origMsg, bool idHeaders)
{
  uint id = identityUoid( item, origMsg );

  if ( idHeaders )
    initHeader( msg, id );
  else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Identity", msg.get(), QString::number(id), "utf-8" );
    msg->setHeader( header );
  }
  if (origMsg->headerByType("X-KMail-Transport")) {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Identity", msg.get(), origMsg->headerByType("X-KMail-Transport")->asUnicodeString(), "utf-8" );
    msg->setHeader( header );
  }
}

KMime::Types::AddrSpecList extractAddrSpecs( const KMime::Message::Ptr &msg, const QByteArray & header )
{
  KMime::Types::AddrSpecList result;
  if ( !msg->headerByType( header ) )
    return result;

  KMime::Types::AddressList al =  MessageViewer::StringUtil::splitAddrField( msg->headerByType( header )->asUnicodeString().toUtf8() );
  for ( KMime::Types::AddressList::const_iterator ait = al.constBegin() ; ait != al.constEnd() ; ++ait )
    for ( KMime::Types::MailboxList::const_iterator mit = (*ait).mailboxList.constBegin() ; mit != (*ait).mailboxList.constEnd() ; ++mit )
      result.push_back( (*mit).addrSpec() );
  return result;
}

QString cleanSubject( const KMime::Message::Ptr &msg )
{
  return cleanSubject( msg, sReplySubjPrefixes + sForwardSubjPrefixes,
		       true, QString() ).trimmed();
}

QString cleanSubject( const KMime::Message::Ptr &msg, const QStringList & prefixRegExps,
                      bool replace, const QString & newPrefix )
{
  return replacePrefixes( msg->subject()->asUnicodeString(), prefixRegExps, replace,
                                     newPrefix );
}

QString forwardSubject( const KMime::Message::Ptr &msg )
{
  return cleanSubject( msg, sForwardSubjPrefixes, sReplaceForwSubjPrefix, "Fwd:" );
}

QString replySubject( const KMime::Message::Ptr &msg )
{
  return cleanSubject( msg, sReplySubjPrefixes, sReplaceSubjPrefix, "Re:" );
}

QString replacePrefixes( const QString& str, const QStringList &prefixRegExps,
                         bool replace, const QString &newPrefix )
{
  bool recognized = false;
  // construct a big regexp that
  // 1. is anchored to the beginning of str (sans whitespace)
  // 2. matches at least one of the part regexps in prefixRegExps
  QString bigRegExp = QString::fromLatin1("^(?:\\s+|(?:%1))+\\s*")
                      .arg( prefixRegExps.join(")|(?:") );
  QRegExp rx( bigRegExp, Qt::CaseInsensitive );
  if ( !rx.isValid() ) {
    kWarning() << "bigRegExp = \""
                   << bigRegExp << "\"\n"
                   << "prefix regexp is invalid!";
    // try good ole Re/Fwd:
    recognized = str.startsWith( newPrefix );
  } else { // valid rx
    QString tmp = str;
    if ( rx.indexIn( tmp ) == 0 ) {
      recognized = true;
      if ( replace )
        return tmp.replace( 0, rx.matchedLength(), newPrefix + ' ' );
    }
  }
  if ( !recognized )
    return newPrefix + ' ' + str;
  else
    return str;
}

uint identityUoid( const Akonadi::Item & item , const KMime::Message::Ptr &msg )
{
  QString idString;
  if ( msg->headerByType("X-KMail-Identity") )
    idString = msg->headerByType("X-KMail-Identity")->asUnicodeString().trimmed();
  bool ok = false;
  int id = idString.toUInt( &ok );

  if ( !ok || id == 0 )
    id = kmkernel->identityManager()->identityForAddress( msg->to()->asUnicodeString() + ", " + msg->cc()->asUnicodeString() ).uoid();

  if ( id == 0 && item.isValid() ) {
    if ( item.parentCollection().isValid() ) {
      QSharedPointer<FolderCollection> fd = FolderCollection::forCollection( item.parentCollection() );
      id = fd->identity();
    }
  }
  return id;
}

KMime::Message::Ptr createDeliveryReceipt( const Akonadi::Item & item, const KMime::Message::Ptr &msg )
{
  QString str, receiptTo;
  KMime::Message::Ptr receipt;

  receiptTo = msg->headerByType("Disposition-Notification-To") ? msg->headerByType("Disposition-Notification-To")->asUnicodeString() : "";
  if ( receiptTo.trimmed().isEmpty() )
    return  KMime::Message::Ptr();
  receiptTo.remove( '\n' );

  receipt =  KMime::Message::Ptr( new KMime::Message );
  initFromMessage( item, receipt, msg );
  receipt->to()->fromUnicodeString( receiptTo, "utf-8" );
  receipt->subject()->fromUnicodeString( i18n("Receipt: ") + msg->subject()->asUnicodeString(), "utf-8");

  str  = "Your message was successfully delivered.";
  str += "\n\n---------- Message header follows ----------\n";
  str += msg->head();
  str += "--------------------------------------------\n";
  // Conversion to toLatin1 is correct here as Mail headers should contain
  // ascii only
  receipt->setBody(str.toLatin1());
  setAutomaticFields( receipt );
  receipt->assemble();

  return receipt;
}

KMime::Message::Ptr createMDN( const Akonadi::Item & item,
                               const KMime::Message::Ptr &msg,
                               KMime::MDN::ActionMode a,
                               KMime::MDN::DispositionType d,
                               bool allowGUI,
                               QList<KMime::MDN::DispositionModifier> m )
{
  // RFC 2298: At most one MDN may be issued on behalf of each
  // particular recipient by their user agent.  That is, once an MDN
  // has been issued on behalf of a recipient, no further MDNs may be
  // issued on behalf of that recipient, even if another disposition
  // is performed on the message.
//#define MDN_DEBUG 1
#ifndef MDN_DEBUG
  if ( MessageInfo::instance()->mdnSentState( msg.get() ) != KMMsgMDNStateUnknown &&
       MessageInfo::instance()->mdnSentState( msg.get() ) != KMMsgMDNNone )
    return KMime::Message::Ptr();
#else
  char st[2]; st[0] = (char)MessageInfo::instance()->mdnSentState( msg ); st[1] = 0;
  kDebug() << "mdnSentState() == '" << st <<"'";
#endif

  // RFC 2298: An MDN MUST NOT be generated in response to an MDN.
  if ( MessageViewer::ObjectTreeParser::findType( msg.get(), "message",
                       "disposition-notification", true, true ) ) {
    MessageInfo::instance()->setMDNSentState( msg.get(), KMMsgMDNIgnore );
    return KMime::Message::Ptr();
  }

  // extract where to send to:
  QString receiptTo = msg->headerByType("Disposition-Notification-To") ? msg->headerByType("Disposition-Notification-To")->asUnicodeString() : "";
  if ( receiptTo.trimmed().isEmpty() ) return KMime::Message::Ptr();
  receiptTo.remove( '\n' );


  KMime::MDN::SendingMode s = KMime::MDN::SentAutomatically; // set to manual if asked user
  QString special; // fill in case of error, warning or failure
  KConfigGroup mdnConfig( KMKernel::config(), "MDN" );

  // default:
  int mode = mdnConfig.readEntry( "default-policy", 0 );
  if ( !mode || mode < 0 || mode > 3 ) {
    // early out for ignore:
    MessageInfo::instance()->setMDNSentState( msg.get(), KMMsgMDNIgnore );
    return KMime::Message::Ptr();
  }
  if ( mode == 1 /* ask */ && !allowGUI )
    return KMime::Message::Ptr(); // don't setMDNSentState here!

  // RFC 2298: An importance of "required" indicates that
  // interpretation of the parameter is necessary for proper
  // generation of an MDN in response to this request.  If a UA does
  // not understand the meaning of the parameter, it MUST NOT generate
  // an MDN with any disposition type other than "failed" in response
  // to the request.
  QString notificationOptions = msg->headerByType("Disposition-Notification-Options") ? msg->headerByType("Disposition-Notification-Options")->asUnicodeString() : "";
  if ( notificationOptions.contains( "required", Qt::CaseSensitive ) ) {
    // ### hacky; should parse...
    // There is a required option that we don't understand. We need to
    // ask the user what we should do:
    if ( !allowGUI ) return KMime::Message::Ptr(); // don't setMDNSentState here!
    mode = requestAdviceOnMDN( "mdnUnknownOption" );
    s = KMime::MDN::SentManually;

    special = i18n("Header \"Disposition-Notification-Options\" contained "
                   "required, but unknown parameter");
    d = KMime::MDN::Failed;
    m.clear(); // clear modifiers
  }

  // RFC 2298: [ Confirmation from the user SHOULD be obtained (or no
  // MDN sent) ] if there is more than one distinct address in the
  // Disposition-Notification-To header.
  kDebug() << "KPIMUtils::splitAddressList(receiptTo):" // krazy:exclude=kdebug
           << KPIMUtils::splitAddressList(receiptTo).join("\n");
  if ( KPIMUtils::splitAddressList(receiptTo).count() > 1 ) {
    if ( !allowGUI ) return KMime::Message::Ptr(); // don't setMDNSentState here!
    mode = requestAdviceOnMDN( "mdnMultipleAddressesInReceiptTo" );
    s = KMime::MDN::SentManually;
  }

  // RFC 2298: MDNs SHOULD NOT be sent automatically if the address in
  // the Disposition-Notification-To header differs from the address
  // in the Return-Path header. [...] Confirmation from the user
  // SHOULD be obtained (or no MDN sent) if there is no Return-Path
  // header in the message [...]
  KMime::Types::AddrSpecList returnPathList = extractAddrSpecs( msg, "Return-Path");
  QString returnPath = returnPathList.isEmpty() ? QString()
    : returnPathList.front().localPart + '@' + returnPathList.front().domain ;
  kDebug() << "clean return path:" << returnPath;
  if ( returnPath.isEmpty() || !receiptTo.contains( returnPath, Qt::CaseSensitive ) ) {
    if ( !allowGUI ) return KMime::Message::Ptr(); // don't setMDNSentState here!
    mode = requestAdviceOnMDN( returnPath.isEmpty() ?
                               "mdnReturnPathEmpty" :
                               "mdnReturnPathNotInReceiptTo" );
    s = KMime::MDN::SentManually;
  }

  if ( true ) {
    if ( mode == 1 ) { // ask
      assert( allowGUI );
      mode = requestAdviceOnMDN( "mdnNormalAsk" );
      s = KMime::MDN::SentManually; // asked user
    }

    switch ( mode ) {
      case 0: // ignore:
        MessageInfo::instance()->setMDNSentState( msg.get(), KMMsgMDNIgnore );
        return KMime::Message::Ptr();
      default:
      case 1:
        kFatal() << "The \"ask\" mode should never appear here!";
        break;
      case 2: // deny
        d = KMime::MDN::Denied;
        m.clear();
        break;
      case 3:
        break;
    }
  }


  // extract where to send from:
  QString finalRecipient = kmkernel->identityManager()
    ->identityForUoidOrDefault( identityUoid( item, msg ) ).fullEmailAddr();

  //
  // Generate message:
  //

  KMime::Message::Ptr receipt( new KMime::Message() );
  initFromMessage( item, receipt, msg);
  receipt->contentType()->from7BitString( "multipart/report" );
  receipt->removeHeader("Content-Transfer-Encoding");
  // Modify the ContentType directly (replaces setAutomaticFields(true))
  receipt->contentType()->setParameter( "report-type", "disposition-notification" );


  QString description = replaceHeadersInString( msg, KMime::MDN::descriptionFor( d, m ) );

  // text/plain part:
  KMime::Content* firstMsgPart = new KMime::Content( msg.get() );
  firstMsgPart->contentType()->setMimeType( "text/plain" );
  firstMsgPart->setBody( description.toUtf8() );
  receipt->addContent( firstMsgPart );

  // message/disposition-notification part:
  KMime::Content* secondMsgPart = new KMime::Content( msg.get() );
  secondMsgPart->contentType()->setMimeType( "message/disposition-notification" );
  //secondMsgPart.setCharset( "us-ascii" );
  //secondMsgPart.setCteStr( "7bit" );
  secondMsgPart->setBody( KMime::MDN::dispositionNotificationBodyContent(
                            finalRecipient,
                            msg->headerByType("Original-Recipient") ? msg->headerByType("Original-Recipient")->as7BitString() : "",
                            msg->messageID()->as7BitString(), /* Message-ID */
                            d, a, s, m, special ) );
  receipt->addContent( secondMsgPart );

  // message/rfc822 or text/rfc822-headers body part:
  int num = mdnConfig.readEntry( "quote-message", 0 );
  if ( num < 0 || num > 2 ) num = 0;
  /* 0=> Nothing, 1=>Full Message, 2=>HeadersOnly*/

  KMime::Content* thirdMsgPart = new KMime::Content( msg.get() );
  switch ( num ) {
  case 1:
    thirdMsgPart->contentType()->setMimeType( "message/rfc822" );
    thirdMsgPart->setBody( asSendableString( msg ) );
    receipt->addContent( thirdMsgPart );
    break;
  case 2:
    thirdMsgPart->contentType()->setMimeType( "text/rfc822-headers" );
    thirdMsgPart->setBody( headerAsSendableString( msg ) );
    receipt->addContent( thirdMsgPart );
    break;
  case 0:
  default:
    break;
  };

  receipt->to()->fromUnicodeString( receiptTo, "utf-8" );
  receipt->subject()->from7BitString( "Message Disposition Notification" );
  KMime::Headers::Generic *header = new KMime::Headers::Generic( "In-Reply-To", receipt.get(), msg->messageID()->as7BitString() );
  receipt->setHeader( header );

  receipt->references()->from7BitString( getRefStr( msg ) );

  receipt->assemble();

  kDebug() << "final message:" + receipt->body();//TODO port asString();

  //
  // Set "MDN sent" status:
  //
  KMMsgMDNSentState state = KMMsgMDNStateUnknown;
  switch ( d ) {
  case KMime::MDN::Displayed:   state = KMMsgMDNDisplayed;  break;
  case KMime::MDN::Deleted:     state = KMMsgMDNDeleted;    break;
  case KMime::MDN::Dispatched:  state = KMMsgMDNDispatched; break;
  case KMime::MDN::Processed:   state = KMMsgMDNProcessed;  break;
  case KMime::MDN::Denied:      state = KMMsgMDNDenied;     break;
  case KMime::MDN::Failed:      state = KMMsgMDNFailed;     break;
  };
  MessageInfo::instance()->setMDNSentState( msg.get(), state );

  receipt->assemble();
  return receipt;
}


void setAutomaticFields(const KMime::Message::Ptr &msg, bool aIsMulti)
{
//TODO review and port  header.MimeVersion().FromString("1.0");

  if (aIsMulti || msg->contents().size() > 1)
  {
    // Set the type to 'Multipart' and the subtype to 'Mixed'
    msg->contentType()->setMimeType( "multipart/mixed" );
    // Create a random printable string and set it as the boundary parameter
//TODO review and port    contentType.CreateBoundary(0);
  }
}

QByteArray asSendableString( const KMime::Message::Ptr &msg )
{
  KMime::Message message;
  message.setContent( msg->encodedContent() );
  removePrivateHeaderFields( KMime::Message::Ptr( &message ) );
  message.removeHeader("Bcc");
  return message.encodedContent();
}

QByteArray headerAsSendableString( const KMime::Message::Ptr &msg )
{
  KMime::Message message;
  message.setContent( msg->encodedContent() );
  removePrivateHeaderFields( KMime::Message::Ptr( &message ) );
  message.removeHeader("Bcc");
  return message.head();
}

void removePrivateHeaderFields( const KMime::Message::Ptr &msg ) {
  msg->removeHeader("Status");
  msg->removeHeader("X-Status");
  msg->removeHeader("X-KMail-EncryptionState");
  msg->removeHeader("X-KMail-SignatureState");
  msg->removeHeader("X-KMail-MDN-Sent");
  msg->removeHeader("X-KMail-Transport");
  msg->removeHeader("X-KMail-Identity");
  msg->removeHeader("X-KMail-Fcc");
  msg->removeHeader("X-KMail-Redirect-From");
  msg->removeHeader("X-KMail-Link-Message");
  msg->removeHeader("X-KMail-Link-Type");
  msg->removeHeader("X-KMail-QuotePrefix");
  msg->removeHeader("X-KMail-CursorPos");
  msg->removeHeader( "X-KMail-Templates" );
  msg->removeHeader( "X-KMail-Drafts" );
  msg->removeHeader( "X-KMail-Tag" );
}

QByteArray getRefStr( const KMime::Message::Ptr &msg )
{
  QByteArray firstRef, lastRef, refStr, retRefStr;
  int i, j;

  refStr = msg->headerByType("References") ? msg->headerByType("References")->as7BitString().trimmed() : "";

  if (refStr.isEmpty())
    return msg->messageID()->as7BitString();

  i = refStr.indexOf('<');
  j = refStr.indexOf('>');
  firstRef = refStr.mid(i, j-i+1);
  if (!firstRef.isEmpty())
    retRefStr = firstRef + ' ';

  i = refStr.lastIndexOf('<');
  j = refStr.lastIndexOf('>');

  lastRef = refStr.mid(i, j-i+1);
  if (!lastRef.isEmpty() && lastRef != firstRef)
    retRefStr += lastRef + ' ';

  retRefStr += msg->messageID()->as7BitString();
  return retRefStr;
}


QList<Nepomuk::Tag> tagList(const Akonadi::Item &msg)
{
  const Nepomuk::Resource res( msg.url() );
  return res.tags();
}

void setTagList( const Akonadi::Item& msg, const QList<Nepomuk::Tag> &tags )
{
  Nepomuk::Resource res( msg.url() );
  res.setTags( tags );
}

QString msgId( const KMime::Message::Ptr &msg )
{
  if ( !msg->headerByType("Message-Id") )
    return QString();
  QString msgId = msg->headerByType("Message-Id")->asUnicodeString();

  // search the end of the message id
  const int rightAngle = msgId.indexOf( '>' );
  if (rightAngle != -1)
    msgId.truncate( rightAngle + 1 );
  // now search the start of the message id
  const int leftAngle = msgId.lastIndexOf( '<' );
  if (leftAngle != -1)
    msgId = msgId.mid( leftAngle );
  return msgId;
}

QString ccStrip( const KMime::Message::Ptr &msg )
{
  return MessageViewer::StringUtil::stripEmailAddr( msg->cc()->asUnicodeString() );
}

QString toStrip( const KMime::Message::Ptr &msg )
{
  return MessageViewer::StringUtil::stripEmailAddr( msg->to()->asUnicodeString() );
}

QString fromStrip( const KMime::Message::Ptr &msg )
{
  return MessageViewer::StringUtil::stripEmailAddr( msg->from()->asUnicodeString() );
}


QString stripOffPrefixes( const QString& str )
{
  return replacePrefixes( str, sReplySubjPrefixes + sForwardSubjPrefixes,
                          true, QString() ).trimmed();
}

QString skipKeyword( const QString& aStr, QChar sepChar,
			       bool* hasKeyword)
{
  QString str = aStr;

  while (str[0] == ' ') str.remove(0,1);
  if (hasKeyword) *hasKeyword=false;

  unsigned int i = 0, maxChars = 3;
  unsigned int strLength(str.length());
  for (i=0; i < strLength && i < maxChars; i++)
  {
    if (str[i] < 'A' || str[i] == sepChar) break;
  }

  if (str[i] == sepChar) // skip following spaces too
  {
    do {
      i++;
    } while (str[i] == ' ');
    if (hasKeyword) *hasKeyword=true;
    return str.mid(i);
  }
  return str;
}


}

}
