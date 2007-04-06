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

#include "mimelibtests.h"
#include "mimelibtests.moc"

#include <kdebug.h>
#include <kunittest/runner.h>
#include <kunittest/module.h>

using namespace KUnitTest;

KUNITTEST_MODULE( kunittest_mimelibmodule, "Mimelib Tests" );
KUNITTEST_MODULE_REGISTER_TESTER( MimeLibTester );

#include <mimelib/string.h>
#include <mimelib/message.h>
#include "util.h"
#include <qfile.h>
#include <assert.h>

#if 0
static QString makePrintable( const QCString& str )
{
  QString a = str;
  a = a.replace( '\r', "\\r" );
  a = a.replace( '\n', "\\n" );
  return a;
}
#endif

static QString makePrintable( const DwString& str )
{
  QString a = KMail::Util::CString( str ); // ## we assume latin1
  a = a.replace( '\r', "\\r" );
  a = a.replace( '\n', "\\n" );
  return a;
}

QCString MimeLibTester::readFile(const QString& fileName)
{
  QFile file( fileName );
  // #!@#$& kunittest... VERIFY() does nothing in setUp. Using assert instead.
  bool ok = file.open( IO_ReadOnly );
  if ( !ok ) {
    kdError() << fileName << " not found!" << endl;
    abort();
  }
  QByteArray data = file.readAll();
  assert( data.size() > 1 );
  QCString result;
  KMail::Util::setFromByteArray( result, data );
  return result;
}

void MimeLibTester::setUp()
{
  // This multipart-mixed mail has a part that starts without headers;
  // the newline after the (empty) headers must be preserved.
  mMultipartMixedMail = readFile( KDESRCDIR "/multipartmixed.mbox" );
  // This is the full signed mail which was simplified to above.
  // Kept around in case we want to do anything else with a signed mail later :)
  mSignedMail = readFile( KDESRCDIR "/signedmail.mbox" );
}

void MimeLibTester::tearDown()
{
}

// Simply test creating a DwMessage and then calling AsString on it.
// Then the same with Parse+Assemble
bool MimeLibTester::test_dwMessage_AsString( const DwString& text )
{
  VERIFY( text.size() > 0 );

  // First without Parse + Assemble
  {
    DwMessage* msg = new DwMessage( text, 0 );
    COMPARE( makePrintable( msg->AsString() ), makePrintable( text ) );
    delete msg;
  }
  // Then with Parse + Assemble
  {
    DwMessage* msg = new DwMessage( text, 0 );
    msg->Parse();
    msg->Assemble();
    COMPARE( makePrintable( msg->AsString() ), makePrintable( text ) );
    if ( msg->AsString() != text )
      return false;
    delete msg;
  }
  return true;
}

void MimeLibTester::test_dwMessage_AsString()
{
  if ( !test_dwMessage_AsString( mMultipartMixedMail.data() ) )
    return;
  if ( !test_dwMessage_AsString( mSignedMail.data() ) )
    return;
}
