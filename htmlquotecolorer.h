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
#ifndef _HTMLQUOTECOLERER_H
#define _HTMLQUOTECOLERER_H

#include <dom/dom_node.h>

#include <QColor>

namespace KPIM
{
  class CSSHelper;
}

namespace KMail
{

/**
 * Little helper class that takes a HTML source as input and finds all
 * lines that are quoted with '>' or '|'. The HTML text is then modified so
 * that the quoted lines appear in the defined quote colors.
 */
class HTMLQuoteColorer
{
  public:

    HTMLQuoteColorer();

    /**
     * Sets the quote color of the specific leve.
     * This class supports 3 levels, from 0 to 2.
     * Level 2 is the most quoted, with three quote signs in front of the line, and
     * level 0 is the least quoted, with one quote sign in front of the line.
     *
     * If you don't call this, the color of the quoting will be black.
     */
    void setQuoteColor( int level, const QColor &color );

    /**
     * Do the work and add nice colors to the HTML.
     * You need to call setQuoteColor() before calling this.
     * @param htmlSource the input HTML code
     * @return the modified HTML code
     */
    QString process( const QString &htmlSource );

  private:

    DOM::Node processNode( DOM::Node node );
    int quoteLength( const QString &line ) const;

    QColor mQuoteColors[3];
    bool mIsQuotedLine;
    bool mIsFirstTextNodeInLine;
    int currentQuoteLength;
};

}

#endif
