/* KMail list that manages the contents of one directory that may
 * contain folders and/or other directories.
 */
#ifndef kmfolderdir_h
#define kmfolderdir_h

#include <qstring.h>
#include "kmfoldernode.h"

class KMFolder;

class KMFolderDir: public KMFolderNode, public KMFolderNodeList
{
  Q_OBJECT

public:
  KMFolderDir(KMFolderDir* parent=NULL, const QString& path=0);
  virtual ~KMFolderDir();

  virtual bool isDir() const { return TRUE; }

  /** Read contents of directory. */
  virtual bool reload();

  /** Return full pathname of this directory. */
  virtual const QString path() const;

  /** Create a mail folder in this directory with given name. If sysFldr==TRUE
   the folder is marked as a (KMail) system folder. 
   Returns Folder on success. */
  virtual KMFolder* createFolder(const QString& folderName,
				 bool sysFldr=FALSE);

  /** Returns folder with given name or zero if it does not exist */
  virtual KMFolderNode* hasNamedFolder(const QString& name);

};


//-----------------------------------------------------------------------------

class KMFolderRootDir: public KMFolderDir
{
  Q_OBJECT

public:
  KMFolderRootDir(const QString& path=0);
  virtual ~KMFolderRootDir();
  virtual const QString path() const;

  // set the absolute path
  virtual void setPath(const QString&);

protected:
  QString mPath;
};

#endif /*kmfolderdir_h*/
