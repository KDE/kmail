/* KMail Account Manager
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmacctmgr_h
#define kmacctmgr_h

#include <qobject.h>
#include <qlist.h>
#include <qstring.h>
#include "kmaccount.h"

#define KMAcctMgrInherited QObject

class KMAcctMgr: public QObject
{
  Q_OBJECT
  friend class KMAccount;

public:
  /** Initialize Account Manager and load accounts with reload() if the
    base path is given */
  KMAcctMgr(const char* aBasePath);
  virtual ~KMAcctMgr();

  /** Returns path to directory where the accounts' configuration is stored. */
  const QString& basePath(void) const { return mBasePath; }

  /** Set base path. Does *not* call reload(). A tilde (~) as the first
    character is expanded to the contents of the HOME environment variable. */
  virtual void setBasePath(const char* aBasePath);

  /** Completely reload accounts from disk. Returns TRUE on success. */
  virtual bool reload(void);

  /** Write accounts to disk. */
  virtual void sync(void);

  /** Create a new account of given type with given name. Currently
   the types "local" for local mail folders and "pop" are supported. */
  virtual KMAccount* create(const QString& type, const QString& name);

  /** Find account by name. Returns NULL if account does not exist.
    Search is done case sensitive. */
  virtual KMAccount* find(const QString& name);

  /** Physically remove account. Also deletes the given account object !
      Returns FALSE and does nothing if the account cannot be removed. */
  virtual bool remove(KMAccount*);

  /** First account of the list */
  virtual KMAccount* first(void);

  /** Next account of the list */
  virtual KMAccount* next(void);

  /** Processes all accounts looking for new mail. Returns TRUE if there
   is new mail in at least one account. */
  virtual bool checkMail(void);

signals:
  /** emitted if new mail arrived in the account */
  void newMail(KMAccount* inAccount);

private:
  QString      mBasePath;
  KMAcctList   mAcctList;
};

#endif /*kmacctmgr_h*/
