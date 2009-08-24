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
#include "htmlquotecolorer.h"

#include <dom/html_document.h>
#include <dom/dom_element.h>
#include <dom/dom_exception.h>

namespace KMail {

HTMLQuoteColorer::HTMLQuoteColorer()
{
}

QString HTMLQuoteColorer::process( const QString &htmlSource )
{
  try {
    // Create a DOM Document from the HTML source
    DOM::HTMLDocument doc;
    doc.open();
    doc.write( htmlSource );
    doc.close();

    mIsQuotedLine = false;
    mIsFirstTextNodeInLine = true;
    processNode( doc.documentElement() );
    return doc.toString().string();
  }
  catch( const DOM::DOMException &exception ) {
    kWarning() << "Got a DOM exception with code" << exception.code;
    kWarning() << "No quote coloring for you, then.";
    return htmlSource;
  }
}

DOM::Node HTMLQuoteColorer::processNode( DOM::Node node )
{
  // Below, we determine if the current text node should be quote colored by keeping track of
  // linebreaks and wether this text node is the first one.
  const QString textContent = node.textContent().string();
  const bool isTextNode = !textContent.isEmpty() && !node.hasChildNodes();
  if ( isTextNode ) {
    if ( mIsFirstTextNodeInLine ) {
      if ( textContent.simplified().startsWith( '>' ) ||
          textContent.simplified().startsWith( '|' ) ) {
        mIsQuotedLine = true;
        currentQuoteLength = quoteLength( textContent ) - 1;
      }
      else {
        mIsQuotedLine = false;
      }
    }

    // All subsequent text nodes are not the first ones anymore
    mIsFirstTextNodeInLine = false;
  }

  const QString nodeName = node.nodeName().string().toLower();
  QStringList lineBreakNodes;
  lineBreakNodes << "br" << "p" << "div" << "ul" << "ol" << "li";
  if ( lineBreakNodes.contains( nodeName ) ) {
    mIsFirstTextNodeInLine = true;
  }

  DOM::Node returnNode = node;
  bool fontTagAdded = false;
  if ( mIsQuotedLine && isTextNode ) {

    // Ok, we are in a line that should be quoted, so create a font node for the color and replace
    // the current node with it.
    DOM::Element font = node.ownerDocument().createElement( QString( "font" ) );
    font.setAttribute( QString( "color" ), mQuoteColors[ currentQuoteLength ].name() );
    node.parentNode().replaceChild( font, node );
    font.appendChild( node );
    returnNode = font;
    fontTagAdded = true;
  }

  // Process all child nodes, but only if we are didn't add those child nodes itself, as otherwise
  // we'll go into an infinite recursion.
  if ( !fontTagAdded ) {
    DOM::Node childNode = node.firstChild();
    while ( !childNode.isNull() ) {
      childNode = processNode( childNode );
      childNode = childNode.nextSibling();
    }
  }
  return returnNode;
}

int HTMLQuoteColorer::quoteLength( const QString &line ) const
{
  QString simplified = line.simplified();
  simplified = simplified.replace( QRegExp( QLatin1String( "\\s" ) ), QString() )
                         .replace( QLatin1Char( '|' ), QLatin1Char( '>' ) );
  if ( simplified.startsWith( ">>>" ) ) return 3;
  if ( simplified.startsWith( ">>" ) ) return 2;
  if ( simplified.startsWith( ">" ) ) return 1;
  return 0;
}

void HTMLQuoteColorer::setQuoteColor( int level, const QColor& color )
{
  if ( level < 3 )
    mQuoteColors[level] = color;
}

}
