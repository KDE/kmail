/* KMail Account Manager
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmacctmgr_h
#define kmacctmgr_h

#include <qobject.h>
#include <qlist.h>
#include <qstring.h>
#include <qdir.h>
#include <qfile.h>
#include <qstringlist.h>
#include "kmnewiostatuswdg.h"
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
  const QString basePath(void) const { return mBasePath; }

  /** Set base path. Does *not* call reload(). A tilde (~) as the first
    character is expanded to the contents of the HOME environment variable. */
  virtual void setBasePath(const char* aBasePath);

  /** Completely reload accounts from config. */
  virtual void readConfig(void);

  /** Write accounts to config. */
  virtual void writeConfig(bool withSync=TRUE);

  /** Create a new account of given type with given name. Currently
   the types "local" for local mail folders and "pop" are supported. */
  virtual KMAccount* create(const QString type, const QString name);

  /** Find account by name. Returns NULL if account does not exist.
    Search is done case sensitive. */
  virtual KMAccount* find(const QString name);

  /** Physically remove account. Also deletes the given account object !
      Returns FALSE and does nothing if the account cannot be removed. */
  virtual bool remove(KMAccount*);

  /** First account of the list */
  virtual KMAccount* first(void);

  /** Next account of the list */
  virtual KMAccount* next(void);

  /** Processes all accounts looking for new mail */
  virtual void checkMail(bool _interactive = true);

  QStringList getAccounts();

public slots:
  virtual void singleCheckMail(KMAccount *, bool _interactive = true);

  virtual void intCheckMail(int, bool _interactive = true);
  virtual void processNextAccount(bool newMail); 

signals:
  /** emitted if new mail has been collected */
  void checkedMail(bool);

private:
  QString      mBasePath;
  KMAcctList   mAcctList;
  QListIterator< KMAccount > *mAccountIt;
  KMAccount *lastAccountChecked;
  bool checking;
  bool newMailArrived;
  bool interactive;
};

#endif /*kmacctmgr_h*/
