/*
    Copyright (c) 2010 Vishesh Handa <handa.vish@gmail.com>

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


#ifndef SPARQLSYNTAXHIGHLIGHTER_H
#define SPARQLSYNTAXHIGHLIGHTER_H

#include <QtCore/QStringList>
#include <QtCore/QRegExp>
#include <QtGui/QSyntaxHighlighter>

namespace Nepomuk2 {

class SparqlSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    SparqlSyntaxHighlighter(QTextDocument* parent);

    void highlightBlock(const QString& text);

private:
    void init();

    struct Rule {
        QRegExp pattern;
        QTextCharFormat format;

        Rule( const QRegExp & r, const QTextCharFormat & f )
            : pattern(r), format(f) {}
    };
    QList<Rule> m_rules;
};
}
#endif // SPARQLSYNTAXHIGHLIGHTER_H
