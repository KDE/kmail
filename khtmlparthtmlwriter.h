/*  -*- c++ -*-
    khtmlparthtmlwriter.h

    KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef __KMAIL_KHTMLPARTHTMLWRITER_H__
#define __KMAIL_KHTMLPARTHTMLWRITER_H__

#include <interfaces/htmlwriter.h>
#include <qobject.h>

#include <qstringlist.h>
#include <qtimer.h>

class KMReaderWin;
class QString;

namespace KMail {

  class KHtmlPartHtmlWriter : public QObject, public HtmlWriter {
    Q_OBJECT
  public:
    KHtmlPartHtmlWriter( KMReaderWin * readerWin,
			 QObject * parent=0, const char * name = 0 );
    virtual ~KHtmlPartHtmlWriter();

    void begin();
    void end();
    void reset();
    //void escapeAndWrite( const QString & str );
    void write( const QString & str );
    //void escapeAndQueue( const QString & str );
    void queue( const QString & str );
    void flush();

  protected:
    //QString escapeHTML( const QString & str );

  private:
    void queueHtml( const QString & str );

  private slots:
    void slotWriteNextHtmlChunk();

  private:
    KMReaderWin * mReaderWin;
    QStringList mHtmlQueue;
    QTimer mHtmlTimer;
  };

}; // namespace KMail

#endif // __KMAIL_KHTMLPARTHTMLWRITER_H__
