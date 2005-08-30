/*  -*- c++ -*-
    khtmlparthtmlwriter.h

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#ifndef __KMAIL_KHTMLPARTHTMLWRITER_H__
#define __KMAIL_KHTMLPARTHTMLWRITER_H__

#include "interfaces/htmlwriter.h"
#include <qobject.h>

#include <qstringlist.h>
#include <qtimer.h>

class QString;
class KHTMLPart;

namespace KMail {

  class KHtmlPartHtmlWriter : public QObject, public HtmlWriter {
    Q_OBJECT
  public:
    // Key is Content-Id, value is URL
    typedef QMap<QString, QString> EmbeddedPartMap;
    KHtmlPartHtmlWriter( KHTMLPart * part,
			 QObject * parent=0, const char * name = 0 );
    virtual ~KHtmlPartHtmlWriter();

    void begin( const QString & cssDefs );
    void end();
    void reset();
    void write( const QString & str );
    void queue( const QString & str );
    void flush();
    void embedPart( const QCString & contentId, const QString & url );

  private slots:
    void slotWriteNextHtmlChunk();

  private:
    void resolveCidUrls();

  private:
    KHTMLPart * mHtmlPart;
    QStringList mHtmlQueue;
    QTimer mHtmlTimer;
    enum State {
      Begun,
      Queued,
      Ended
    } mState;
    EmbeddedPartMap mEmbeddedPartMap;
  };

} // namespace KMail

#endif // __KMAIL_KHTMLPARTHTMLWRITER_H__
