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
#include "stringutiltest.h"

#include "stringutil.h"

#include <KDebug>
#include "qtest_kde.h"

using namespace KMail;

QTEST_KDEMAIN_CORE( StringUtilTest )

void StringUtilTest::test_signatureStripping()
{
  //QStringList tests;
  const QString test1 =
      "text1\n"
      "-- \n"
      "Signature Block1\n"
      "Signature Block1\n\n"
      "> text2\n"
      "> -- \n"
      "> Signature Block 2\n"
      "> Signature Block 2\n"
      ">> text3 -- not a signature block\n"
      ">> text3\n"
      ">>> text4\n"
      "> -- \n"
      "> Signature Block 4\n"
      "> Signature Block 4\n"
      ">>-------------\n"
      ">>-- text5 --\n"
      ">>-------------------\n"
      ">>-- \n"
      ">>\n"
      ">> Signature Block 5\n"
      "text6\n"
      "-- \n"
      "Signature Block 6\n";

  const QString test1Result =
      "text1\n"
      "> text2\n"
      ">> text3 -- not a signature block\n"
      ">> text3\n"
      ">>> text4\n"
      ">>-------------\n"
      ">>-- text5 --\n"
      ">>-------------------\n"
      "text6\n";

  QCOMPARE( StringUtil::stripSignature( test1, false ), test1Result );


  const QString test2 =
      "text1\n"
      "> text2\n"
      ">> text3 -- not a signature block\n"
      ">> text3\n"
      ">>> text4\n"
      ">>-------------\n"
      ">>-- text5 --\n"
      ">>-------------------\n"
      "text6\n";

  // No actual signature - should stay the same
  QCOMPARE( StringUtil::stripSignature( test2, false ), test2 );

  const QString test3 =
      "text1\n"
      "-- \n"
      "Signature Block1\n"
      "Signature Block1\n\n"
      ">text2\n"
      ">-- \n"
      ">Signature Block 2\n"
      ">Signature Block 2\n"
      "> >text3\n"
      "> >text3\n"
      "> >-- \n"
      ">>Not Signature Block 3\n"
      "> > Not Signature Block 3\n"
      ">text4\n"
      ">-- \n"
      ">Signature Block 4\n"
      ">Signature Block 4\n"
      "text5\n"
      "-- \n"
      "Signature Block 5";

  const QString test3Result =
      "text1\n"
      ">text2\n"
      "> >text3\n"
      "> >text3\n"
      ">>Not Signature Block 3\n"
      "> > Not Signature Block 3\n"
      ">text4\n"
      "text5\n";

  QCOMPARE( StringUtil::stripSignature( test3, false ), test3Result );

  const QString test4 =
      "Text 1\n"
      "-- \n"
      "First sign\n\n\n"
      "> From: bla\n"
      "> Texto 2\n\n"
      "> Aqui algo de texto.\n\n"
      ">> --\n"
      ">> Not Signature Block 2\n\n"
      "> Adios\n\n"
      ">> Texto 3\n\n"
      ">> --\n"
      ">> Not Signature block 3\n";

  const QString test4Result =
      "Text 1\n"
      "> From: bla\n"
      "> Texto 2\n\n"
      "> Aqui algo de texto.\n\n"
      ">> --\n"
      ">> Not Signature Block 2\n\n"
      "> Adios\n\n"
      ">> Texto 3\n\n"
      ">> --\n"
      ">> Not Signature block 3\n";

  QCOMPARE( StringUtil::stripSignature( test4, false ), test4Result );

  const QString test5 =
      "-- \n"
      "-- ACME, Inc\n"
      "-- Joe User\n"
      "-- PHB\n"
      "-- Tel.: 555 1234\n"
      "--";

  QCOMPARE( StringUtil::stripSignature( test5, false ), QString() );

  const QString test6 =
      "Text 1\n\n\n\n"
      "> From: bla\n"
      "> Texto 2\n\n"
      "> Aqui algo de texto.\n\n"
      ">> Not Signature Block 2\n\n"
      "> Adios\n\n"
      ">> Texto 3\n\n"
      ">> --\n"
      ">> Not Signature block 3\n";

  // Again, no actual signature in here
  QCOMPARE( StringUtil::stripSignature( test6, false ), test6 );
}

void StringUtilTest::test_isCryptoPart()
{
  QVERIFY( StringUtil::isCryptoPart( "application", "pgp-encrypted", QString() ) );
  QVERIFY( StringUtil::isCryptoPart( "application", "pgp-signature", QString() ) );
  QVERIFY( StringUtil::isCryptoPart( "application", "pkcs7-mime", QString() ) );
  QVERIFY( StringUtil::isCryptoPart( "application", "pkcs7-signature", QString() ) );
  QVERIFY( StringUtil::isCryptoPart( "application", "x-pkcs7-signature", QString() ) );
  QVERIFY( StringUtil::isCryptoPart( "application", "octet-stream", "msg.asc" ) );
  QVERIFY( !StringUtil::isCryptoPart( "application", "octet-stream", "bla.foo" ) );
  QVERIFY( !StringUtil::isCryptoPart( "application", "foo", QString() ) );
  QVERIFY( !StringUtil::isCryptoPart( "application", "foo", "msg.asc" ) );
}
