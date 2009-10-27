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

#ifndef KMAIL_MESSAGE_HELPER_H
#define KMAIL_MESSAGE_HELPER_H

#include <kmime/kmime_headers.h>
#include <kmime/kmime_mdn.h>

namespace KMail {
  /**
   * Enumeration that defines the available reply "modes"
   */
  enum ReplyStrategy {
    ReplySmart = 0,    //< Attempt to automatically guess the best recipient for the reply
    ReplyAuthor,       //< Reply to the author of the message (possibly NOT the mailing list, if any)
    ReplyList,         //< Reply to the mailing list (and not the author of the message)
    ReplyAll,          //< Reply to author and all the recipients in CC
    ReplyNone          //< Don't set reply addresses: they will be set manually
  };
}

namespace KMime {
  class Message;
}

namespace KMail {
namespace MessageHelper {

  /** Initialize header fields. Should be called on new messages
    if they are not set manually. E.g. before composing. Calling
    of setAutomaticFields(), see below, is still required. */
  void initHeader( KMime::Message* message, uint id = 0 );

  /** Set the from, to, cc, bcc, encrytion etc headers as specified in the
  * given identity. */
  void applyIdentity(KMime::Message* message,  uint id );

  /** Initialize headers fields according to the identity and the transport
   header of the given original message */
  void initFromMessage(KMime::Message *msg, KMime::Message *orgiMsg, bool idHeaders = true);

  /// Small helper structure which encapsulates the KMMessage created when creating a reply, and
  /// the reply mode
  struct MessageReply
  {
    KMime::Message *msg;  ///< The actual reply message
    bool replyAll;   ///< If true, the "reply all" template was used, otherwise the normal reply
                     ///  template
  };

    /** Create a new message that is a reply to this message, filling all
      required header fields with the proper values. The returned message
      is not stored in any folder. Marks this message as replied. */
  KMime::Message* createReply( KMime::Message* origMsg, KMail::ReplyStrategy replyStrategy = KMail::ReplySmart,
                          const QString &selection=QString(), bool noQuote=false,
                          bool allowDecryption=true, bool selectionIsBody=false,
                          const QString &tmpl = QString() );


  /**
   * Create a new message that is a reply to this message, filling all
   * required header fields with the proper values. The returned message
   * is not stored in any folder. Marks this message as replied.
   *
   * @return the reply created, including the reply mode
   */
//TODO see if this can be merged with the above one!
  MessageReply createReply2( KMime::Message* origMsg,
                            KMail::ReplyStrategy replyStrategy = KMail::ReplySmart,
                            const QString &selection=QString(), bool noQuote=false,
                            bool allowDecryption=true, bool selectionIsBody=false,
                            const QString &tmpl = QString() );

  /** Create a new message that is a forward of this message, filling all
    required header fields with the proper values. The returned message
    is not stored in any folder. Marks this message as forwarded. */
  KMime::Message* createForward(KMime::Message *origMsg, const QString &tmpl = QString() );

  /** Create a new message that is a redirect to this message, filling all
    required header fields with the proper values. The returned message
    is not stored in any folder. Marks this message as replied.
    Redirects differ from forwards so they are forwarded to some other
    user, mail is not changed and the reply-to field is set to
    the email address of the original sender
   */
  KMime::Message* createRedirect( KMime::Message *origMsg, const QString &toStr );

  KMime::Message* createResend( KMime::Message *origMsg );

  /** @return the UOID of the identity for this message.
      Searches the "x-kmail-identity" header and if that fails,
      searches with KPIMIdentities::IdentityManager::identityForAddress()
      and if that fails queries the KMMsgBase::parent() folder for a default.
   **/
  uint identityUoid();

  KMime::Types::AddrSpecList extractAddrSpecs( KMime::Message* msg, const QByteArray & header );

