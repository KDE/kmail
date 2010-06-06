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
#include "kmmsgbase.h"
#include <kurl.h>

namespace KMail
{

namespace StringUtil
{
#ifndef KMAIL_UNITTESTS
QString encodeMailtoUrl( const QString& str )
{
  QString result;
  result = QString::fromLatin1( KMMsgBase::encodeRFC2047String( str,
                                "utf-8" ) );
  result = KURL::encode_string( result );
  return result;
}

QString decodeMailtoUrl( const QString& url )
{
  QString result;
  result = KURL::decode_string( url.latin1() );
  result = KMMsgBase::decodeRFC2047String( result.latin1() );
  return result;
}
#endif

}

}
