/**
 * Copyright (C)  2005 David Faure <faure@kde.org>
 * This file is subject to the GPL version 2.
 */

#include "qtest_kde.h"
#include "utiltests.h"
#include "utiltests.moc"

QTEST_KDEMAIN_CORE( UtilTester )

#include <kdebug.h>
#include "util.h"
#include <mimelib/string.h>


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

void UtilTester::test_DwStringConversions( const QByteArray& cstr )
{
  // QByteArray->DwString->QByteArray
  QCOMPARE( (int)cstr.size(), 7 );
  DwString dwstr = KMail::Util::dwString( cstr );
  QCOMPARE( (int)dwstr.size(), 7 );
  QCOMPARE( dwstr[6], 'r' );
  QByteArray cstr2 = KMail::Util::ByteArray( dwstr );
  QCOMPARE( (int)cstr2.size(), 7 );
  QCOMPARE( cstr2, cstr );
  QCOMPARE( (char)cstr2[6], 'r' );

  // DwString->QByteArray
  QByteArray ba = KMail::Util::ByteArray( dwstr );
  QCOMPARE( (int)ba.size(), 7 );
  QCOMPARE( (char)ba[6], 'r' );

  ba = KMail::Util::ByteArray( DwString() );
  QCOMPARE( (int)ba.size(), 0 );
}

void UtilTester::test_DwStringConversions()
{
  QByteArray cstr = "foo&bar";
  test_DwStringConversions( cstr );
  // now embed a nul. Note that cstr="foo\0bar" wouldn't work.
  cstr[3] = '\0';
  test_DwStringConversions( cstr );

  cstr = QByteArray();
  DwString dwstr = KMail::Util::dwString( cstr );
  QCOMPARE( (int)dwstr.size(), 0 );
  QVERIFY( dwstr.empty() );

  dwstr = KMail::Util::dwString( QByteArray() );
  QCOMPARE( (int)dwstr.size(), 0 );
  QVERIFY( dwstr.empty() );
}
