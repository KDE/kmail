/* -*- mode: C++ -*-
 * KMail Account: Abstract base class for accounts.
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmaccount_h
#define kmaccount_h

#include <kprocess.h>
#include <kaccount.h>

#include <qtimer.h>
#include <qsignal.h>
#include <qstring.h>
#include <qptrlist.h>
#include <qvaluelist.h>
#include <qguardedptr.h>
#include <qmap.h>

class KMAcctMgr;
class KMFolder;
class KMAcctFolder;
class KConfig;
class KMMessage;
class KMFolderJob;
namespace  KMail {
  class FolderJob;
}
namespace KPIM { class ProgressItem; }
using KMail::FolderJob;
using KPIM::ProgressItem;

class KMPrecommand : public QObject
{
  Q_OBJECT

public:
  KMPrecommand(const QString &precommand, QObject *parent = 0);
  virtual ~KMPrecommand();
  bool start();

protected slots:
  void precommandExited(KProcess *);

signals:
  void finished(bool);

protected:
  KProcess mPrecommandProcess;
  QString mPrecommand;
};


class KMAccount: public QObject, public KAccount
{
  Q_OBJECT
  friend class KMAcctMgr;
  friend class FolderJob;
  friend class KMFolderCachedImap; /* HACK for processNewMSg() */

public:
  virtual ~KMAccount();

  enum CheckStatus { CheckOK, CheckIgnored, CheckCanceled, CheckAborted,
                     CheckError };

  /** The default check interval */
  static const int DefaultCheckInterval = 5;

  /**
   * Returns type of the account
   */
  virtual QString type() const { return QString::null; }

  /**
   * Reimplemented, set account name
   */
  virtual void setName(const QString&);

  /**
   * Account name (reimpl because of ambiguous QObject::name())
   */
  virtual QString name() const { return KAccount::name(); }

  /**
   * Set password to "" (empty string)
   */
  virtual void clearPasswd();

  /**
   * Set intelligent default values to the fields of the account.
   */
  virtual void init();

  /**
   * A weak assignment operator
   */
  virtual void pseudoAssign(const KMAccount * a );

  /**
   * There can be exactly one folder that is fed by messages from an
   * account. */
  KMFolder* folder(void) const { return ((KMFolder*)((KMAcctFolder*)mFolder)); }
  virtual void setFolder(KMFolder*, bool addAccount = false);

  /**
   * the id of the trash folder (if any) for this account
   */
  QString trash() const { return mTrash; }
  virtual void setTrash(const QString& aTrash) { mTrash = aTrash; }

  /**
   * Process new mail for this account if one arrived. Returns TRUE if new
   * mail has been found. Whether the mail is automatically loaded to
   * an associated folder or not depends on the type of the account.
   */
  virtual void processNewMail(bool interactive) = 0;

  /**
   * Read config file entries. This method is called by the account
   * manager when a new account is created. The config group is
   * already properly set by the caller.
   */
  virtual void readConfig(KConfig& config);

  /**
   * Write all account information to given config file. The config group
   * is already properly set by the caller.
   */
  virtual void writeConfig(KConfig& config);

  /**
   * Set/get interval for checking if new mail arrived (in minutes).
   * An interval of zero (or less) disables the automatic checking.
   */
  virtual void setCheckInterval(int aInterval);
  int checkInterval(void) const { return mInterval; }

  /**
   * This can be used to provide a more complex calculation later if we want
   */
  inline int defaultCheckInterval(void) const { return DefaultCheckInterval; }

  /**
   * Deletes the set of FolderJob associated with this folder.
   */
  void deleteFolderJobs();

  /**
   * delete jobs associated with this message
   */
  virtual void ignoreJobsForMessage( KMMessage* );
  /**
   * Set/get whether account should be part of the accounts checked
   * with "Check Mail".
   */
  virtual void setCheckExclude(bool aExclude);
  bool checkExclude(void) const { return mExclude; }

   /**
    * Pre command
    */
  const QString& precommand(void) const { return mPrecommand; }
  virtual void setPrecommand(const QString &cmd) { mPrecommand = cmd; }

  /**
   * Runs the precommand. If the precommand is empty, the method
   * will just return success and not actually do anything
   *
   * @return True if successful, false otherwise
   */
  bool runPrecommand(const QString &precommand);

  /**
   * Very primitive en/de-cryption so that the password is not
   * readable in the config file. But still very easy breakable.
   */
  static QString encryptStr(const QString& inStr);
  static QString decryptStr(const QString& inStr) { return  encryptStr(inStr); }

  static QString importPassword(const QString &);

  /** @return whether this account has an inbox */
  bool hasInbox() const { return mHasInbox; }
  virtual void setHasInbox( bool has ) { mHasInbox = has; }

  /**
   * If this account is a disconnected IMAP account, invalidate it.
   */
  virtual void invalidateIMAPFolders();

  /**
   * Set/Get if this account is currently checking mail
   */
  bool checkingMail() { return mCheckingMail; }
  void setCheckingMail( bool checking ) { mCheckingMail = checking; }

  /**
   * Call this if the newmail-check ended.
   * @param newMail true if new mail arrived
   * @param status the status of the mail check
   */
  void checkDone( bool newMail, CheckStatus status );

  /**
   * Abort all running mail checks. Used when closing the last KMMainWin.
   * Ensure that mail check can be restarted later, e.g. if reopening a mainwindow
   * from a composer window.
   */
  virtual void cancelMailCheck() {}

  /**
   * Call ->progress( int foo ) on this to update the account's progress
   * indicators.
   */
  ProgressItem *mailCheckProgressItem() const {
    return mMailCheckProgressItem;
  }

