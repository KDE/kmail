/* KMail list that manages the contents of one directory that may
 * contain folders and/or other directories.
 */
#ifndef kmfolderdir_h
#define kmfolderdir_h

#include <qstring.h>
#include "kmfoldernode.h"

class KMFolder;
class QCString;

class KMFolderDir: public KMFolderNode, public KMFolderNodeList
{
  Q_OBJECT

public:
  KMFolderDir(KMFolderDir* parent=NULL, const QCString& path=0);
  virtual ~KMFolderDir();

  virtual bool isDir() const { return TRUE; }

  /** Read contents of directory. */
  virtual bool reload();

  /** Return full pathname of this directory. */
  virtual const QCString path() const;

  /** Rename directory. Returns 0 on success and errno on failure. */
  virtual int rename(const QCString&);

  /** Create a mail folder in this directory with given name. If sysFldr==TRUE
   the folder is marked as a (KMail) system folder. 
   Returns Folder on success. */
  virtual KMFolder* createFolder(const QCString& folderName,
				     bool sysFldr=FALSE);

  /** Create a directory in this directory with given name. If sysFldr==TRUE
      the folder is marked as a (KMail) system folder.
      Returns TRUE on success. */
  virtual bool createDirectory(const QCString& folderName);

protected:
  /** Recursively loads all folders. Loads from mPath if path
    is not given. Stops when directory depth reaches 10. */
  virtual bool loadFolders(const QCString path=0, int depth=0);

};


//-----------------------------------------------------------------------------
class KMFolderRootDir: public KMFolderDir
{
  Q_OBJECT

public:
  KMFolderRootDir(const QCString& path=0);
  virtual ~KMFolderRootDir();
  virtual const QCString path() const;

  // set the absolute path
  virtual void setPath(const QCString&);

protected:
  QCString mPath;
};

#endif /*kmfolderdir_h*/
