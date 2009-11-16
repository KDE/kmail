/*  -*- c++ -*-
    htmlstatusbar.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Ingo Kloecker <kloecker@kde.org>
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
#ifndef _KMAIL_HTMLSTATUSBAR_H_
#define _KMAIL_HTMLSTATUSBAR_H_

#include <QLabel>

class QString;
class QColor;
class QMouseEvent;

namespace KMail {

  /**
   * @short The HTML statusbar widget for use with the reader.
   *
   * The HTML status bar is a small widget that acts as an indicator
   * for the message content. It can be in one of four modes:
   *
   * <dl>
   * <dt><code>Normal</code></dt>
   * <dd>Default. No HTML.</dd>
   * <dt><code>Html</code></dt>
   * <dd>HTML content is being shown. Since HTML mails can mimic all sorts
   *     of KMail markup in the reader, this provides out-of-band information
   *     about the presence of (rendered) HTML.</dd>
   * <dt><code>MultipartPlain</code></dt>
   * <dd>Viewed as plain text with HTML part also available.</dd>
   * <dt><code>MultipartHtml</code></dt>
   * <dd>Viewed as Html with plain text part also available.</dd>
   * </dl>
   *
   * @author Ingo Kloecker <kloecker@kde.org>, Marc Mutz <mutz@kde.org>
   **/
  class HtmlStatusBar : public QLabel {
    Q_OBJECT
  public:
    explicit HtmlStatusBar( QWidget * parent=0, const char * name=0, Qt::WFlags f=0 );
    virtual ~HtmlStatusBar();

    enum Mode {
      Normal,
      Html,
      MultipartPlain,
      MultipartHtml
    };

    /** @return current mode. */
    Mode mode() const { return mMode ; }
    bool isHtml() const { return mode() == Html ; }
    bool isNormal() const { return mode() == Normal ; }
    bool isMultipartHtml() const { return mode() == MultipartHtml; }
    bool isMultipartPlain() const { return mode() == MultipartPlain; }

    // Update the status bar, for example when the color scheme changed.
    void update();

  public slots:
    /** Switch to "html mode". */
    void setHtmlMode();
    /** Switch to "normal mode". */
    void setNormalMode();
    /** Switch to "multipart html mode". */
    void setMultipartHtmlMode();
    /** Switch to "multipart plain mode". */
    void setMultipartPlainMode();
    /** Switch to mode @p m */
    void setMode( Mode m );

  signals:
    /** The user has clicked the status bar. */
    void clicked();

  protected:
    void mousePressEvent( QMouseEvent * event );

  private:
    QString message() const;
    QString toolTip() const;
    QColor bgColor() const;
    QColor fgColor() const;

    Mode mMode;
  };

} // namespace KMail

#endif // _KMAIL_HTMLSTATUSBAR_H_
