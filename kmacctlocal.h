/* KMail account for local mail folders
 *
 */
#ifndef kmacctlocal_h
#define kmacctlocal_h

#include "kmaccount.h"
#include "kmglobal.h"

#define KMAcctLocalInherited KMAccount

class KMAcctLocal: public KMAccount
{
protected:
  friend class KMAcctMgr;

  KMAcctLocal(KMAcctMgr* owner, const char* accountName);

public:
  virtual ~KMAcctLocal();
  virtual void init(void);

  /** Access to location of local mail file (usually something like
   "/var/spool/mail/joe"). */
  const QString& location(void) const { return mLocation; }
  virtual void setLocation(const QString&);

  virtual const char* type(void) const;
  virtual void processNewMail(bool);
  virtual void readConfig(KConfig&);
  virtual void writeConfig(KConfig&);
  virtual void pseudoAssign(KMAccount*);

  LockType mLock;

protected:
  QString mLocation;
  bool hasNewMail;
};

#endif /*kmacctlocal_h*/
