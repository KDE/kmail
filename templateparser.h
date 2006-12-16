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
    TemplateParser( KMMessage *amsg, const Mode amode, const QString aselection,
                    bool aSmartQuote, bool anoQuote, bool aallowDecryption,
                    bool aselectionIsBody );

    virtual void process( KMMessage *aorig_msg, KMFolder *afolder = NULL, bool append = false );
    virtual QString findTemplate();
    virtual QString pipe( const QString cmd, const QString buf );

    virtual QString getFName( const QString str );
    virtual QString getLName( const QString str );

  protected:
    Mode mode;
    KMFolder *folder;
    uint identity;
    KMMessage *msg;
    KMMessage *orig_msg;
    QString selection;
    bool smartQuote;
    bool noQuote;
    bool allowDecryption;
    bool selectionIsBody;
    int pipe_rc;
    QString pipe_out;
    QString pipe_err;
    bool debug;
    QString quoteString;

    int parseQuotes( const QString prefix, const QString str,
                     QString &quote ) const;

  protected slots:
    void onProcessExited( KProcess *proc );
    void onReceivedStdout( KProcess *proc, char *buffer, int buflen );
    void onReceivedStderr( KProcess *proc, char *buffer, int buflen );
    void onWroteStdin( KProcess *proc );
};

#endif // __KMAIL_TEMPLATEPARSER_H__
