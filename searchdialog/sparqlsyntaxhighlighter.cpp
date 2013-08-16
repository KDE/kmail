/*
    Copyright (C) 2010 Vishesh Handa <handa.vish@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "sparqlsyntaxhighlighter.h"
#include <KDebug>

Nepomuk2::SparqlSyntaxHighlighter::SparqlSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    init();
}

void Nepomuk2::SparqlSyntaxHighlighter::init()
{
    // Keywords
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground( Qt::darkMagenta );
    keywordFormat.setFontWeight( QFont::Bold );
    QStringList keywords;

    //FIXME: Separate special inbuilt keywords
    keywords << QLatin1String( "\\bprefix\\b" ) << QLatin1String( "\\bselect\\b" ) << QLatin1String( "\\bdistinct\\b" ) << QLatin1String( "\\breduced\\b" )
             << QLatin1String( "\\bconstruct\\b" ) << QLatin1String( "\\bdescribe\\b" ) << QLatin1String( "\\bask\\b" ) << QLatin1String( "\\bfrom\\b" )
             << QLatin1String( "\\bnamed\\b" ) << QLatin1String( "\\bwhere\\b" ) << QLatin1String( "\\border\\b" ) << QLatin1String( "\\bby\\b" ) << QLatin1String( "\\basc\\b" )
             << QLatin1String( "\\bdesc\\b" ) << QLatin1String( "\\blimit\\b" ) << QLatin1String( "\\boffset\\b" ) << QLatin1String( "\\boptional\\b" )
             << QLatin1String( "\\bgraph\\b" ) << QLatin1String( "\\bunion\\b" ) << QLatin1String( "\\bfilter\\b" ) << QLatin1String( "\\bstr\\b" )
             << QLatin1String( "\\blang\\b" ) << QLatin1String( "\\blangmatches\\b" ) << QLatin1String( "\\bdatatype\\b" ) << QLatin1String( "\\bbound\\b" )
             << QLatin1String( "\\bsameTerm\\b" ) << QLatin1String( "\\bisIRI\\b" ) << QLatin1String( "\\bisURI\\b" ) << QLatin1String( "\\bisLiteral\\b" )
             << QLatin1String( "\\bisBlank\\b" ) << QLatin1String( "\\bregex\\b" ) << QLatin1String( "\\btrue\\b" ) << QLatin1String( "\\bfalse\\b" ) << QLatin1String( "\\ba\\b" );

    foreach( const QString & s, keywords ) {
        QRegExp regex( s, Qt::CaseInsensitive );
        m_rules.append( Rule( regex, keywordFormat ) );
    }

    // Variables
    QTextCharFormat varFormat;
    varFormat.setForeground( Qt::blue );
    QRegExp varRegex( QLatin1String( "\\?\\w+" ) );
    m_rules.append( Rule( varRegex, varFormat ) );

    // URI
    QTextCharFormat uriFormat;
    uriFormat.setForeground( Qt::darkGreen );
    QRegExp uriRegex( QLatin1String( "<[^ >]*>" ) );
    m_rules.append( Rule( uriRegex, uriFormat ) );

    // Abbreviated uris --> uri:word
    //TODO: Highlight uri and word with different colours
    QTextCharFormat abrUriFormat;
    abrUriFormat.setForeground( Qt::darkGray );
    QRegExp abrUriRegex( QLatin1String( "\\b\\w+:\\w*\\b" ) );
    m_rules.append( Rule( abrUriRegex, abrUriFormat ) );

    // Literals
    QTextCharFormat literalFormat;
    literalFormat.setForeground( Qt::red );
    QRegExp literalRegex( QLatin1String( "(\"[^\"]*\")|(\'[^\']*\')" ) );
    m_rules.append( Rule( literalRegex, literalFormat ) );

    // Comments
    QTextCharFormat commentFormat;
    commentFormat.setForeground( Qt::darkYellow );
    QRegExp commentRegex( QLatin1String( "^#.*$" ) );
    m_rules.append( Rule( commentRegex, commentFormat ) );
}

void Nepomuk2::SparqlSyntaxHighlighter::highlightBlock(const QString& text)
{
    foreach (const Rule &rule, m_rules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        int length = 0;
        while (index >= 0 && ( length = expression.matchedLength() ) > 0 ) {
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
}

#include "sparqlsyntaxhighlighter.moc"
