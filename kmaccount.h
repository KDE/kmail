/* KMail Account: Abstract base class for accounts.
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmaccount_h
#define kmaccount_h

#include <qstring.h>
#include <qlist.h>
#include <Kconfig.h>

class QFile;
class QTextStream;
class KMAcctMgr;
class KMAcctFolder;

class KMAccount
{
  friend class KMAcctMgr;

public:
  virtual ~KMAccount();

  /** Returns type of the account */
  virtual const char* type(void) const = 0;

  /** Returns account name */
  const QString& name(void) const { return mName; }
  virtual void setName(const QString&);

#ifdef DO_WE_NEED_THAT
  enum { amRemote=1, amMove=2, amCopy=3 } AccessMethod;

  /** New messages can be either  */
  AccessMethod accessMethod(void) const { return mAccessMethod; }
  virtual void setAccessMethod(AccessMethod);
#endif

  /** There can be exactly one folder that is fed by messages from an
    account. */
  KMAcctFolder* folder(void) { return mFolder; }
  virtual void setFolder(KMAcctFolder*);

  /** Process new mail for this account if one arrived. Returns TRUE if new
    mail has been found. Whether the mail is automatically loaded to
    an associated folder or not depends on the type of the account. */
  virtual bool processNewMail(void) = 0;

  /** Read config file entries. This method is called by the account
    manager when a new account is created. */
  virtual void readConfig(void) = 0;

  /** Set config entries and save them to the config file. This method
   is called before the account is deleted in the destructor. */
  virtual void writeConfig(void) = 0;

  /** For compatibility this method allows access to the config
    object. DO NOT USE THIS METHOD IN NEW CODE. It will go away soon. */
  KConfig* config(void) { return mConfig; }

protected:
  KMAccount(KMAcctMgr* owner, const char* accountName);

  /** Takes ownership of given, already opened, config object and calls
    readConfig() which sets the field necessary for the specific type of
    account. There is usually no need to inherit this method. */
  virtual void takeConfig(KConfig*, QFile*, QTextStream*);

  QString       mName;
  KMAcctMgr*    mOwner;
  KMAcctFolder* mFolder;
  KConfig*      mConfig;
  QFile*        mCFile;
  QTextStream*  mCStream;
};

typedef QList<KMAccount> KMAcctList;

#endif /*kmaccount_h*/
