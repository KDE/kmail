/* KMail list that manages the contents of one directory that may
 * contain folders and/or other directories.
 */
#ifndef kmfolderdir_h
#define kmfolderdir_h

#include <qstring.h>
#include "kmfoldernode.h"

class KMAcctFolder;

class KMFolderDir: public KMFolderNode, public KMFolderNodeList
{
  Q_OBJECT

public:
  KMFolderDir(KMFolderDir* parent=NULL, const char* path=NULL);
  virtual ~KMFolderDir();

  virtual bool isDir(void) const { return TRUE; }

  /** Read contents of directory. */
  virtual bool reload(void);

  /** Return full pathname of this directory. */
  virtual const QString& path(void) const;

  /** Create a mail folder in this directory with given name. If sysFldr==TRUE
   the folder is marked as a (KMail) system folder. 
   Returns Folder on success. */
  virtual KMAcctFolder* createFolder(const char* folderName,
				     bool sysFldr=FALSE);
};


//-----------------------------------------------------------------------------

class KMFolderRootDir: public KMFolderDir
{
  Q_OBJECT

public:
  KMFolderRootDir(const char* name=NULL);
  virtual const QString& path(void) const;

  // set the absolute path
  virtual void setPath(const char*);

protected:
  QString mPath;
};

#endif /*kmfolderdir_h*/