signals:
  /**
   * Emitted after the mail check is finished.
   * @param newMail true if there was new mail
   * @param status the status of the mail check
   **/
  virtual void finishedCheck( bool newMail, CheckStatus status );

  /**
   * Emitted after the mail check is finished.
   * @param newInFolder number of new messages for each folder
   **/
  virtual void newMailsProcessed( const QMap<QString, int> & newInFolder );

protected slots:
  virtual void mailCheck();
  virtual void sendReceipts();
  virtual void precommandExited(bool);

protected:
  KMAccount(KMAcctMgr* owner, const QString& accountName, uint id);

  /**
   * Does filtering and storing in a folder for the given message.
   * Shall be called from within processNewMail() to process the new
   * messages. Returns false if failed to add new message.
   */
  virtual bool processNewMsg(KMMessage* msg);

  /**
   * Send receipt of message back to sender (confirming
   *   delivery). Checks the config settings, calls
   *   @see KMMessage::createDeliveryReceipt and queues the resulting
   *   message in @p mReceipts.
   */
  virtual void sendReceipt(KMMessage* msg);

  /**
   * Install/deinstall automatic new-mail checker timer.
   */
  virtual void installTimer();
  virtual void deinstallTimer();

  /**
   * Call this to increase the number of new messages in a folder for
   * messages which are _not_ processed with processNewMsg().
   * @param folderId the id of the folder
   * @param num the number of new message in this folder
   */
  void addToNewInFolder( QString folderId, int num );

protected:
  QString       mPrecommand;
  QString       mTrash;
  KMAcctMgr*    mOwner;
  QGuardedPtr<KMAcctFolder> mFolder;
  QTimer *mTimer, mReceiptTimer;
  int mInterval;
  bool mExclude;
  bool mCheckingMail : 1;
  bool mPrecommandSuccess;
  QValueList<KMMessage*> mReceipts;
  QPtrList<FolderJob>  mJobList;
  bool mHasInbox : 1;
  ProgressItem *mMailCheckProgressItem;

private:
    /**
     * avoid compiler warning about hidden virtual
     */
    virtual void setName( const char *name ) { QObject::setName( name ); }

private:
  // for detailed (per folder) new mail notification
  QMap<QString, int> mNewInFolder;
};

class KMAcctList: public QPtrList<KMAccount>
{
public:
  virtual ~KMAcctList() {}
  /** some compilers fail otherwise */
  short _dummy;
};

#endif /*kmaccount_h*/
