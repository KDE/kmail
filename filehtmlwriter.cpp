/*  -*- c++ -*-
    filehtmlwriter.h

    KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#include "filehtmlwriter.h"

#include <kdebug.h>

#include <qstring.h>

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

  void FileHtmlWriter::begin() {
    openOrWarn();
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
    


}; // namespace KMail
