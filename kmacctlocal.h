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
  friend class KMAcctMgr;

  KMAcctLocal(KMAcctMgr* owner, const QString& accountName);

public:
  typedef KMAccount base;

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

  virtual QString type(void) const;
  virtual void processNewMail(bool);
  virtual void readConfig(KConfig&);
  virtual void writeConfig(KConfig&);

  LockType mLock;

protected:
  QString mLocation;
  QString mProcmailLockFileName;
  bool hasNewMail;
};

#endif /*kmacctlocal_h*/
