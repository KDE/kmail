/* Mail folder that has a list of accounts with which it is associated
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmacctfolder_h
#define kmacctfolder_h

#include "kmfolder.h"
#include "kmaccount.h"

#define KMAcctFolderInherited KMFolder

class KMAcctFolder: public KMFolder
{
public:
  KMAcctFolder(KMFolderDir* parent=NULL, const char* name=NULL);
  virtual ~KMAcctFolder();

  /** Returns first account or NULL if no account is associated with this
      folder */
  virtual KMAccount* account(void);

  /** Returns next account or NULL if at the end of the list */
  virtual KMAccount* nextAccount(void);

  /** Add given account to the list */
  virtual void addAccount(KMAccount*);

  /** Remove given account from the list */
  virtual void removeAccount(KMAccount*);

  /** Clear list of accounts */
  virtual void clearAccountList(void);

protected:
  virtual void readTocHeader(void);
  virtual int createTocHeader(void);

  KMAcctList mAcctList;
};


#endif /*kmacctfolder_h*/
