#ifndef kmacctfolder_h
#define kmacctfolder_h

#include "kmfolder.h"

class KMAccount;

/** Simple wrapper class that contains the kmail account handling
 * stuff that is usually not required outside kmail.
 *
 * WARNING: do not add virtual methods in this class. This class is
 * used as a typecast for KMFolder only !!
 * @author Stefan Taferner <taferner@alpin.or.at>
 */
class KMAcctFolder: public KMFolder
{
public:
  /** Returns first account or 0 if no account is associated with this
      folder */
  KMAccount* account(void);

  /** Returns next account or 0 if at the end of the list */
  KMAccount* nextAccount(void);

  /** Add given account to the list */
  void addAccount(KMAccount*);

  /** Remove given account from the list */
  void removeAccount(KMAccount*);

  /** Clear list of accounts */
  void clearAccountList(void);

private:
  KMAcctFolder( KMFolderDir* parent, const QString& name,
                KMFolderType aFolderType );
};


#endif /*kmacctfolder_h*/
