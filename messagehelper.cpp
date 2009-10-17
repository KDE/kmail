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
#include "version-kmail.h"
#include "kmversion.h"
#include "templateparser.h"

#include <KDateTime>
#include <KProtocolManager>
#include <KMime/Message>
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>

namespace KMail {

namespace MessageHelper {

//TODO init them somewhere, now the init code is in KMMsgBase::readConfig()
static QStringList sReplySubjPrefixes, sForwardSubjPrefixes;
static bool sReplaceSubjPrefix, sReplaceForwSubjPrefix;

void initHeader( KMime::Message* message, uint id )
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

void applyIdentity( KMime::Message *message, uint id )
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
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "Organization", message, ident.organization(), "utf-8" );
    message->setHeader( header );
  }

  if (ident.isDefault())
    message->removeHeader("X-KMail-Identity");
  else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Identity", message, QString::number( ident.uoid() ), "utf-8" );
    message->setHeader( header );
  }

  if (ident.transport().isEmpty())
    message->removeHeader("X-KMail-Transport");
  else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Transport", message, ident.transport(), "utf-8" );
    message->setHeader( header );
  }

  if (ident.fcc().isEmpty())
    message->removeHeader("X-KMail-Fcc");
  else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Fcc", message, ident.fcc(), "utf-8" );
    message->setHeader( header );
  }


  /** TODO Port to KMime
  if (ident.drafts().isEmpty())
    setDrafts( QString() );
  else
    setDrafts( ident.drafts() );

  if (ident.templates().isEmpty())
    setTemplates( QString() );
  else
    setTemplates( ident.templates() );
  */
}

KMime::Message* createReply( KMime::Message *origMsg,
                        ReplyStrategy replyStrategy,
                        const QString &selection /*.clear() */,
                        bool noQuote /* = false */,
                        bool allowDecryption /* = true */,
                        bool selectionIsBody /* = false */,
                        const QString &tmpl /* = QString() */ )
{
  KMime::Message* msg = new KMime::Message;
  QString str, mailingListStr, replyToStr, toStr;
  QStringList mailingListAddresses;
  QByteArray refStr, headerName;
  bool replyAll = true;

  initFromMessage( msg, origMsg );

  /** TODO port it to KMime
  MailingList::name(this, headerName, mailingListStr);
  */
  replyToStr = origMsg->replyTo()->asUnicodeString();

  msg->contentType()->setCharset("utf-8");

  /** TODO port it to KMime parent() is the parent folder of origMsg
  
  // determine the mailing list posting address
  if ( parent() && parent()->isMailingListEnabled() &&
       !parent()->mailingListPostAddress().isEmpty() ) {
    mailingListAddresses << parent()->mailingListPostAddress();
  }
  */
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
      // assume a Reply-To header mangling mailing list
      toStr = replyToStr;
    }
    else if ( !mailingListAddresses.isEmpty() ) {
      toStr = mailingListAddresses[0];
    }
    else {
      // doesn't seem to be a mailing list, reply to From: address
      toStr = origMsg->from()->asUnicodeString();
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
        recipients = StringUtil::stripAddressFromAddressList( *it, recipients );
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
        recipients += origMsg->from()->asUnicodeString();
        kDebug() << "Added" << origMsg->from()->asUnicodeString() <<"to the list of recipients";
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
        if(    !StringUtil::addressIsInAddressList( *it, recipients )
            && !StringUtil::addressIsInAddressList( *it, ccRecipients ) ) {
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

      msg->cc()->fromUnicodeString(ccRecipients.join(", "), "utf-8" );
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
        recipients = StringUtil::stripAddressFromAddressList( *it, recipients );
      }
      if ( !recipients.isEmpty() ) {
        toStr = recipients.join(", ");
      }
      else {
        // there was only the mailing list post address in the Reply-To header,
        // so use the From address instead
        toStr = origMsg->from()->asUnicodeString();
      }
    }
    else if ( !origMsg->from()->asUnicodeString().isEmpty() ) {
      toStr = origMsg->from()->asUnicodeString();
    }
    replyAll = false;
    break;
  }
  case ReplyNone : {
    // the addressees will be set by the caller
  }
  }

  msg->to()->fromUnicodeString( toStr, "utf-8" );

  /** TODO Port to KMime
  refStr = getRefStr();
  if (!refStr.isEmpty())
    msg->setReferences(refStr);
  //In-Reply-To = original msg-id
  msg->setReplyToId(msgId());  

  msg->setSubject( replySubject() );
*/


  /** TODO Port to KMime
  // If the reply shouldn't be blank, apply the template to the message
  if ( !noQuote ) {
    TemplateParser parser( msg, (replyAll ? TemplateParser::ReplyAll : TemplateParser::Reply),
                           selection, s->smartQuote, allowDecryption, selectionIsBody );
    if ( !tmpl.isEmpty() )
      parser.process( tmpl, this );
    else
      parser.process( this );
  }

  msg->link( this, MessageStatus::statusReplied() );

  if ( parent() && parent()->putRepliesInSameFolder() )
    msg->setFcc( parent()->idString() );

  // replies to an encrypted message should be encrypted as well
  if ( encryptionState() == KMMsgPartiallyEncrypted ||
       encryptionState() == KMMsgFullyEncrypted ) {
    msg->setEncryptionState( KMMsgFullyEncrypted );
  }
*/
  return msg;
}

