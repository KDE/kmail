/* KMail Folder Manager
 *
 */
#ifndef kmfoldermgr_h
#define kmfoldermgr_h

#include <qstring.h>
#include <qlist.h>

#include "kmfolderdir.h"

class KMFolder;

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
  KMFolder* find(const char* folderName, bool foldersOnly=TRUE);

  /** Create a mail folder in the root folder directory dir()
    with given name. Returns Folder on success. */
  KMFolder* createFolder(const char* fName) {return mDir.createFolder(fName);}

protected:
  QString mBasePath;
  KMFolderRootDir mDir;
};

#endif /*kmfoldermgr_h*/
