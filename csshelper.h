/*  -*- c++ -*-
    csshelper.h

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

#ifndef __KMAIL_CSSHELPER_H__
#define __KMAIL_CSSHELPER_H__

#include "configmanager.h"

#include <qpaintdevicemetrics.h>

class QString;
class QFont;

namespace KMail {

  class CSSHelper : public ConfigManager {
  public:
    CSSHelper( const QPaintDeviceMetrics & pdm, QObject * parent=0, const char * name=0 );
    virtual ~CSSHelper();

    /** @reimplemented */
    void commit();
    /** @reimplemented */
    void rollback();
    /** @reimplemented */
    bool hasPendingChanges() const;

    /** @return HTML head including style sheet definitions and the
	&gt;body&lt; tag */
    QString htmlHead( bool fixedFont=false ) const;

    /** @return The collected CSS definitions as a string */
    QString cssDefinitions( bool fixedFont=false ) const;

    /** @return a &lt;div&gt; start tag with embedded style
	information suitable for quoted text with quote level @p level */
    QString quoteFontTag( int level ) const;
    /** @return a &lt;div&gt; start tag with embedded style
	information suitable for non-quoted text */
    QString nonQuotedFontTag() const;

    QFont bodyFont( bool fixedFont=false, bool printing=false ) const;

  private:
    class Private;
    friend class Private; // This should not be necessary, should it?
    Private * d;
    Private * s;

    const QPaintDeviceMetrics mMetrics;
  };

} // namespace KMail

#endif // __KMAIL_CSSHELPER_H__
