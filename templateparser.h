/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   Copyright (C) 2006 Dmitry Morozhnikov <dmiceman@mail.ru>
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

    virtual void process( KMMessage *aorig_msg, KMFolder *afolder = 0, bool append = false );
    virtual void process( const QString &tmplName, KMMessage *aorig_msg,
                          KMFolder *afolder = 0, bool append = false );
    virtual void processWithIdentity( uint uoid, KMMessage *aorig_msg, KMFolder *afolder = 0,
                          bool append = false );
    virtual void processWithTemplate( const QString &tmpl );

    /// This finds the template to use. Either the one from the folder, identity or
    /// finally the global template.
    /// This also reads the To and CC address of the template
    /// @return the contents of the template
    virtual QString findTemplate();

    /// Finds the template with the given name.
    /// This also reads the To and CC address of the template
    /// @return the contents of the template
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
    QString mTo, mCC;

    /**
     * Called by processWithTemplate(). This adds the completely processed body to
     * the message.
     *
     * In append mode, this will simply append the text to the body.
     *
     * Otherwise, the content of the old message is deleted and replaced with @p body.
     * Attachments of the original message are also added back to the new message. 
     */
    void addProcessedBodyToMessage( const QString &body );

    int parseQuotes( const QString &prefix, const QString &str,
                     QString &quote ) const;

  private:
    /**
     * Return the text signature used the by current identity.
     */
    QString getSignature() const;
};

} // namespace KMail

#endif // __KMAIL_TEMPLATEPARSER_H__
