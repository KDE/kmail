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
#include "htmlquotecolorertest.h"

#include "htmlquotecolorer.h"

#include "qtest_kde.h"

using namespace KMail;

QTEST_KDEMAIN( HTMLQuoteColorerTester, GUI )

void HTMLQuoteColorerTester::test_QuoteColor()
{
  QFETCH( QString, originalCode );
  QFETCH( QString, expectedCode );

  HTMLQuoteColorer colorer;
  colorer.setQuoteColor( 0, QColor( "#FF0000" ) );
  colorer.setQuoteColor( 1, QColor( "#00FF00" ) );
  colorer.setQuoteColor( 2, QColor( "#0000FF" ) );
  QCOMPARE( colorer.process( originalCode ), expectedCode );
}

void HTMLQuoteColorerTester::test_QuoteColor_data()
{
  QTest::addColumn<QString>( "originalCode" );
  QTest::addColumn<QString>( "expectedCode" );

  QTest::newRow( "" ) << "<html><body>Some unquoted text.</body></html>"
                      << "<html><body>Some unquoted text.</body></html>";
  QTest::newRow( "" ) << "<html><body>Some unquoted &gt;text.</body></html>"
                      << "<html><body>Some unquoted &gt;text.</body></html>";
  QTest::newRow( "" ) << "<html><body>Some unquoted \n&gt;text.</body></html>"
                      << "<html><body>Some unquoted \n&gt;text.</body></html>";
  QTest::newRow( "" ) << "<html><body>Some unquoted <b>&gt;</b>text.</body></html>"
                      << "<html><body>Some unquoted <b>&gt;</b>text.</body></html>";
  QTest::newRow( "" ) << "<html><body>&gt;Some quoted text.</body></html>"
                      << "<html><body><font color=\"#ff0000\">&gt;Some quoted text.</font></body></html>";
  QTest::newRow( "" ) << "<html><body><b>&gt;Some quoted</b> text.</body></html>"
                      << "<html><body><b><font color=\"#ff0000\">&gt;Some quoted</font></b><font color=\"#ff0000\"> text.</font></body></html>";
  QTest::newRow( "" ) << "<html><body>&gt;Some quoted &gt;text.</body></html>"
                      << "<html><body><font color=\"#ff0000\">&gt;Some quoted &gt;text.</font></body></html>";
  QTest::newRow( "" ) << "<html><body>&gt;Some <b>quoted</b> text.</body></html>"
                      << "<html><body><font color=\"#ff0000\">&gt;Some </font><b><font color=\"#ff0000\">quoted</font></b><font color=\"#ff0000\"> text.</font></body></html>";
  QTest::newRow( "" ) << "<html><body>"
                         "&gt;&gt;&gt;Level 3 Quote<br>"
                         "No Quote<br>"
                         "&gt;&gt;Level 2 Quote<br>"
                         "&gt;Level 1 Quote<br>"
                         "No Quote<br>"
                         "</body></html>"
                      << "<html><body>"
                         "<font color=\"#0000ff\">&gt;&gt;&gt;Level 3 Quote</font><br>"
                         "No Quote<br>"
                         "<font color=\"#00ff00\">&gt;&gt;Level 2 Quote</font><br>"
                         "<font color=\"#ff0000\">&gt;Level 1 Quote</font><br>"
                         "No Quote<br>"
                         "</body></html>";
}
