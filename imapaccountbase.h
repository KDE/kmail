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
template <typename T> class QValueVector;

namespace KIO {
  class Job;
}

namespace KMail {

  struct ACLListEntry;
  typedef QValueVector<KMail::ACLListEntry> ACLList;

  class AttachmentStrategy;

  class ImapAccountBase : public KMail::NetworkAccount {
    Q_OBJECT
  protected:
    ImapAccountBase( KMAcctMgr * parent, const QString & name, uint id );
  public:
    virtual ~ImapAccountBase();

    /** Set the config options to a decent state */
    virtual void init();

    /** A weak assignment operator */
    virtual void pseudoAssign( const KMAccount * a );

   /**
    * Set the account idle or busy
    */
   void setIdle(bool aIdle) { mIdle = aIdle; }

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

    virtual void readConfig( KConfig& config );
    virtual void writeConfig( KConfig& config );

    enum ConnectionState { Error = 0, Connected, Connecting };
    enum ListType { List, ListSubscribed, ListSubscribedNoCheck };
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
    void listDirectory(QString path, ListType subscription,
        bool secondStep = FALSE, KMFolder* parent = NULL, bool reset = false,
        bool complete = false);

    /**
     * Starts the folderlisting for the root folder
     */
    virtual void listDirectory() = 0;

    /**
     * Subscribe (@p subscribe = TRUE) / Unsubscribe the folder
     * identified by @p imapPath.
     * Emits subscriptionChanged signal on success.
     */
    void changeSubscription(bool subscribe, QString imapPath);

    /**
     * Retrieve the users' right on the folder
     * identified by @p folder and @p imapPath.
     * Emits receivedUserRights signal on success/error.
     */
    void getUserRights( KMFolder* folder, const QString& imapPath );

    /**
     * Retrieve the complete list of ACLs on the folder
     * identified by @p imapPath.
     * Emits receivedACL signal on success/error.
     */
    void getACL( KMFolder* folder, const QString& imapPath );

    /**
     * Set the status on the server
     * Emits imapStatusChanged signal on success/error.
     */
    void setImapStatus( KMFolder* folder, const QString& path, const QCString& flags );

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
     * Check whether we're checking for new mail
     * and the folder is included
     */
    bool checkingMail( KMFolder *folder );

    bool checkingMail() { return NetworkAccount::checkingMail(); }

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

    /**
     * Reimplemented. Additionally set the folder label
     */
    virtual void setFolder(KMFolder*, bool addAccount = false);

    /**
     * Returns false if the IMAP server for this account doesn't support ACLs.
     * (and true if it does, or if we didn't try yet).
     */
    bool hasACLSupport() const { return mACLSupport; }

    /**
     * Deprecated method for error handling. Please port to handleJobError.
     */
    void slotSlaveError(KIO::Slave *aSlave, int, const QString &errorMsg);

    /**
     * React to an error from the job. Uses job->error and job->errorString and calls
     * the protected virtual handleJobError with them. See below for details.
     */
    bool handleJobError( KIO::Job* job, const QString& context, bool abortSync = false );

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

  protected slots:
    /**
     * new-mail-notification for not-selected folders (is called via
     * numUnreadMsgsChanged)
     */
    virtual void postProcessNewMail( KMFolder * );
    void slotCheckQueuedFolders();

    /// Handle a message coming from the KIO scheduler saying that the slave is now connected
    void slotSchedulerSlaveConnected(KIO::Slave *aSlave);
    /// Handle an error coming from the KIO scheduler
    void slotSchedulerSlaveError(KIO::Slave *aSlave, int, const QString &errorMsg);

    /**
     * Only delete information about the job and ignore write errors
     */
    void slotSetStatusResult(KIO::Job * job);

    /// Result of getUserRights() job
    void slotGetUserRightsResult( KIO::Job* _job );

    /// Result of getACL() job
    void slotGetACLResult( KIO::Job* _job );

    /**
     * Send a NOOP command or log out when idle
     */
    void slotIdleTimeout();

    /**
     * Kills all jobs
     */
    void slotAbortRequested();

   /**
    * Only delete information about the job
    */
    void slotSimpleResult(KIO::Job * job);
  protected:

  /**
     * Handle an error coming from a KIO job
     * and abort everything (in all cases) if abortSync is true [this is for slotSlaveError].
     * Otherwise (abortSync==false), dimap will only abort in case of severe errors (connection broken),
     * but on "normal" errors (no permission to delete, etc.) it will ask the user.
     *
     * @param error the error code, usually job->error())
     * @param errorMsg the error message, usually job->errorText()
     * @param job the kio job (can be 0). If set, removeJob will be called automatically.
     * This is important! It means you should not call removeJob yourself in case of errors.
     * We can't let the caller do that, since it should only be done afterwards, and only if we didn't abort.
     *
     * @param context a sentence that gives some context to the error, e.g. i18n("Error while uploading message [...]")
     * @param abortSync if true, abort sync in all cases (see above). If false, ask the user (when possible).
     * @return false when aborting, true when continuing
     */
    virtual bool handleJobErrorInternal( int error, const QString &errorMsg, KIO::Job* job, const QString& context, bool abortSync = false ) = 0;

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
    bool mACLSupport : 1;

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
     * Emitted when the subscription has changed,
     * as a result of a changeSubscription call.
     */
    void subscriptionChanged(const QString& imapPath, bool subscribed);

    /**
     * Emitted upon completion of the job for setting the status for a group of UIDs,
     * as a result of a setImapStatus call.
     * On error, if the user chooses abort (not continue), cont is set to false.
     */
    void imapStatusChanged( KMFolder*, const QString& imapPath, bool cont );

    /**
     * Emitted when the get-user-rights job is done,
     * as a result of a getUserRights call.
     * Use userRights() to retrieve them, they will still be on 0 if the job failed.
     */
    void receivedUserRights( KMFolder* folder );

    /**
     * Emitted when the get-the-ACLs job is done,
     * as a result of a getACL call.
     * @param folder the folder for which we were listing the ACLs (can be 0)
     * @param job the job that was used for doing so (can be used to display errors)
     * @param entries the ACL list. Make your copy of it, it comes from the job.
     */
    void receivedACL( KMFolder* folder, KIO::Job* job, const KMail::ACLList& entries );
  };


} // namespace KMail

#endif // __KMAIL_IMAPACCOUNTBASE_H__
