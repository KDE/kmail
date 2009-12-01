/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/


#include <qtest_kde.h>

#include "kmsearchpattern.cpp"
#include "filterlog.cpp"

#include <QDir>

class SearchPatternTest : public QObject
{
  Q_OBJECT
  private slots:
    void testLoadAndToSparql_data()
    {
      QTest::addColumn<QString>( "configFile" );
      QTest::addColumn<QString>( "sparqlFile" );

      const QDir dir( KDESRCDIR "/searchpatterns" );
      foreach( QString entry, dir.entryList( QStringList( "*.config" ), QDir::Files | QDir::Readable ) ) {
        const QString configFile = dir.absolutePath() + QDir::separator() + entry;
        entry.chop( 7 );
        const QString sparqlFile = dir.absolutePath() + QDir::separator() + entry + ".sparql";
        QTest::newRow( entry.toUtf8() ) << configFile << sparqlFile;
      }
    }

    void testLoadAndToSparql()
    {
      QFETCH( QString, configFile );
      QFETCH( QString, sparqlFile );

      QVERIFY( QFile::exists( configFile ) );
      QVERIFY( QFile::exists( sparqlFile ) );

      KConfig config( configFile );
      KConfigGroup cfgGroup( &config, "Filter #0" );

      KMSearchPattern pattern( cfgGroup );
      qDebug() << pattern.asString();

      QFile file( sparqlFile );
      QVERIFY( file.open( QIODevice::ReadOnly ) );
      QString expectedSparql = QString::fromUtf8( file.readAll() );
      // normalize whitespaces, so we can have more readable queries in the reference files
      expectedSparql.replace( "\n", " " );
      expectedSparql.replace( "\r", " " );
      while ( expectedSparql.contains( "  " ) )
        expectedSparql.replace( "  ", " " );

      QCOMPARE( pattern.asSparqlQuery().trimmed(), expectedSparql.trimmed() );
    }
};

QTEST_KDEMAIN( SearchPatternTest, NoGUI )

#include "searchpatterntest.moc"
