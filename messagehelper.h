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

#include "kmmessage.h" //TODO Temporary for ReplyStrategy, move that here!

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

    /** Create a new message that is a reply to this message, filling all
      required header fields with the proper values. The returned message
      is not stored in any folder. Marks this message as replied. */
  KMime::Message* createReply( KMime::Message* origMsg, KMail::ReplyStrategy replyStrategy = KMail::ReplySmart,
                          const QString &selection=QString(), bool noQuote=false,
                          bool allowDecryption=true, bool selectionIsBody=false,
                          const QString &tmpl = QString() );
                          

  /** Create a new message that is a forward of this message, filling all
    required header fields with the proper values. The returned message
    is not stored in any folder. Marks this message as forwarded. */
  KMMessage* createForward( const QString &tmpl = QString() );

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
}
  
}

#endif