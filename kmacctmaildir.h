/* KMail account for maildir mail folders
 *
 */
#ifndef kmacctmaildir_h
#define kmacctmaildir_h

#include "kmaccount.h"
#include "kmglobal.h"

#define KMAcctMaildirInherited KMAccount

class KMAcctMaildir: public KMAccount
{
protected:
  friend class KMAcctMgr;

  KMAcctMaildir(KMAcctMgr* owner, const QString& accountName);

public:
  virtual ~KMAcctMaildir();
  virtual void init(void);

  /** Access to location of maildir mail file (usually something like
   "/home/joe/Maildir"). */
  const QString& location(void) const { return mLocation; }
  virtual void setLocation(const QString&);

  virtual QString type(void) const;
  virtual void processNewMail(bool);
  virtual void readConfig(KConfig&);
  virtual void writeConfig(KConfig&);
  virtual void pseudoAssign(KMAccount*);

protected:
  QString mLocation;
  bool hasNewMail;
};

#endif /*kmacctmaildir_h*/
