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
  private:
    QString normalizeString( const QString &input )
    {
      QString output( input );
      output.replace( "\n", " " );
      output.replace( "\r", " " );
      while ( output.contains( "  " ) )
        output.replace( "  ", " " );
      output.replace( QRegExp( "<nepomuk:[^>]*>" ), "<nepomuk:random-id>" );
      return output.trimmed();
    }

  private slots:
    void testLoadAndToSparql_data()
    {
      QTest::addColumn<QString>( "configFile" );
      QTest::addColumn<QString>( "sparqlFile" );
      QTest::addColumn<QString>( "entry" );

      const QDir dir( KDESRCDIR "/searchpatterns" );
      foreach( QString entry, dir.entryList( QStringList( "*.config" ), QDir::Files | QDir::Readable ) ) {
        const QString configFile = dir.absolutePath() + QDir::separator() + entry;
        entry.chop( 7 );
        const QString sparqlFile = dir.absolutePath() + QDir::separator() + entry + ".sparql";
        QTest::newRow( entry.toUtf8() ) << configFile << sparqlFile << entry;
      }
    }

    void testLoadAndToSparql()
    {
      QFETCH( QString, configFile );
      QFETCH( QString, sparqlFile );
      QFETCH( QString, entry );

      QVERIFY( QFile::exists( configFile ) );
      QVERIFY( QFile::exists( sparqlFile ) );

      KConfig config( configFile );
      KConfigGroup cfgGroup( &config, "Filter #0" );

      KMSearchPattern pattern( cfgGroup );
      qDebug() << pattern.asString();

      QFile file( sparqlFile );
      QVERIFY( file.open( QIODevice::ReadOnly ) );
      const QString expectedSparql = normalizeString( QString::fromUtf8( file.readAll() ) );
      const QString actualSparql = normalizeString( pattern.asSparqlQuery() );

      QStringList blackListenEntries;
      blackListenEntries << "all-of-tags";
      blackListenEntries << "has-tag";
      blackListenEntries << "not-watched";
      blackListenEntries << "any-of-tags";

      if ( blackListenEntries.contains( entry.toLower() ) )
        QEXPECT_FAIL( entry.toAscii(), "Test needs running Nepomuk server in unit test env", Continue );
      QCOMPARE( actualSparql, expectedSparql );
    }
};

QTEST_KDEMAIN( SearchPatternTest, NoGUI )

#include "searchpatterntest.moc"
