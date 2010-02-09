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
class KProcess;

/**
 * The TemplateParser transforms a message with a given template.
 *
 * A template contains text and commands, such as %QUOTE or %ODATE, which will be
 * replaced with the real values in process().
 *
 * The message given in the constructor is the message that is being transformed.
 * The message text will be replaced by the processed text of the template, but other
 * properties, such as the attachments or the subject, are preserved.
 *
 * There are two different kind of commands: Those that work on the message that is
 * to be transformed and those that work on an 'original message'.
 * Those that work on the message that is to be transformed have no special prefix, e.g.
 * '%DATE'. Those that work on the original message have an 'O' prefix, for example
 * '%ODATE'.
 * This means that the %DATE command will take the date of the message passed in the
 * constructor, the message which is to be transformed, whereas the %ODATE command will
 * take the date of the message that is being passed in process(), the original message.
 *
 * TODO: What is the usecase of the commands that work on the message to be transformed?
 *       In general you only use the commands that work on the original message...
 */
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

    static const int PipeTimeout = 15;

  public:
    TemplateParser( KMMessage *amsg, const Mode amode );
    ~TemplateParser();

    /**
     * Sets the selection. If this is set, only the selection will be added to commands such
     * as %QUOTE. Otherwise, the whole message is quoted.
     * If this is not called at all, the whole message is quoted as well.
     * Call this before calling process().
     */
    void setSelection( const QString &selection );

    /**
     * Sets whether the template parser is allowed to decrypt the original message when needing
     * its message text, for example for the %QUOTE command.
     * If true, it will tell the ObjectTreeParser it uses internally to decrypt the message,
     * and that will possibly show a password request dialog to the user.
     *
     * The default is false.
     */
    void setAllowDecryption( const bool allowDecryption );

    virtual void process( KMMessage *aorig_msg, KMFolder *afolder = 0, bool append = false );
    virtual void process( const QString &tmplName, KMMessage *aorig_msg,
                          KMFolder *afolder = 0, bool append = false );
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
    bool mAllowDecryption;
    int mPipeRc;
    QString mPipeOut;
    QString mPipeErr;
    bool mDebug;
    QString mQuoteString;
    bool mAppend;
    QString mTo, mCC;
    partNode *mOrigRoot;

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
    partNode* parsedObjectTree();

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

    /**
     * Determines whether the signature should be stripped when getting the text of the original
     * message, e.g. for commands such as %QUOTE
     */
    bool shouldStripSignature() const;

    int parseQuotes( const QString &prefix, const QString &str,
                     QString &quote ) const;

  protected slots:
    void onProcessExited( KProcess *proc );
    void onReceivedStdout( KProcess *proc, char *buffer, int buflen );
    void onReceivedStderr( KProcess *proc, char *buffer, int buflen );
    void onWroteStdin( KProcess *proc );
};

#endif // __KMAIL_TEMPLATEPARSER_H__
