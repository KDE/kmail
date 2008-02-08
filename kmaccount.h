/* -*- mode: C++ -*-
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef kmaccount_h
#define kmaccount_h

#include "kmmessage.h"
#include "kmacctfolder.h"

#include <kprocess.h>
#include <kaccount.h>

#include <QList>
#include <QMap>
#include <QPointer>
#include <QString>

class QTimer;

class KMFolder;
class KMAcctFolder;
class KMFolderCachedImap;
class AccountsPageReceivingTab;

namespace  KMail {
  class FolderJob;
  class AccountManager;
}
using KMail::AccountManager;
using KMail::FolderJob;
namespace KPIM { class ProgressItem; }
using KPIM::ProgressItem;
using KPIM::KAccount;

class KMAccount;
typedef QList< ::KMAccount* > AccountList;

class KMPrecommand : public QObject
{
  Q_OBJECT

public:
  explicit KMPrecommand(const QString &precommand, QObject *parent = 0);
  virtual ~KMPrecommand();
  bool start();

signals:
  void finished(bool);

private slots:
  void precommandExited(int errorCode, QProcess::ExitStatus status);

private:
  KProcess mPrecommandProcess;
  QString mPrecommand;
};


class KMAccount: public QObject, public KAccount
{
  Q_OBJECT
  friend class KMail::AccountManager;
  friend class ::KMail::FolderJob;
  friend class ::AccountsPageReceivingTab; // part of the config dialog
  friend class ::KMFolderCachedImap; /* HACK for processNewMSg() */

public:
  virtual ~KMAccount();

  enum CheckStatus { CheckOK, CheckIgnored, CheckCanceled, CheckAborted,
                     CheckError };

  /** The default check interval */
  static const int DefaultCheckInterval = 5;

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
   * Process new mail for this account if one arrived. Returns true if new
   * mail has been found. Whether the mail is automatically loaded to
   * an associated folder or not depends on the type of the account.
   */
  virtual void processNewMail(bool interactive) = 0;

  /**
   * Read config file entries. This method is called by the account
   * manager when a new account is created.
   */
  virtual void readConfig(KConfigGroup& config);

  /**
   * Write all account information to given config file.
   */
  virtual void writeConfig(KConfigGroup& config);

  /**
   * Set/get interval for checking if new mail arrived (in minutes).
   * An interval of zero (or less) disables the automatic checking.
   */
  virtual void setCheckInterval(int aInterval);
  int checkInterval() const;

  /**
   * This can be used to provide a more complex calculation later if we want
   */
  inline int defaultCheckInterval(void) const { return DefaultCheckInterval; }

  /**
   * Deletes the set of FolderJob associated with this account.
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

  static QString importPassword(const QString &);

  /** @return whether this account has an inbox */
  bool hasInbox() const { return mHasInbox; }
  virtual void setHasInbox( bool has ) { mHasInbox = has; }

  /**
   * If this account is a disconnected IMAP account, invalidate it.
   */
  virtual void invalidateIMAPFolders();

  /**
   * Determines whether the account can be checked, currently.
   * Reimplementations can use this to prevent mailchecks due to
   * exceeded connection limits, or because a network link iis down.
   * @return whether mail checks can proceed
   */
  virtual bool mailCheckCanProceed() const { return true; }

  /**
   * Set/Get if this account is currently checking mail
   */
  bool checkingMail() { return mCheckingMail; }
  virtual void setCheckingMail( bool checking ) { mCheckingMail = checking; }

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

  /**
   * Set/get identity for checking account.
   * If useDefaultIdentity() is true, the getter will always return the
   * default identity of the identity manager.
   */
  void setIdentityId( uint identityId ) { mIdentityId = identityId; }
  uint identityId() const;

  /**
   * Set/get whether to use the default identity instead of the identity
   * specified with setIdentityId().
   */
  void setUseDefaultIdentity( bool useDefaultIdentity ) {
    mUseDefaultIdentity = useDefaultIdentity;  }
  bool useDefaultIdentity() const { return mUseDefaultIdentity; }

signals:
  /**
   * Emitted after the mail check is finished.
   * @param newMail true if there was new mail
   * @param status the status of the mail check
   **/
  void finishedCheck( bool newMail, CheckStatus status );

  /**
   * Emitted after the mail check is finished.
   * @param newInFolder number of new messages for each folder
   **/
  void newMailsProcessed( const QMap<QString, int> & newInFolder );

protected slots:
  virtual void mailCheck();
  virtual void sendReceipts();
  virtual void precommandExited(bool);
  void slotIdentitiesChanged();

protected:
  KMAccount( AccountManager* owner, const QString& accountName, uint id);

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
  void addToNewInFolder( const QString &folderId, int num );

protected:
  QString       mPrecommand;
  QString       mTrash;
  AccountManager*    mOwner;
  QPointer<KMAcctFolder> mFolder;
  QTimer *mTimer;
  int mInterval; // this is a switch and a scalar at the same time. If it is 0,
  // interval mail checking is turned off and the interval spinbox proposes a
  // default value. If it is non-null, it is the count of minutes between two
  // automated mail checks. This means that as soon as you disable interval
  // mail checking, the value in the spin box returns to a default value.
  bool mExclude;
  bool mCheckingMail : 1;
  bool mPrecommandSuccess;
  bool mUseDefaultIdentity;
  QList<KMMessage*> mReceipts;
  QList<FolderJob*>  mJobList;
  bool mHasInbox : 1;
  ProgressItem *mMailCheckProgressItem;
  uint mIdentityId;
private:
  // for detailed (per folder) new mail notification
  QMap<QString, int> mNewInFolder;
};

#endif /*kmaccount_h*/
