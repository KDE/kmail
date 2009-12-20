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

#include <kmime/kmime_charfreq.h>
#include <kmime/kmime_header_parsing.h>
#include <kmime/kmime_util.h>

#include "kmkernel.h"
#include <KPIMUtils/Email>
#include <kmime/kmime_util.h>
#include <KPIMIdentities/IdentityManager>

#include <KDebug>
#include <KUrl>

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

QMap<QString, QString> parseMailtoUrl ( const KUrl& url )
{
  kDebug() << url.pathOrUrl();
  QMap<QString, QString> values = url.queryItems( KUrl::CaseInsensitiveKeys );
  QString to = MessageViewer::StringUtil::decodeMailtoUrl( url.path() );
  to = to.isEmpty() ?  values.value( "to" ) : to + QString( ", " ) + values.value( "to" );
  values.insert( "to", to );
  return values;
}

#endif

}

}
