/* This file is part of the KDE project
   Copyright (C) 2007 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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
#include "mimelibtests.h"
#include "mimelibtests.moc"

QTEST_KDEMAIN_CORE( MimeLibTester )

#include <mimelib/string.h>
#include <mimelib/message.h>
#include <kdebug.h>
#include "util.h"
#include <qfile.h>
#include <assert.h>

#if 0
static QString makePrintable( const QByteArray& str )
{
  QString a = str;
  a = a.replace( '\r', "\\r" );
  a = a.replace( '\n', "\\n" );
  return a;
}
#endif

static QString makePrintable( const DwString& str )
{
  QString a = KMail::Util::ByteArray( str ); // ## we assume latin1
  a = a.replace( '\r', "\\r" );
  a = a.replace( '\n', "\\n" );
  return a;
}

QByteArray MimeLibTester::readFile(const QString& fileName)
{
  QFile file( fileName );
  bool ok = file.open( QIODevice::ReadOnly );
  if ( !ok ) {
    kError() << fileName << "not found!";
    abort();
  }
  QByteArray data = file.readAll();
  assert( data.size() > 1 );
  return data;
}

void MimeLibTester::initTestCase()
{
  // This multipart-mixed mail has a part that starts without headers;
  // the newline after the (empty) headers must be preserved.
  mMultipartMixedMail = readFile( KDESRCDIR "/multipartmixed.mbox" );
  // This is the full signed mail which was simplified to above.
  // Kept around in case we want to do anything else with a signed mail later :)
  mSignedMail = readFile( KDESRCDIR "/signedmail.mbox" );
}

// Simply test creating a DwMessage and then calling AsString on it.
// Then the same with Parse+Assemble
void MimeLibTester::test_dwMessage_AsString( const DwString& text )
{
  QVERIFY( text.size() > 0 );

  // First without Parse + Assemble
  {
    DwMessage* msg = new DwMessage( text, 0 );
    QCOMPARE( makePrintable( msg->AsString() ), makePrintable( text ) );
    delete msg;
  }
  // Then with Parse + Assemble
  {
    DwMessage* msg = new DwMessage( text, 0 );
    msg->Parse();
    msg->Assemble();
    QCOMPARE( makePrintable( msg->AsString() ), makePrintable( text ) );
    delete msg;
  }
}

void MimeLibTester::test_dwMessage_AsString()
{
  test_dwMessage_AsString( mMultipartMixedMail.data() );
  test_dwMessage_AsString( mSignedMail.data() );
}
