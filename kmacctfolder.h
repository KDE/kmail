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
  virtual const char* type(void) const;

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

  /** Returns TRUE if the folder is a kmail system folder. These are
    the folders 'inbox', 'outbox', 'sent', 'trash'. The name of these
    folders is also nationalized in the folder display. */
  bool isSystemFolder(void) const { return mIsSystemFolder; }
  void setSystemFolder(bool itIs) { mIsSystemFolder=itIs; }

  /** Returns the label of the folder for visualization. */
  virtual const QString label(void) const;
  void setLabel(const QString lbl) { mLabel = lbl; }

protected:
  virtual void readTocHeader(void);
  virtual int createTocHeader(void);

  KMAcctList mAcctList;
  bool mIsSystemFolder;
  QString mLabel;
};


#endif /*kmacctfolder_h*/