  /** Check for prefixes @p prefixRegExps in #subject(). If none
      is found, @p newPrefix + ' ' is prepended to the subject and the
      resulting string is returned. If @p replace is true, any
      sequence of whitespace-delimited prefixes at the beginning of
      #subject() is replaced by @p newPrefix
  **/
  QString cleanSubject( KMime::Message* msg, const QStringList& prefixRegExps, bool replace,
                        const QString& newPrefix );

  /** Return this mails subject, with all "forward" and "reply"
      prefixes removed */
  QString cleanSubject( KMime::Message* msg );

  /** Return this mails subject, formatted for "forward" mails */
  QString forwardSubject(KMime::Message* msg );

  /** Return this mails subject, formatted for "reply" mails */
  QString replySubject( KMime::Message* msg );
  /** Check for prefixes @p prefixRegExps in @p str. If none
      is found, @p newPrefix + ' ' is prepended to @p str and the
      resulting string is returned. If @p replace is true, any
      sequence of whitespace-delimited prefixes at the beginning of
      @p str is replaced by @p newPrefix.
  **/
  QString replacePrefixes( const QString& str,
                            const QStringList& prefixRegExps,
                            bool replace,
                            const QString& newPrefix );

  /** @return the UOID of the identity for this message.
      Searches the "x-kmail-identity" header and if that fails,
      searches with KPIMIdentities::IdentityManager::identityForAddress()
      and if that fails queries the KMMsgBase::parent() folder for a default.
   **/
  uint identityUoid( KMime::Message *msg );

  /** Create a new message that is a delivery receipt of this message,
      filling required header fileds with the proper values. The
      returned message is not stored in any folder. */
  KMime::Message* createDeliveryReceipt( KMime::Message* msg );

  /** Create a new message that is a MDN for this message, filling all
      required fields with proper values. The returned message is not
      stored in any folder.

      @param a Use AutomaticAction for filtering and ManualAction for
               user-induced events.
      @param d See docs for KMime::MDN::DispositionType
      @param m See docs for KMime::MDN::DispositionModifier
      @param allowGUI Set to true if this method is allowed to ask the
                      user questions

      @return The notification message or 0, if none should be sent.
   **/
  KMime::Message* createMDN( KMime::Message *msg,
          KMime::MDN::ActionMode a,
          KMime::MDN::DispositionType d,
          bool allowGUI=false,
          QList<KMime::MDN::DispositionModifier> m=QList<KMime::MDN::DispositionModifier>() );

/** Set fields that are either automatically set (Message-id)
    or that do not change from one message to another (MIME-Version).
    Call this method before sending *after* all changes to the message
    are done because this method does things different if there are
    attachments / multiple body parts. */
  void setAutomaticFields( KMime::Message* masg, bool isMultipart=false );

  /**
   * Return the message contents with the headers that should not be
   * sent stripped off.
   */
  QByteArray asSendableString( KMime::Message *msg );

  /**
   * Return the message header with the headers that should not be
   * sent stripped off.
   */
  QByteArray headerAsSendableString( KMime::Message *msg );

 /**
   * Remove all private header fields: *Status: and X-KMail-*
   **/
  void removePrivateHeaderFields( KMime::Message *msg );
  /** Creates reference string for reply to messages.
   *  reference = original first reference + original last reference + original msg-id
   */
  QByteArray getRefStr( KMime::Message *msg );

  QString ccStrip( KMime::Message *msg );
  QString toStrip( KMime::Message *msg );
  QString fromStrip( KMime::Message *msg );

  /** Returns @p str with all "forward" and "reply" prefixes stripped off.
  **/
  QString stripOffPrefixes( const QString& str );

  /** Skip leading keyword if keyword has given character at it's end
   * (e.g. ':' or ',') and skip the then following blanks (if any) too.
   * If keywordFound is specified it will be true if a keyword was skipped
   * and false otherwise. */
  QString skipKeyword(const QString& str, QChar sepChar=':',
				 bool* keywordFound=0);

}

}

#endif
