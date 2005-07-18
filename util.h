/*******************************************************************************
**
** Filename   : util
** Created on : 03 April, 2005
** Copyright  : (c) 2005 Till Adam
** Email      : <adam@kde.org>
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   It is distributed in the hope that it will be useful, but
**   WITHOUT ANY WARRANTY; without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**   General Public License for more details.
**
**   You should have received a copy of the GNU General Public License
**   along with this program; if not, write to the Free Software
**   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
**
*******************************************************************************/
#ifndef KMAILUTIL_H
#define KMAILUTIL_H

#include <stdlib.h>
#include <qobject.h>
#include <qcstring.h>

namespace KMail
{
    /**
     * The Util namespace contains a collection of helper functions use in
     * various places.
     */
namespace Util {
    /**
     * Convert all sequences of "\r\n" (carriage return followed by a line
     * feed) to a single "\n" (line feed). The conversion happens in place.
     * Returns the length of the resulting string.
     * @param str The string to convert.
     * @param strLen The length of the string to convert.
     * @return The new length of the converted string.
     */
    size_t crlf2lf( char* str, const size_t strLen );

    
    /**
     * Convert "\n" line endings to "\r\n".
     * @param src The source string to convert.
     * @return The result string.
     */
    QCString lf2crlf( const QCString & src );

    /**
     * A LaterDeleter is intended to be used with the RAII ( Resource
     * Acquisiation is Initialization ) paradigm. When an instance of it
     * goes out of scope it deletes the associated object after calling the 
     * virtual doReleaseResources() method which subclasses can reimplement 
     * to custom resource release as needed. It can be disabled, in case the
     * deletion needs to be avoided for some reason, since going out-of-scope
     * cannot be avoided.
     */
    class LaterDeleter
    {
      public:
      LaterDeleter( QObject *o)
        :m_object( o ), m_disabled( false )
      {
      }
      virtual ~LaterDeleter()
      {
        if ( !m_disabled ) {
          doReleaseResources();
          m_object->deleteLater();
        }
      }
      void setDisabled( bool v )
      {
        m_disabled = v;
      }
      protected:
      virtual void doReleaseResources() {}
      
      protected:
      QObject *m_object;
      bool m_disabled;
    };
}
}

#endif
