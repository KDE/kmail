/* Copyright 2009 Thomas McGuire <mcguire@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef KMAIL_STRINGUTIL_H
#define KMAIL_STRINGUTIL_H

#include <QString>

class KUrl;
namespace KMime
{
  class CharFreq;
  namespace Types
  {
    struct Address;
    typedef QList<Address> AddressList;
  }
}

namespace KMail
{

/**
 * This namespace contain helper functions for string manipulation
 */
namespace StringUtil
{

  /**
   * Strips the signature blocks from a message text. "-- " is considered as a signature block separator.
   @param msg. The message to remove the signature block from.
   @param clearSigned. Before a message is cryptographically signed
   all trailing whitespace is removed. Therefore the signature
   separator loses the trailing space.
   */
  QString stripSignature ( const QString & msg, bool clearSigned );

  /**
   * Returns a list of content-transfer-encodings that can be used with
   * the given result of the character frequency analysis of a message or
   * message part under the given restrictions.
   */
  QList<int> determineAllowedCtes( const KMime::CharFreq& cf,
                                   bool allow8Bit,
                                   bool willBeSigned );

  /**
   * Splits the given address list into separate addresses.
   */
  KMime::Types::AddressList splitAddrField( const QByteArray & str );

  /**
   * Generates the Message-Id. It uses either the Message-Id suffix
   * defined by the user or the given email address as suffix. The address
   * must be given as addr-spec as defined in RFC 2822.
   */
  QString generateMessageId( const QString& addr );

  /**
   * Convert '<' into "&lt;" resp. '>' into "&gt;" in order to
   * prevent their interpretation by KHTML.
   * Does *not* use the Qt replace function but runs a very fast C code
   * the same way as lf2crlf() does.
   */
  QByteArray html2source( const QByteArray & src );


  /** Encodes an email address as mailto URL
   */
  QString encodeMailtoUrl( const QString& str );

  /** Decodes a mailto URL
   */
  QString decodeMailtoUrl( const QString& url );

  /**
   * This function generates a displayable string from a list of email
   * addresses.
   * Input : mailbox-list
   * Output: comma separated list of display name resp. comment resp.
   *         address
   */
  QByteArray stripEmailAddr( const QByteArray& emailAddr );

  /**
   * Does the same as the above function. Shouldn't be used.
   */
  QString stripEmailAddr( const QString& emailAddr );

  /**
   * Quotes the following characters which have a special meaning in HTML:
   * '<'  '>'  '&'  '"'. Additionally '\\n' is converted to "<br />" if
   * @p removeLineBreaks is false. If @p removeLineBreaks is true, then
   * '\\n' is removed. Last but not least '\\r' is removed.
   */
  QString quoteHtmlChars( const QString& str,
                          bool removeLineBreaks = false );

  /**
   * Used to determine if the visible part of the anchor contains
   * only the name part and not the given emailAddr or the full address.
   */
  enum Display {
    DisplayNameOnly,
    DisplayFullAddress
  };

  /** Used to determine if the address should be a link or not */
  enum Link {
    ShowLink,
    HideLink
  };

  /** Used to determine if the address field should be expandable/collapsable */
  enum AddressMode {
    ExpandableAddresses,
    FullAddresses
  };

  /** Converts the email address(es) to (a) nice HTML mailto: anchor(s). */
  QString emailAddrAsAnchor( const QString& emailAddr,
                             Display display = DisplayNameOnly, const QString& cssStyle = QString(),
                             Link link = ShowLink, AddressMode expandable = FullAddresses,
                             const QString& fieldName = QString() );

  /**
   * Strips an address from an address list. This is for example used
   * when replying to all.
   */
  QStringList stripAddressFromAddressList( const QString& address,
                                           const QStringList& addresses );

  /**
   * Strips all the user's addresses from an address list. This is used
   * when replying.
   */
  QStringList stripMyAddressesFromAddressList( const QStringList& list );

  /**
   * Returns true if the given address is contained in the given address list.
   */
  bool addressIsInAddressList( const QString& address,
                               const QStringList& addresses );

  /**
   * Expands aliases (distribution lists and nick names) and appends a
   * domain part to all email addresses which are missing the domain part.
   */
  QString expandAliases( const QString& recipients,QStringList &distributionListIsEmpty );

  /**
   * Uses the hostname as domain part and tries to determine the real name
   * from the entries in the password file.
   */
  QString guessEmailAddressFromLoginName( const QString& userName );

  /**
   *  Given argument msg add quoting characters and relayout for max width maxLength
   *  @param msg the string which it to be quoted
   *  @param maxLineLength reformat text to be this amount of columns at maximum, adding
   *    linefeeds at word boundaries to make it fit.
   */
  QString smartQuote( const QString &msg, int maxLineLength );

  /**
   * Convert wildcards into normal string
   * @param wildString the string to be converted
   * @fromAddr from email address to convert to displayable string
   */
  QString formatString( const QString &wildString, const QString &fromAddr = QString() );

  /**
   * Parses a mailto: url and extracts the information about to, cc, subject and body out into
   * the QStrings given as argument.
   */
  void parseMailtoUrl( const KUrl &url, QString &to, QString &cc, QString &subject, QString &body );

  /**
   * Determines if the MIME part with the specified type and subtype is a crypto part.
   * For example, this function returns true for type "application" and subtype "pgp-encrypted".
   * The filename is needed if the part is named "msg.asc", in which case it is also a crypto part.
   *
   * This function is useful for finding out if the part should be handled as attachment or not.
   *
   * All strings are handled case-insensitive.
   */
  bool isCryptoPart( const QString &type, const QString &subType, const QString &fileName );
}

}

#endif
