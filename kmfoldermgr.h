/* KMail Folder Manager
 *
 */
#ifndef kmfoldermgr_h
#define kmfoldermgr_h

#include <qstring.h>
#include <qlist.h>

#include "kmfolderdir.h"

class KMAcctFolder;

class KMFolderMgr
{
public:
  KMFolderMgr(const char* basePath);
  virtual ~KMFolderMgr();

  /** Returns path to directory where all the folders live. */
  const QString& basePath(void) const { return mBasePath; }

  /** Set base path. Also calls reload() on the base directory. */
  virtual void setBasePath(const char*);

  /** Provides access to base directory */
  KMFolderRootDir& dir(void) { return mDir; }

  /** Searches folder and returns it. Skips directories 
    (objects of type KMFolderDir) if foldersOnly is TRUE. */
  KMAcctFolder* find(const char* folderName, bool foldersOnly=TRUE);

  /** Uses find() to find given folder. If not found the folder is
    created. Directories are skipped. */
  KMAcctFolder* findOrCreate(const char* folderName);

  /** Create a mail folder in the root folder directory dir()
    with given name. Returns Folder on success. */
  KMAcctFolder* createFolder(const char* fName, bool sysFldr=FALSE);

protected:
  QString mBasePath;
  KMFolderRootDir mDir;
};

#endif /*kmfoldermgr_h*/
