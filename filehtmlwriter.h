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

#ifndef __KMAIL_FILEHTMLWRITER_H__
#define __KMAIL_FILEHTMLWRITER_H__

#include "interfaces/htmlwriter.h"

#include <qfile.h>
#include <qtextstream.h>

class QString;

namespace KMail {

  class FileHtmlWriter : public KMail::HtmlWriter {
  public:
    FileHtmlWriter( const QString & filename );
    virtual ~FileHtmlWriter();

    void begin();
    void end();
    void reset();
    void write( const QString & str );
    void queue( const QString & str );
    void flush();

  private:
    void openOrWarn();

  private:
    QFile mFile;
    QTextStream mStream;
  };

}; // namespace KMail

#endif // __KMAIL_FILEHTMLWRITER_H__
