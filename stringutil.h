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
#include <QMap>

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
   * Returns a list of content-transfer-encodings that can be used with
   * the given result of the character frequency analysis of a message or
   * message part under the given restrictions.
   */
  QList<int> determineAllowedCtes( const KMime::CharFreq& cf,
                                   bool allow8Bit,
                                   bool willBeSigned );

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
   * Strips all the user's addresses from an address list. This is used
   * when replying.
   */
  QStringList stripMyAddressesFromAddressList( const QStringList& list );


  /**
   * Parses a mailto: url and extracts the information in the QMap (field name as key).
   */
  QMap<QString, QString> parseMailtoUrl( const KUrl &url );

}

}

#endif
