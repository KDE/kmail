/** -*- c++ -*-
 * imapaccountbase.h
 *
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 * Copyright (c) 2002 Marc Mutz <mutz@kde.org>
 *
 * This file is based on work on pop3 and imap account implementations
 * by Don Sanders <sanders@kde.org> and Michael Haeckel <haeckel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __KMAIL_IMAPACCOUNTBASE_H__
#define __KMAIL_IMAPACCOUNTBASE_H__

#include "networkaccount.h"

#include <qtimer.h>

class KMAcctMgr;
class KMFolder;
class KConfig/*Base*/;

namespace KMail {

  class ImapAccountBase : public KMail::NetworkAccount {
    Q_OBJECT
  protected:
    ImapAccountBase( KMAcctMgr * parent, const QString & name );
  public:
    typedef KMail::NetworkAccount base;

    virtual ~ImapAccountBase();

    /** Set the config options to a decent state */
    virtual void init();

    /** A weak assignment operator */
    virtual void pseudoAssign( const KMAccount * a );


    /** IMAP folder prefix */
    QString prefix() const { return mPrefix; }
    virtual void setPrefix( const QString & prefix );

    /** @return whether to automatically expunge deleted messages when
        leaving the folder */
    bool autoExpunge() const { return mAutoExpunge; }
    virtual void setAutoExpunge( bool expunge );

    /** @return whether to show hidden files on the server */
    bool hiddenFolders() const { return mHiddenFolders; }
    virtual void setHiddenFolders( bool show );

    /** @return whether to show only subscribed folders */
    bool onlySubscribedFolders() const { return mOnlySubscribedFolders; }
    virtual void setOnlySubscribedFolders( bool show );

    /** Configure the slave by adding to the meta data map */
    virtual KIO::MetaData slaveConfig() const;

    virtual void readConfig( /*const*/ KConfig/*Base*/ & config );
    virtual void writeConfig( KConfig/*Base*/ & config ) /*const*/;

    /**
     * Connect to the server, if no connection is active
     */
    virtual bool makeConnection();


  protected slots:
    /**
     * new-mail-notification for not-selected folders (is called via
     * numUnreadMsgsChanged)
     */
    virtual void postProcessNewMail( KMFolder * );


  protected:
    virtual QString protocol() const;
    virtual unsigned short int defaultPort() const;
    // ### Hacks
    virtual void setPrefixHook() = 0;

  protected:
    QTimer mIdleTimer;
    QString mPrefix;
    int mTotal, mCountUnread, mCountLastUnread, mCountRemainChecks;
    bool mAutoExpunge : 1;
    bool mHiddenFolders : 1;
    bool mOnlySubscribedFolders : 1;
    bool mProgressEnabled : 1;

    bool mIdle : 1;
    bool mErrorDialogIsActive : 1;


  };


}; // namespace KMail

#endif // __KMAIL_IMAPACCOUNTBASE_H__
