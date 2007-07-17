/* KMail account for local mail folders
 *
 */
#ifndef kmacctlocal_h
#define kmacctlocal_h

#include "kmaccount.h"
#include "kmglobal.h"

class KMAcctLocal: public KMAccount
{
protected:
  friend class ::AccountManager;

  KMAcctLocal(AccountManager* owner, const QString& accountName, uint id);

public:
  virtual ~KMAcctLocal();
  virtual void init(void);

  virtual void pseudoAssign( const KMAccount * a );

  /** Access to location of local mail file (usually something like
   "/var/spool/mail/joe"). */
  QString location(void) const { return mLocation; }
  virtual void setLocation(const QString&);

  /** Acceso to Locking method */
  LockType lockType(void) const { return mLock; }
  void setLockType(LockType lt) { mLock = lt; }

  QString procmailLockFileName(void) const { return mProcmailLockFileName; }
  void setProcmailLockFileName(const QString& s);

  virtual void processNewMail(bool);
  virtual void readConfig(KConfigGroup&);
  virtual void writeConfig(KConfigGroup&);

private slots:
  bool preProcess();
  bool fetchMsg();
  void postProcess();

private:
  QString mLocation;
  QString mProcmailLockFileName;
  bool mHasNewMail;
  bool mProcessingNewMail;
  bool mAddedOk;
  LockType mLock;
  int mNumMsgs;
  int mMsgsFetched;
  KMFolder *mMailFolder;
};

#endif /*kmacctlocal_h*/
