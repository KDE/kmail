/* KMail Account Manager
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmacctmgr_h
#define kmacctmgr_h

#include <qobject.h>
#include "kmaccount.h"

class QString;
class QStringList;


class KMAcctMgr: public QObject
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
  virtual KMAccount* create(const QString& type, const QString& name);

  /** Adds an account to the list of accounts */
  virtual void add(KMAccount *account);

  /** Find account by name. Returns 0 if account does not exist.
    Search is done case sensitive. */
  virtual KMAccount* find(const QString& name);

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

public slots:
  virtual void singleCheckMail(KMAccount *, bool _interactive = true);
  virtual void singleInvalidateIMAPFolders(KMAccount *);

  virtual void intCheckMail(int, bool _interactive = true);
  virtual void processNextCheck(bool _newMail);

  /** this slot increases the count of new mails to show a total number
  after checking in multiple accounts. */
  virtual void addToTotalNewMailCount(int numNewMails);

signals:
  /** emitted if new mail has been collected */
  void checkedMail(bool, bool);

private:
  KMAcctList   mAcctList;
  KMAcctList   mAcctChecking;
  KMAcctList   mAcctTodo;
  bool newMailArrived;
  bool interactive;
  int  mTotalNewMailsArrived;
};

#endif /*kmacctmgr_h*/
