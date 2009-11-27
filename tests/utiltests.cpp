/* This file is part of the KDE project
   Copyright 2008 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#include "qtest_kde.h"
#include "utiltests.h"
#include "utiltests.moc"

QTEST_KDEMAIN_CORE( UtilTester )

#include <kdebug.h>
#include "util.h"


static QString makePrintable( const QByteArray& str )
{
  QString a = str;
  a = a.replace( '\r', "\\r" );
  a = a.replace( '\n', "\\n" );
  return a;
}

void UtilTester::test_lf2crlf()
{
  QByteArray src = "\nfoo\r\n\nbar\rblah\n\r\r\n\n\r";
  QCOMPARE( (char)src[src.size()-1], '\r' );
  QByteArray conv = KMail::Util::lf2crlf( src );
  QCOMPARE( makePrintable( conv ), makePrintable("\r\nfoo\r\n\r\nbar\rblah\r\n\r\r\n\r\n\r") );
  QCOMPARE( KMail::Util::lf2crlf( QByteArray("") ), QByteArray("") );
}

void UtilTester::test_crlf2lf()
{
  QByteArray src = "\r\n\r\nfoo\r\n\r\nbar\rblah\r\n\r\r\n\r\n\r";
  int len = src.length();
  QCOMPARE( (char)src[len], '\0' );
  int newLen = KMail::Util::crlf2lf( src.data(), len );
  QVERIFY( newLen <= len );
  QCOMPARE( makePrintable( src ), makePrintable("\n\nfoo\n\nbar\rblah\n\r\n\n\r") );
}

void UtilTester::test_escapeFrom()
{
  // TODO

}

