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
#include "stringutil.h"
#include "messageviewer/stringutil.h"

#ifndef KMAIL_UNITTESTS

#include "messageviewer/kmaddrbook.h"
#include "kmkernel.h"

#include <libkdepim/kaddrbookexternal.h>
#include <mimelib/enum.h>

#include <kmime/kmime_charfreq.h>
#include <kmime/kmime_header_parsing.h>
#include <kmime/kmime_util.h>

#include <KPIMUtils/Email>
#include <KPIMIdentities/IdentityManager>
#include <kmime/kmime_util.h>

#include <kascii.h>
#include <KConfigGroup>
#include <KDebug>
#include <KUrl>
#include <kuser.h>

#include <QHostInfo>
#include <QRegExp>
#endif
#include <QStringList>

#ifndef KMAIL_UNITTESTS

using namespace KMime;
using namespace KMime::Types;
using namespace KMime::HeaderParsing;

#endif

namespace KMail
{

namespace StringUtil
{

#ifndef KMAIL_UNITTESTS
//-----------------------------------------------------------------------------
QList<int> determineAllowedCtes( const CharFreq& cf,
                                 bool allow8Bit,
                                 bool willBeSigned )
{
  QList<int> allowedCtes;

  switch ( cf.type() ) {
    case CharFreq::SevenBitText:
      allowedCtes << DwMime::kCte7bit;
    case CharFreq::EightBitText:
      if ( allow8Bit )
        allowedCtes << DwMime::kCte8bit;
    case CharFreq::SevenBitData:
      if ( cf.printableRatio() > 5.0/6.0 ) {
      // let n the length of data and p the number of printable chars.
      // Then base64 \approx 4n/3; qp \approx p + 3(n-p)
      // => qp < base64 iff p > 5n/6.
        allowedCtes << DwMime::kCteQp;
        allowedCtes << DwMime::kCteBase64;
      } else {
        allowedCtes << DwMime::kCteBase64;
        allowedCtes << DwMime::kCteQp;
      }
      break;
    case CharFreq::EightBitData:
      allowedCtes << DwMime::kCteBase64;
      break;
    case CharFreq::None:
    default:
    // just nothing (avoid compiler warning)
      ;
  }

  // In the following cases only QP and Base64 are allowed:
  // - the buffer will be OpenPGP/MIME signed and it contains trailing
  //   whitespace (cf. RFC 3156)
  // - a line starts with "From "
  if ( ( willBeSigned && cf.hasTrailingWhitespace() ) ||
         cf.hasLeadingFrom() ) {
    allowedCtes.removeAll( DwMime::kCte8bit );
    allowedCtes.removeAll( DwMime::kCte7bit );
         }

         return allowedCtes;
}


QString emailAddrAsAnchor( const QString& aEmail, Display display, const QString& cssStyle,
                             Link link, AddressMode expandable, const QString& fieldName )
{
  if( aEmail.isEmpty() )
    return aEmail;

  const QStringList addressList = KPIMUtils::splitAddressList( aEmail );

  QString result;
  int numberAddresses = 0;
  bool expandableInserted = false;

  for( QStringList::ConstIterator it = addressList.constBegin();
       ( it != addressList.constEnd() );
       ++it ) {
    if( !(*it).isEmpty() ) {
      numberAddresses++;

      QString address = *it;
      if( expandable == ExpandableAddresses &&
          !expandableInserted && numberAddresses > GlobalSettings::self()->numberOfAddressesToShow() ) {
        Q_ASSERT( !fieldName.isEmpty() );
        result = "<span id=\"icon" + fieldName + "\"></span>" + result;
        result += "<span id=\"dots" + fieldName + "\">...</span><span id=\"hidden" + fieldName +"\">";
        expandableInserted = true;
      }
      if( link == ShowLink ) {
        result += "<a href=\"mailto:"
                + MessageViewer::StringUtil::encodeMailtoUrl( address )
                + "\" "+cssStyle+">";
      }
      if( display == DisplayNameOnly )
        address = MessageViewer::StringUtil::stripEmailAddr( address );
      result += MessageViewer::StringUtil::quoteHtmlChars( address, true );
      if( link == ShowLink ) {
        result += "</a>, ";
      }
    }
  }
  // cut of the trailing ", "
  if( link == ShowLink ) {
    result.truncate( result.length() - 2 );
  }
  if( expandableInserted ) {
    result += "</span>";
  }

  //kDebug() << "('" << aEmail << "') returns:\n-->" << result << "<--";
  return result;
}


QStringList stripMyAddressesFromAddressList( const QStringList& list )
{
  QStringList addresses = list;
  for( QStringList::Iterator it = addresses.begin();
       it != addresses.end(); ) {
    kDebug() << "Check whether" << *it <<"is one of my addresses";
    if( kmkernel->identityManager()->thatIsMe( KPIMUtils::extractEmailAddress( *it ) ) ) {
      kDebug() << "Removing" << *it <<"from the address list";
      it = addresses.erase( it );
    }
    else
      ++it;
  }
  return addresses;
}

#endif


#ifndef KMAIL_UNITTESTS

void parseMailtoUrl ( const KUrl& url, QString& to, QString& cc, QString& subject, QString& body )
{
  kDebug() << url.pathOrUrl();
  to = MessageViewer::StringUtil::decodeMailtoUrl( url.path() );
  QMap<QString, QString> values = url.queryItems( KUrl::CaseInsensitiveKeys );
  to += ", " + values.value( "to" );
  body = values.value( "body" );
  subject = values.value( "subject" );
  cc = values.value( "cc" );
}

#endif

}

}
