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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#ifndef kmacctmgr_h
#define kmacctmgr_h

#include <qobject.h>
#include "kmaccount.h"
#include <kdepimmacros.h>

class QString;
class QStringList;


class KDE_EXPORT KMAcctMgr: public QObject
{
  Q_OBJECT
  friend class KMAccount;

public:
  /** Initialize Account Manager and load accounts with reload() if the
    base path is given */
  KMAcctMgr();
  virtual ~KMAcctMgr();

  /** Completely reload accounts from config. */
  virtual void readConfig(void);

  /** Write accounts to config. */
  virtual void writeConfig(bool withSync=TRUE);

  /** Create a new account of given type with given name. Currently
   the types "local" for local mail folders and "pop" are supported. */
  virtual KMAccount* create( const QString& type,
                             const QString& name = QString::null, uint id = 0);

  /** Adds an account to the list of accounts */
  virtual void add(KMAccount *account);

  /** Find account by name. Returns 0 if account does not exist.
    Search is done case sensitive. */
  virtual KMAccount* findByName(const QString& name);

  /** Find account by id. Returns 0 if account does not exist.
   */
  virtual KMAccount* find(const uint id);

  /** Physically remove account. Also deletes the given account object !
      Returns FALSE and does nothing if the account cannot be removed. */
  virtual bool remove(KMAccount*);

  /** First account of the list */
  virtual KMAccount* first(void);

  /** Next account of the list */
  virtual KMAccount* next(void);

  /** Processes all accounts looking for new mail */
  virtual void checkMail(bool _interactive = true);

  /** Delete all IMAP folders and resync them */
  void invalidateIMAPFolders();

  QStringList getAccounts(bool noImap = false);

  /** Create a new unique ID */
  uint createId();

  /// Called on exit (KMMainWin::queryExit)
  void cancelMailCheck();

  /** Read passwords of all accounts from the wallet */
  void readPasswords();

  /** Reset connection list for the account */
  void resetConnectionList( KMAccount* acct ) {
    mServerConnections[ hostForAccount( acct ) ] = 0; }

public slots:
  virtual void singleCheckMail(KMAccount *, bool _interactive = true);
  virtual void singleInvalidateIMAPFolders(KMAccount *);

  virtual void intCheckMail(int, bool _interactive = true);
  virtual void processNextCheck(bool _newMail);

  /** this slot increases the count of new mails to show a total number
  after checking in multiple accounts. */
  virtual void addToTotalNewMailCount( const QMap<QString, int> & newInFolder );

signals:
  /**
   * Emitted if new mail has been collected.
   * @param newMail true if there was new mail
   * @param interactive true if the mail check was initiated by the user
   * @param newInFolder number of new messages for each folder
   **/
  void checkedMail( bool newMail, bool interactive,
                    const QMap<QString, int> & newInFolder );
  /** emitted when an account is removed */
  void accountRemoved( KMAccount* account );
  /** emitted when an account is added */
  void accountAdded( KMAccount* account );

private:
  KMAcctList   mAcctList;
  KMAcctList   mAcctChecking;
  KMAcctList   mAcctTodo;
  bool newMailArrived;
  bool interactive;
  int  mTotalNewMailsArrived;

  // for detailed (per folder) new mail notification
  QMap<QString, int> mTotalNewInFolder;

  // for restricting number of concurrent connections to the same server
  QMap<QString, int> mServerConnections;
  QString hostForAccount(const KMAccount *acct) const;

  // if a summary should be displayed
  bool mDisplaySummary;

};

#endif /*kmacctmgr_h*/
