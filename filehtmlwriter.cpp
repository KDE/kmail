/*  -*- c++ -*-
    filehtmlwriter.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "filehtmlwriter.h"

#include <kdebug.h>


namespace KMail {

  FileHtmlWriter::FileHtmlWriter( const QString & filename )
    : HtmlWriter(),
      mFile( filename.isEmpty() ? QString( "filehtmlwriter.out" ) : filename )
  {
    mStream.setEncoding( QTextStream::UnicodeUTF8 );
  }

  FileHtmlWriter::~FileHtmlWriter() {
    if ( mFile.isOpen() ) {
      kdWarning( 5006 ) << "FileHtmlWriter: file still open!" << endl;
      mStream.unsetDevice();
      mFile.close();
    }
  }

  void FileHtmlWriter::begin( const QString & css ) {
    openOrWarn();
    if ( !css.isEmpty() )
      write( "<!-- CSS Definitions \n" + css + "-->\n" );
  }

  void FileHtmlWriter::end() {
    flush();
    mStream.unsetDevice();
    mFile.close();
  }

  void FileHtmlWriter::reset() {
    if ( mFile.isOpen() ) {
      mStream.unsetDevice();
      mFile.close();
    }
  }

  void FileHtmlWriter::write( const QString & str ) {
    mStream << str;
    flush();
  }

  void FileHtmlWriter::queue( const QString & str ) {
    write( str );
  }

  void FileHtmlWriter::flush() {
    mFile.flush();
  }

  void FileHtmlWriter::openOrWarn() {
    if ( mFile.isOpen() ) {
      kdWarning( 5006 ) << "FileHtmlWriter: file still open!" << endl;
      mStream.unsetDevice();
      mFile.close();
    }
    if ( !mFile.open( IO_WriteOnly ) )
      kdWarning( 5006 ) << "FileHtmlWriter: Cannot open file " << mFile.name() << endl;
    else
      mStream.setDevice( &mFile );
  }
    


} // namespace KMail
