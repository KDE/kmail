/* Simple wrapper class that contains the kmail account handling
 * stuff that is usually not required outside kmail.
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmacctfolder_h
#define kmacctfolder_h

#include "kmfolder.h"

class KMAccount;

/** WARNING: do not add virtual methods in this class. This class is
  * used as a typecast for KMFolder only !!
  */
class KMAcctFolder: public KMFolder
{
public:
  /** Returns first account or NULL if no account is associated with this
      folder */
  KMAccount* account(void);

  /** Returns next account or NULL if at the end of the list */
  KMAccount* nextAccount(void);

  /** Add given account to the list */
  void addAccount(KMAccount*);

  /** Remove given account from the list */
  void removeAccount(KMAccount*);

  /** Clear list of accounts */
  void clearAccountList(void);
};


#endif /*kmacctfolder_h*/
