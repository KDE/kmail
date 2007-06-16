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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef accountmanager_h
#define accountmanager_h

#include "kmail_export.h"
#include "kmaccount.h"
#include <QObject>

class QString;
class QStringList;

namespace KMail {
/**
 * The account manager is responsible for creating accounts of various types
 * via the factory method create() and for keeping track of them.
 */
class KMAIL_EXPORT AccountManager: public QObject
{
  Q_OBJECT
  friend class ::KMAccount;

  public:
    /**
      Initializes the account manager. readConfig() needs to be called in
      order to fill it with persisted account information from the config file.
    */
    AccountManager();
    ~AccountManager();

    /**
      Completely reload accounts from config.
    */
    void readConfig( void );

    /**
      Write accounts to config.
    */
    void writeConfig( bool withSync = true );

    /**
      Create a new account of given type with given name. Currently
      the types "local" for local mail folders and "pop" are supported.
    */
    KMAccount *create( const QString &type,
                       const QString &name = QString(),
                       uint id = 0 );

    /**
      Adds an account to the list of accounts.
    */
    void add( KMAccount *account );

    /**
      Find account by name. Returns 0 if account does not exist.
      Search is done case sensitive.
    */
    KMAccount *findByName( const QString &name ) const;

    /**
      Find account by id. Returns 0 if account does not exist.
    */
    KMAccount *find( const uint id ) const;

    /**
      Physically remove account. Also deletes the given account object !
      Returns false and does nothing if the account cannot be removed.
    */
    bool remove( KMAccount *account );

    /**
      First account of the list.
    */
    const KMAccount *first() const { return first(); }
    KMAccount *first();

    /**
      Next account of the list/
    */
    const KMAccount *next() const { return next(); }
    KMAccount *next();

    /**
      Processes all accounts looking for new mail.
    */
    void checkMail( bool interactive = true );

    /**
      Delete all IMAP folders and resync them.
    */
    void invalidateIMAPFolders();

    QStringList getAccounts() const;

    /// Called on exit (KMMainWin::queryExit)
    void cancelMailCheck();

    /**
      Read passwords of all accounts from the wallet.
    */
    void readPasswords();

  public slots:
    void singleCheckMail( KMAccount *account, bool interactive = true );
    void singleInvalidateIMAPFolders( KMAccount *account );

    void intCheckMail( int, bool interactive = true );
    void processNextCheck( bool newMail );

    /**
      Increases the count of new mails to show a total number after checking
      in multiple accounts.
    */
    void addToTotalNewMailCount( const QMap<QString, int> &newInFolder );

  signals:
    /**
      Emitted if new mail has been collected.
      @param newMail true if there was new mail
      @param interactive true if the mail check was initiated by the user
      @param newInFolder number of new messages for each folder
    */
    void checkedMail( bool newMail, bool interactive,
                      const QMap<QString, int> &newInFolder );

    /**
      Emitted when an account is removed.
    */
    void accountRemoved( KMAccount *account );

    /**
      Emitted when an account is added.
    */
    void accountAdded( KMAccount *account );

  private:
     /**
       Creates a new unique ID.
     */
    uint createId();

    AccountList   mAcctList;
    AccountList::Iterator mPtrListInterfaceProxyIterator;
    AccountList   mAcctChecking;
    AccountList   mAcctTodo;
    bool mNewMailArrived;
    bool mInteractive;
    int  mTotalNewMailsArrived;

    // for detailed (per folder) new mail notification
    QMap<QString, int> mTotalNewInFolder;

    // if a summary should be displayed
    bool mDisplaySummary;
};

} // namespace KMail
#endif /*accountmanager_h*/