KMime::Message* createForward( KMime::Message *origMsg, const QString &tmpl /* = QString() */ )
{
  KMime::Message* msg = new KMime::Message();

  // This is a non-multipart, non-text mail (e.g. text/calendar). Construct
  // a multipart/mixed mail and add the original body as an attachment.
  if ( !origMsg->contentType()->isMultipart() &&
      ( !origMsg->contentType()->isText() ||
      ( origMsg->contentType()->isText() && origMsg->contentType()->subType() != "html"
        && origMsg->contentType()->subType() != "plain" ) ) ) {
    initFromMessage( msg, origMsg );
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
    initFromMessage( msg, origMsg );

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

#if 0 //TODO port to akonadi
  msg->link( this, MessageStatus::statusForwarded() );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return msg;
}


void initFromMessage(KMime::Message *msg, KMime::Message *origMsg, bool idHeaders)
{
  uint id = identityUoid( origMsg );

  if ( idHeaders )
    initHeader( msg, id );
  else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Identity", msg, QString::number(id), "utf-8" );
    msg->setHeader( header );
  }
  if (origMsg->headerByType("X-KMail-Transport")) {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Identity", msg, origMsg->headerByType("X-KMail-Transport")->asUnicodeString(), "utf-8" );
    msg->setHeader( header );
  }
}    

KMime::Types::AddrSpecList extractAddrSpecs( KMime::Message* msg, const QByteArray & header )
{
  KMime::Types::AddrSpecList result;
  if ( !msg->headerByType( header ) )
    return result;

  KMime::Types::AddressList al =  StringUtil::splitAddrField( msg->headerByType( header )->asUnicodeString().toUtf8() );
  for ( KMime::Types::AddressList::const_iterator ait = al.constBegin() ; ait != al.constEnd() ; ++ait )
    for ( KMime::Types::MailboxList::const_iterator mit = (*ait).mailboxList.constBegin() ; mit != (*ait).mailboxList.constEnd() ; ++mit )
      result.push_back( (*mit).addrSpec() );
  return result;
}

QString cleanSubject( KMime::Message* msg )
{
  return cleanSubject( msg, sReplySubjPrefixes + sForwardSubjPrefixes,
		       true, QString() ).trimmed();
}

QString cleanSubject( KMime::Message* msg, const QStringList & prefixRegExps,
                                 bool replace,
                                 const QString & newPrefix )
{
  return replacePrefixes( msg->subject()->asUnicodeString(), prefixRegExps, replace,
                                     newPrefix );
}

QString forwardSubject( KMime::Message* msg )
{
  return cleanSubject( msg, sForwardSubjPrefixes, sReplaceForwSubjPrefix, "Fwd:" );
}

QString replySubject(KMime::Message* msg )
{
  return cleanSubject( msg, sReplySubjPrefixes, sReplaceSubjPrefix, "Re:" );
}

QString replacePrefixes( const QString& str,
                          const QStringList& prefixRegExps,
                          bool replace,
                          const QString& newPrefix )
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

uint identityUoid( KMime::Message *msg )
{
  QString idString;
  if ( msg->headerByType("X-KMail-Identity") )
    idString = msg->headerByType("X-KMail-Identity")->asUnicodeString().trimmed();
  bool ok = false;
  int id = idString.toUInt( &ok );

  if ( !ok || id == 0 )
    id = kmkernel->identityManager()->identityForAddress( msg->to()->asUnicodeString() + ", " + msg->cc()->asUnicodeString() ).uoid();

#if 0 //TODO port to akonadi
  if ( id == 0 && parent() )
    id = parent()->identity();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return id;
}

}
  
}
