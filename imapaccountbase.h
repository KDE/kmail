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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __KMAIL_IMAPACCOUNTBASE_H__
#define __KMAIL_IMAPACCOUNTBASE_H__

#include "networkaccount.h"

#include <qtimer.h>
#include <kio/global.h>

class KMAcctMgr;
class KMFolder;
class KConfig/*Base*/;
class KMessage;
class KMMessagePart;
class DwBodyPart;
class DwMessage;

namespace KIO {
  class Job;
}

namespace KMail {

class AttachmentStrategy;

  class ImapAccountBase : public KMail::NetworkAccount {
    Q_OBJECT
  protected:
    ImapAccountBase( KMAcctMgr * parent, const QString & name );
  public:
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

    /** @return whether to load attachments on demand */
    bool loadOnDemand() const { return mLoadOnDemand; }
    virtual void setLoadOnDemand( bool load );

    /** Configure the slave by adding to the meta data map */
    virtual KIO::MetaData slaveConfig() const;

    virtual void readConfig( /*const*/ KConfig/*Base*/ & config );
    virtual void writeConfig( KConfig/*Base*/ & config ) /*const*/;

    enum ConnectionState { Error = 0, Connected, Connecting };
    /**
     * Connect to the server, if no connection is active
     * Returns Connected (ok), Error (ko) or Connecting - which means
     * that one should wait for the slaveConnected signal from KIO::Scheduler
     * before proceeding.
     */
    ConnectionState makeConnection();

    /**
     * Info Data for the Job
     */
    struct jobData
    {
      // Needed by QMap, don't use
      jobData() : url(QString::null), parent(0), total(1), done(0), offset(0), inboxOnly(false), quiet(false) {}
      // Real constructor
      jobData( const QString& _url, KMFolder *_parent = 0,
          int _total = 1, int _done = 0, bool _quiet = false, bool _inboxOnly = false )
        : url(_url), parent(_parent), total(_total), done(_done), offset(0),
      inboxOnly(_inboxOnly), quiet(_quiet)
      {}
      // Return "url" in a form that can be displayed in HTML (w/o password)
      QString htmlURL() const;

      QString path;
      QString url;
      QByteArray data;
      QCString cdata;
      QStringList items;
      KMFolder *parent;
      QPtrList<KMMessage> msgList;
      int total, done, offset;
      bool inboxOnly, quiet, onlySubscribed;
    };

    typedef QMap<KIO::Job *, jobData>::Iterator JobIterator;
    /**
     * Call this when starting a new job
     */
    void insertJob( KIO::Job* job, const jobData& data ) {
      mapJobData.insert( job, data );
      displayProgress();
    }
    /**
     * Look for the jobData related to a given job. Compare with end()
     */
    JobIterator findJob( KIO::Job* job ) { return mapJobData.find( job ); }
    JobIterator jobsEnd() { return mapJobData.end(); }
    /**
     * Call this when a job is finished.
     * Don't use @p *it afterwards!
     */
    void removeJob( JobIterator& it ) {
      mapJobData.remove( it );
      displayProgress();
    }
    // for KMImapJob::ignoreJobsForMessage...
    void removeJob( KIO::Job* job ) {
      mapJobData.remove( job );
      displayProgress();
    }

    /**
     * Lists the directory starting from @p path
     * All parameters (onlySubscribed, secondStep, parent) are included
     * in the jobData
     * connects to slotListResult and slotListEntries
     */
    void listDirectory(QString path, bool onlySubscribed,
        bool secondStep = FALSE, KMFolder* parent = NULL);

    /** 
     * Starts the folderlisting for the root folder
     */ 
    virtual void listDirectory() = 0;

    /**
     * Subscribe (@p subscribe = TRUE) / Unsubscribe the folder
     * identified by @p imapPath
     */
    void changeSubscription(bool subscribe, QString imapPath);

    /**
     * The KIO-Slave died
     */
    void slaveDied() { mSlave = 0; killAllJobs(); }

