/* KMail account for local mail folders
 *
 */
#ifndef kmacctlocal_h
#define kmacctlocal_h

#include "kmaccount.h"

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
  virtual bool processNewMail(KMIOStatus *);
  virtual void readConfig(KConfig&);
  virtual void writeConfig(KConfig&);

protected:
  QString mLocation;
};

#endif /*kmacctlocal_h*/
