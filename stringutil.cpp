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
#include "stringutil.h"

#include <QRegExp>

namespace KMail
{

namespace StringUtil
{

QString stripSignature ( const QString & msg, bool clearSigned )
{
  // Following RFC 3676, only > before --
  // I prefer to not delete a SB instead of delete good mail content.
  const QRegExp sbDelimiterSearch = clearSigned ?
      QRegExp( "(^|\n)[> ]*--\\s?\n" ) : QRegExp( "(^|\n)[> ]*-- \n" );
  // The regular expresion to look for prefix change
  const QRegExp commonReplySearch = QRegExp( "^[ ]*>" );

  QString res = msg;
  int posDeletingStart = 1; // to start looking at 0

  // While there are SB delimiters (start looking just before the deleted SB)
  while ( ( posDeletingStart = res.indexOf( sbDelimiterSearch , posDeletingStart -1 ) ) >= 0 )
  {
    QString prefix; // the current prefix
    QString line; // the line to check if is part of the SB
    int posNewLine = -1;
    int posSignatureBlock = -1;
    // Look for the SB begining
    posSignatureBlock = res.indexOf( '-', posDeletingStart );
    // The prefix before "-- "$
    if ( res[posDeletingStart] == '\n' ) ++posDeletingStart;
    prefix = res.mid( posDeletingStart, posSignatureBlock - posDeletingStart );
    posNewLine = res.indexOf( '\n', posSignatureBlock ) + 1;

    // now go to the end of the SB
    while ( posNewLine < res.size() && posNewLine > 0 )
    {
      // handle the undefined case for mid ( x , -n ) where n>1
      int nextPosNewLine = res.indexOf( '\n', posNewLine );
      if ( nextPosNewLine < 0 ) nextPosNewLine = posNewLine - 1;
      line = res.mid( posNewLine, nextPosNewLine - posNewLine );

      // check when the SB ends:
      // * does not starts with prefix or
      // * starts with prefix+(any substring of prefix)
      if ( ( prefix.isEmpty() && line.indexOf( commonReplySearch ) < 0 ) ||
             ( !prefix.isEmpty() && line.startsWith( prefix ) &&
             line.mid( prefix.size() ).indexOf( commonReplySearch ) < 0 ) )
      {
        posNewLine = res.indexOf( '\n', posNewLine ) + 1;
      }
      else
        break; // end of the SB
    }
    // remove the SB or truncate when is the last SB
    if ( posNewLine > 0 )
      res.remove( posDeletingStart, posNewLine - posDeletingStart );
    else
      res.truncate( posDeletingStart );
  }
  return res;
}

}

}
