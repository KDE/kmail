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

class QString;
class KHTMLPart;

namespace KMail {

  class KHtmlPartHtmlWriter : public QObject, public HtmlWriter {
    Q_OBJECT
  public:
    KHtmlPartHtmlWriter( KHTMLPart * part,
			 QObject * parent=0, const char * name = 0 );
    virtual ~KHtmlPartHtmlWriter();

    void begin();
    void end();
    void reset();
    void write( const QString & str );
    void queue( const QString & str );
    void flush();

  private slots:
    void slotWriteNextHtmlChunk();

  private:
    KHTMLPart * mHtmlPart;
    QStringList mHtmlQueue;
    QTimer mHtmlTimer;
  };

}; // namespace KMail

#endif // __KMAIL_KHTMLPARTHTMLWRITER_H__
