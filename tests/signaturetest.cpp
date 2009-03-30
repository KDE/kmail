/* Copyright 2009 Thomas McGuire <mcguire@kde.org>

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
#include "signaturetest.h"

#include "stringutil.h"

#include <KDebug>
#include "qtest_kde.h"

using namespace KMail;

QTEST_KDEMAIN_CORE( SignatureTester )

void SignatureTester::test_signatureStripping()
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