    /**
     * Kill the slave if any jobs are active
     */
    void killAllJobs( bool disconnectSlave=false ) = 0;

	/**
	 * Init a new-mail-check for a single folder
	 */
	void processNewMailSingleFolder(KMFolder* folder);

    /**
     * Set whether the current listDirectory should create an INBOX
     */
    bool createInbox() { return mCreateInbox; }
    void setCreateInbox( bool create ) { mCreateInbox = create; }

    /**
     * Handles the result from a BODYSTRUCTURE fetch
     */ 
    void handleBodyStructure( QDataStream & stream, KMMessage * msg,
                              const AttachmentStrategy *as );

  public slots:
    /**
     * gets the results of listDirectory
     * it includes the folder-information in mSubfolderNames, -Paths and -MIMETypes
     */
    void slotListEntries(KIO::Job * job, const KIO::UDSEntryList & uds);

    /**
     * is called when listDirectory has finished
     * emits receivedFolders
     */
    void slotListResult(KIO::Job * job);

    /**
     * is called when the changeSubscription has finished
     * emits subscriptionChanged
     */
    void slotSubscriptionResult(KIO::Job * job);

    /**
     * Update the progress bar
     */
    virtual void displayProgress();

    /**
     * Display an error message
     */
    void slotSlaveError(KIO::Slave *aSlave, int, const QString &errorMsg);

  protected slots:
    /**
     * new-mail-notification for not-selected folders (is called via
     * numUnreadMsgsChanged)
     */
    virtual void postProcessNewMail( KMFolder * );
    void slotCheckQueuedFolders();

    void slotSchedulerSlaveConnected(KIO::Slave *aSlave);
    void slotSchedulerSlaveError(KIO::Slave *aSlave, int, const QString &errorMsg);

  protected:
    virtual QString protocol() const;
    virtual unsigned short int defaultPort() const;
    // ### Hacks
    virtual void setPrefixHook() = 0;

    /**
     * Build KMMessageParts and DwBodyParts from the bodystructure-stream
     */ 
    void constructParts( QDataStream & stream, int count, KMMessagePart* parentKMPart,
       DwBodyPart * parent, const DwMessage * dwmsg );

  protected:
    QPtrList<QGuardedPtr<KMFolder> > mOpenFolders;
    QStringList mSubfolderNames, mSubfolderPaths,
        mSubfolderMimeTypes;
    QMap<KIO::Job *, jobData> mapJobData;
    QTimer mIdleTimer;
    QString mPrefix;
    int mTotal, mCountUnread, mCountLastUnread, mCountRemainChecks;
    bool mAutoExpunge : 1;
    bool mHiddenFolders : 1;
    bool mOnlySubscribedFolders : 1;
    bool mLoadOnDemand : 1;
    bool mProgressEnabled : 1;

    bool mIdle : 1;
    bool mErrorDialogIsActive : 1;
    bool mPasswordDialogIsActive : 1;
	// folders that should be checked for new mails
	QValueList<QGuardedPtr<KMFolder> > mMailCheckFolders;
        // folders that should be checked after the current check is done
	QValueList<QGuardedPtr<KMFolder> > mFoldersQueuedForChecking;
    bool mCreateInbox;
    // holds messageparts from the bodystructure
    QPtrList<KMMessagePart> mBodyPartList;
    // the current message for the bodystructure
    KMMessage* mCurrentMsg;

  signals:
    /**
     * Emitted when the slave managed or failed to connect
     * This is always emitted at some point after makeConnection returned Connecting.
     * @param errorCode 0 for success, != 0 in case of error
     */
    void connectionResult( int errorCode );

    /**
     * Emitted when new folders have been received
     */
    void receivedFolders(QStringList, QStringList,
        QStringList, const ImapAccountBase::jobData &);

    /**
     * Emitted when the subscription has changed
     */
    void subscriptionChanged(QString imapPath, bool subscribed);

  };


} // namespace KMail

#endif // __KMAIL_IMAPACCOUNTBASE_H__
