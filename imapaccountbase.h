/* -*- c++ -*-
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __KMAIL_IMAPACCOUNTBASE_H__
#define __KMAIL_IMAPACCOUNTBASE_H__

#include <set>

#include "networkaccount.h"

#include <kio/global.h>

#include <QList>
#include <QPointer>
#include <QTimer>

#include <QByteArray>

class AccountManager;
class KMFolder;
class KConfigGroup;
class KMMessagePart;
class DwBodyPart;
class DwMessage;
class FolderStorage;
template <typename T> class QVector;

namespace KIO {
  class Job;
}

namespace KPIM {
  class ProgressItem;
}

namespace KMail {
  struct ACLListEntry;
  struct QuotaInfo;
  typedef QVector<KMail::ACLListEntry> ACLList;

  class AttachmentStrategy;

  class ImapAccountBase : public KMail::NetworkAccount {
    Q_OBJECT
  protected:
    ImapAccountBase( AccountManager * parent, const QString & name, uint id );
  public:
    virtual ~ImapAccountBase();

    /** Set the config options to a decent state */
    virtual void init();

    /** A weak assignment operator */
    virtual void pseudoAssign( const KMAccount * a );

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

    /** @return whether to show only locally subscribed folders */
    bool onlyLocallySubscribedFolders() const { return mOnlyLocallySubscribedFolders; }
    virtual void setOnlyLocallySubscribedFolders( bool show );


    /** @return whether to load attachments on demand */
    bool loadOnDemand() const { return mLoadOnDemand; }
    virtual void setLoadOnDemand( bool load );

    /** @return whether to list only open folders */
    bool listOnlyOpenFolders() const { return mListOnlyOpenFolders; }
    virtual void setListOnlyOpenFolders( bool only );

    /** Configure the slave by adding to the meta data map */
    virtual KIO::MetaData slaveConfig() const;

    virtual void readConfig( KConfigGroup& config );
    virtual void writeConfig( KConfigGroup& config );

    /**
     * The state of the kioslave connection
     */
    enum ConnectionState { Error = 0, Connected, Connecting };

    // possible list types
    enum ListType {
      List,
      ListSubscribed,
      ListSubscribedNoCheck,
      ListFolderOnly,
      ListFolderOnlySubscribed
    };

    /**
     * Connect to the server, if no connection is active
     * Returns Connected (ok), Error (ko) or Connecting - which means
     * that one should wait for the slaveConnected signal from KIO::Scheduler
     * before proceeding.
     */
    ConnectionState makeConnection();

    // namespace defines
    enum imapNamespace { PersonalNS=0, OtherUsersNS=1, SharedNS=2 };

    // map a namespace type to a list of namespaces
    typedef QMap<imapNamespace, QStringList> nsMap;

    // map a namespace to a delimiter
    typedef QMap<QString, QString> namespaceDelim;

    // map a namespace type to a map with the namespace and the delimiter
    typedef QMap<imapNamespace, namespaceDelim> nsDelimMap;

    /**
     * Info Data for the Job
     */
    struct jobData
    {
      // Needed by QMap, don't use
      jobData() : url(QString()), parent(0), total(1), done(0), offset(0), progressItem(0),
                  onlySubscribed(false), quiet(false), cancellable(false) {}
      // Real constructor
      jobData( const QString& _url, KMFolder *_parent = 0,
          int _total = 1, int _done = 0, bool _quiet = false,
          bool _cancelable = false )
        : url(_url), parent(_parent), total(_total), done(_done), offset(0),
          progressItem(0), quiet(_quiet), cancellable(_cancelable) {}

      // Return "url" in a form that can be displayed in HTML (w/o password)
      QString htmlURL() const;

      QString path;
      QString url;
      QString curNamespace;
      QByteArray data;
      QByteArray cdata;
      QStringList items;
      KMFolder *parent;
      QList<KMMessage*> msgList;
      int total, done, offset;
      KPIM::ProgressItem *progressItem;
      bool onlySubscribed, quiet, cancellable;
    };

    typedef QMap<KJob *, jobData>::Iterator JobIterator;
    /**
     * Call this when starting a new job
     */
    void insertJob( KJob* job, const jobData& data ) {
      mapJobData.insert( job, data );
    }
    /**
     * Look for the jobData related to a given job. Compare with end()
     */
    JobIterator findJob( KJob* job ) { return mapJobData.find( job ); }
    JobIterator jobsEnd() { return mapJobData.end(); }
    /**
     * Call this when a job is finished.
     * Don't use @p *it afterwards!
     */
    void removeJob( JobIterator& it );

    void removeJob( KIO::Job* job );

    /**
     * Subscribe (@p subscribe = true) / Unsubscribe the folder
     * identified by @p imapPath.
     * Emits subscriptionChanged signal on success.
     */
    void changeSubscription(bool subscribe, const QString& imapPath);

    /**
     * Returns whether the account is locally subscribed to the
     * folder @param imapPath. No relation to server side subscription above.
     */
    bool locallySubscribedTo( const QString& imapPath );

    /**
     * Locally subscribe (@p subscribe = TRUE) / Unsubscribe the folder
     * identified by @p imapPath.
     */
    void changeLocalSubscription( const QString& imapPath, bool subscribe );


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
     * Retrieve the the quota inforamiton on the folder
     * identified by @p imapPath.
     * Emits receivedQuotaInfo signal on success/error.
     */
    void getStorageQuotaInfo( KMFolder* folder, const QString& imapPath );

    /**
     * Set the status on the server
     * Emits imapStatusChanged signal on success/error.
     */
    void setImapStatus( KMFolder* folder, const QString& path, const QByteArray& flags );

    /**
     * The KIO-Slave died
     */
    void slaveDied() { mSlave = 0; killAllJobs(); }

    /**
     * Kill the slave if any jobs are active
     */
    void killAllJobs( bool disconnectSlave=false ) = 0;

    /**
     * Abort all running mail checks. Used when exiting.
     */
    virtual void cancelMailCheck();

    /**
     * Init a new-mail-check for a single folder
     */
    void processNewMailSingleFolder(KMFolder* folder);

    /**
     * Return true if we are processing a mailcheck for a single folder
     */
    bool checkingSingleFolder() { return mCheckingSingleFolder; }

    /**
     * Called when we're completely done checking mail for this account
     * When @p setStatusMsg is true a status msg is shown
     */
    void postProcessNewMail( bool setStatusMsg = true );

    /**
     * Check whether we're checking for new mail
     * and the folder is included
     */
    bool checkingMail( KMFolder *folder );

    bool checkingMail() { return NetworkAccount::checkingMail(); }

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
     * Returns false if the IMAP server for this account doesn't support annotations.
     * (and true if it does, or if we didn't try yet).
     */
    bool hasAnnotationSupport() const { return mAnnotationSupport; }

    /**
     * Called if the annotation command failed due to 'unsupported'
     */
    void setHasNoAnnotationSupport() { mAnnotationSupport = false; }

    /**
     * Returns false if the IMAP server for this account doesn't support quotas.
     * (and true if it does, or if we didn't try yet).
     */
    bool hasQuotaSupport() const { return mQuotaSupport; }

    /**
     * Called if the quota command failed due to 'unsupported'
     */
    void setHasNoQuotaSupport() { mQuotaSupport = false; }

    /**
     * React to an error from the job. Uses job->error and job->errorString and calls
     * the protected virtual handleJobError with them. See handleError below for details.
     */
    bool handleJobError( KIO::Job* job, const QString& context, bool abortSync = false );

    /**
     * Returns the root folder of this account
     */
    virtual FolderStorage* rootFolder() const = 0;

    /**
     * Progress item for listDir
     */
    KPIM::ProgressItem* listDirProgressItem();

    /**
     * @return the number of (subscribed, if applicable) folders in this
     * account.
     */
    virtual unsigned int folderCount() const;

    /**
     * @return defined namespaces
     */
    nsMap namespaces() const { return mNamespaces; }

    /**
     * Set defined namespaces
     */
    virtual void setNamespaces( nsMap map )
    { mNamespaces = map; }

    /**
     * Full blown section - namespace - delimiter map
     * Do not call this very often as the map is constructed on the fly
     */
    nsDelimMap namespacesWithDelimiter();

    /**
     * @return the namespace for the @p folder
     */
     QString namespaceForFolder( FolderStorage* );

     /**
      * Adds "/" as needed to the given namespace
      */
     QString addPathToNamespace( const QString& ns );

     /**
      * @return the delimiter for the @p namespace
      */
     QString delimiterForNamespace( const QString& prefix );

     /**
      * @return the delimiter for the @p folderstorage
      */
     QString delimiterForFolder( FolderStorage* );

     /**
      * @return the namespace - delimiter map
      */
     namespaceDelim namespaceToDelimiter() const
     { return mNamespaceToDelimiter; }

     /**
      * Set the namespace - delimiter map
      */
     void setNamespaceToDelimiter( namespaceDelim map )
     { mNamespaceToDelimiter = map; }

     /**
      * Returns true if the given string is a namespace
      */
     bool isNamespaceFolder( QString& name );

     /**
      * Returns true if the account has the given capability
      */
     bool hasCapability( const QString& capa ) {
      return mCapabilities.contains( capa ); }

     /**
      * Create an IMAP path for a parent folder and a foldername
      * Parent and folder are separated with the delimiter of the account
      * The path starts and ends with '/'
      */
     QString createImapPath( FolderStorage* parent, const QString& folderName );

     /**
      * Create an IMAP path for a parent imapPath and a folderName
      */
     QString createImapPath( const QString& parent, const QString& folderName );


  public slots:
    /**
     * Call this to get the namespaces
     * You are notified by the signal namespacesFetched
     */
    void getNamespaces();

  private slots:
    /**
     * is called when the changeSubscription has finished
     * emits subscriptionChanged
     */
    void slotSubscriptionResult(KJob * job);

  protected slots:
    virtual void slotCheckQueuedFolders();

    /// Handle a message coming from the KIO scheduler saying that the slave is now connected
    void slotSchedulerSlaveConnected(KIO::Slave *aSlave);
    /// Handle an error coming from the KIO scheduler
    void slotSchedulerSlaveError(KIO::Slave *aSlave, int, const QString &errorMsg);

    /**
     * Only delete information about the job and ignore write errors
     */
    void slotSetStatusResult(KJob * job);

    /// Result of getUserRights() job
    void slotGetUserRightsResult( KJob* _job );

    /// Result of getACL() job
    void slotGetACLResult( KJob* _job );

    /// Result of getStorageQuotaInfo() job
    void slotGetStorageQuotaInfoResult( KIO::Job* _job );

    /**
     * Send a NOOP command regularly to keep the slave from disconnecting
     */
    void slotNoopTimeout();
    /**
     * Log out when idle
     */
    void slotIdleTimeout();

    /**
     * Kills all jobs
     */
    void slotAbortRequested( KPIM::ProgressItem* );

    /**
     * Only delete information about the job
     */
    void slotSimpleResult(KJob * job);

    /** Gets and parses the namespaces */
    void slotNamespaceResult( KJob*, const QString& str,const QString & );

    /**
     * Saves the fetched namespaces
     */
    void slotSaveNamespaces( const ImapAccountBase::nsDelimMap& map );

    /**
     * Saves the capabilities list
     */
    void slotCapabilitiesResult( KJob*, const QString& result,const QString & );

  protected:

  /**
     * Handle an error coming from a KIO job or from a KIO slave (via the scheduler)
     * and abort everything (in all cases) if abortSync is true [this is for slotSchedulerSlaveError].
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
    virtual bool handleError( int error, const QString &errorMsg, KIO::Job* job, const QString& context, bool abortSync = false );

    /** Handle an error during KIO::put - helper method */
    bool handlePutError( KIO::Job* job, jobData& jd, KMFolder* folder );

    virtual QString protocol() const;
    virtual unsigned short int defaultPort() const;

    /**
     * Build KMMessageParts and DwBodyParts from the bodystructure-stream
     */
    void constructParts( QDataStream & stream, int count, KMMessagePart* parentKMPart,
       DwBodyPart * parent, const DwMessage * dwmsg );

    /** Migrate the prefix */
    void migratePrefix();

    // used for writing the blacklist out to the config file
    QStringList locallyBlacklistedFolders() const;
    void localBlacklistFromStringList( const QStringList & );

  protected:
    QList<QPointer<KMFolder>* > mOpenFolders;
    QStringList mSubfolderNames, mSubfolderPaths,
        mSubfolderMimeTypes, mSubfolderAttributes;
    QMap<KJob *, jobData> mapJobData;
    /** used to detect when the slave has not been used for a while */
    QTimer mIdleTimer;
    /** used to send a noop to the slave in regular intervals to keep it from disonnecting */
    QTimer mNoopTimer;
    int mTotal, mCountUnread, mCountLastUnread;
    QMap<QString, int> mUnreadBeforeCheck;
    bool mAutoExpunge : 1;
    bool mHiddenFolders : 1;
    bool mOnlySubscribedFolders : 1;
    bool mOnlyLocallySubscribedFolders : 1;
    bool mLoadOnDemand : 1;
    bool mListOnlyOpenFolders : 1;
    bool mProgressEnabled : 1;

    bool mErrorDialogIsActive : 1;
    bool mPasswordDialogIsActive : 1;
    bool mACLSupport : 1;
    bool mAnnotationSupport : 1;
    bool mQuotaSupport : 1;
    bool mSlaveConnected : 1;
    bool mSlaveConnectionError : 1;
    bool mCheckingSingleFolder : 1;

    // folders that should be checked for new mails
    QList<QPointer<KMFolder> > mMailCheckFolders;
    // folders that should be checked after the current check is done
    QList<QPointer<KMFolder> > mFoldersQueuedForChecking;
    // holds messageparts from the bodystructure
    QList<KMMessagePart*> mBodyPartList;
    // the current message for the bodystructure
    KMMessage* mCurrentMsg;

    QPointer<KPIM::ProgressItem> mListDirProgressItem;

    // our namespaces in the form section=namespaceList
    nsMap mNamespaces;

    // namespace - delimiter map
    namespaceDelim mNamespaceToDelimiter;

    // old prefix for migration
    QString mOldPrefix;

    // capabilities
    QStringList mCapabilities;

    std::set<QString> mLocalSubscriptionBlackList;

  signals:
    /**
     * Emitted when the slave managed or failed to connect
     * This is always emitted at some point after makeConnection returned Connecting.
     * @param errorCode 0 for success, != 0 in case of error
     * @param errorMsg if errorCode is != 0, this goes with errorCode to call KIO::buildErrorString
     */
    void connectionResult( int errorCode, const QString& errorMsg );

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

    /**
     * Emitted when the getQuotaInfo job is done,
     * as a result of a getQuotaInfo call.
     * @param folder The folder for which we were getting quota info (can be 0)
     * @param job The job that was used for doing so (can be used to display errors)
     * @param info The quota information for this folder. Make your copy of it,
     * it comes from the job.
     */
    void receivedStorageQuotaInfo( KMFolder* folder, KIO::Job* job, const KMail::QuotaInfo& entries );

    /**
     * Emitted when we got the namespaces
     */
    void namespacesFetched( const ImapAccountBase::nsDelimMap& );

    /**
     * Emitted when we got the namespaces, and they were set on the object.
     */
    void namespacesFetched();
  };


} // namespace KMail

#endif // __KMAIL_IMAPACCOUNTBASE_H__
