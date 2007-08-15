/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2006 Dmitry Morozhnikov <dmiceman@mail.ru>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef __KMAIL_TEMPLATEPARSER_H__
#define __KMAIL_TEMPLATEPARSER_H__

#include <qobject.h>

class KMMessage;
class QString;
class KMFolder;
class QObject;

namespace KMail {

class TemplateParser : public QObject
{
  Q_OBJECT

  public:
    enum Mode {
      NewMessage,
      Reply,
      ReplyAll,
      Forward
    };

  public:
    TemplateParser( KMMessage *amsg, const Mode amode, const QString &aselection,
                    bool aSmartQuote, bool aallowDecryption,
                    bool aselectionIsBody );

    virtual void process( KMMessage *aorig_msg, KMFolder *afolder = NULL, bool append = false );
    virtual void process( const QString &tmplName, KMMessage *aorig_msg,
                          KMFolder *afolder = NULL, bool append = false );
    virtual void processWithTemplate( const QString &tmpl );
    virtual QString findTemplate();
    virtual QString findCustomTemplate( const QString &tmpl );
    virtual QString pipe( const QString &cmd, const QString &buf );

    virtual QString getFName( const QString &str );
    virtual QString getLName( const QString &str );

  protected:
    Mode mMode;
    KMFolder *mFolder;
    uint mIdentity;
    KMMessage *mMsg;
    KMMessage *mOrigMsg;
    QString mSelection;
    bool mSmartQuote;
    bool mAllowDecryption;
    bool mSelectionIsBody;
    bool mDebug;
    QString mQuoteString;
    bool mAppend;

    int parseQuotes( const QString &prefix, const QString &str,
                     QString &quote ) const;
};

} // namespace KMail

#endif // __KMAIL_TEMPLATEPARSER_H__
