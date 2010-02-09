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
#include <akonadi/collection.h>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

namespace MessageViewer {
  class ObjectTreeParser;
}

class QString;
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
    TemplateParser( const KMime::Message::Ptr &amsg, const Mode amode, const QString &aselection,
                    bool aSmartQuote, bool aallowDecryption );
    ~TemplateParser();

    virtual void process( const KMime::Message::Ptr &aorig_msg,
                          const Akonadi::Collection& afolder = Akonadi::Collection(),
                          bool append = false );
    virtual void process( const QString &tmplName, const KMime::Message::Ptr &aorig_msg,
                         const Akonadi::Collection& afolder = Akonadi::Collection(), bool append = false );
    virtual void processWithIdentity( uint uoid, const KMime::Message::Ptr &aorig_msg,const Akonadi::Collection& afolder = Akonadi::Collection(),
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
    Akonadi::Collection mFolder;
    uint mIdentity;
    KMime::Message::Ptr mMsg;
    KMime::Message::Ptr mOrigMsg;
    QString mSelection;
    bool mSmartQuote;
    bool mAllowDecryption;
    bool mDebug;
    QString mQuoteString;
    bool mAppend;
    QString mTo, mCC;
    KMime::Content *mOrigRoot;

    /**
     * If there was a text selection set in the constructor, that will be returned.
     * Otherwise, returns the plain text of the original message, as in KMMessage::asPlainText().
     * The only difference is that this uses the cached object tree from parsedObjectTree()
     *
     * @param allowSelectionOnly if false, it will always return the complete mail text
     */
    QString messageText( bool allowSelectionOnly );

    /**
     * Returns the parsed object tree of the original message.
     * The result is cached in mOrigRoot, therefore calling this multiple times will only parse
     * the tree once.
     */
    KMime::Content* parsedObjectTree();

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

  /** Returns a decoded body part string to be further processed
    by function asQuotedString().
    THIS FUNCTION WILL BE REPLACED ONCE KMime IS FULLY INTEGRATED
    (khz, June 05 2002)*/ //TODO Review if we can get rid of it
    void parseTextStringFromContent( KMime::Content * root,
                                     QByteArray& parsedString,
                                     const QTextCodec*& codec,
                                     bool& isHTML ) const;

    /**
      * Returns message body indented by the
      * given indentation string. This is suitable for including the message
      * in another message of for replies, forwards.
      *
      * No attachments are handled if includeAttach is false.
      * The signature is stripped if aStripSignature is true and
      * smart quoting is turned on. Signed or encrypted texts
      * get converted to plain text when allowDecryption is true.
    */
    QString asQuotedString( const KMime::Message::Ptr &msg,
                            const QString &indentStr,
                            const QString & election=QString(),
                            bool aStripSignature=true,
                            bool allowDecryption=true);

    /** Return the textual content of the message as plain text,
        converting HTML to plain text if necessary. */
    QString asPlainText( const KMime::Message::Ptr &msg, bool stripSignature, bool allowDecryption );

    /**
    * Same as asPlainText(), only that this method expects an already parsed object tree as
    * parameter.
    * By passing an already parsed objecttree, this allows to share the objecttree and therefore
    * reduce the amount of parsing (which can include decrypting, which can include a passphrase dialog)
    */
    QString asPlainTextFromObjectTree( const KMime::Message::Ptr &msg, KMime::Content *root,
                                       MessageViewer::ObjectTreeParser *otp, bool stripSignature,
                                       bool allowDecryption );

};

} // namespace KMail

#endif // __KMAIL_TEMPLATEPARSER_H__
