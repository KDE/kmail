#ifndef kmfolderdir_h
#define kmfolderdir_h

#include <qstring.h>
#include "kmfoldernode.h"
#include "kmfoldertype.h"

class KMFolder;
class KMFolderMgr;


/** KMail list that manages the contents of one directory that may
 * contain folders and/or other directories.
 */
class KMFolderDir: public KMFolderNode, public KMFolderNodeList
{
  Q_OBJECT

public:
  KMFolderDir(KMFolderDir* parent=0, const QString& path=QString::null, 
	      KMFolderDirType = KMStandardDir );
  virtual ~KMFolderDir();

  virtual bool isDir() const { return TRUE; }

  /** Read contents of directory. */
  virtual bool reload();

  /** Return full pathname of this directory. */
  virtual QString path() const;

  /** Create a mail folder in this directory with given name. If sysFldr==TRUE
   the folder is marked as a (KMail) system folder.
   Returns Folder on success. */
  virtual KMFolder* createFolder(const QString& folderName,
				 bool sysFldr=FALSE,
                                 KMFolderType folderType=KMFolderTypeMbox);

  /** Returns folder with given name or zero if it does not exist */
  virtual KMFolderNode* hasNamedFolder(const QString& name);

  /** Returns the folder manager that manages this folder */
  virtual KMFolderMgr* manager() const;

  virtual KMFolderDirType type() { return mDirType; }

protected:
  KMFolderDirType mDirType;
};


//-----------------------------------------------------------------------------

class KMFolderRootDir: public KMFolderDir
{
  Q_OBJECT

public:
  KMFolderRootDir(KMFolderMgr* manager,
		  const QString& path=QString::null, 
		  KMFolderDirType dirType = KMStandardDir);
  virtual ~KMFolderRootDir();
  virtual QString path() const;

  /** set the absolute path */
  virtual void setPath(const QString&);

  virtual KMFolderMgr* manager() const;

protected:
  QString mPath;
  KMFolderMgr *mManager;
};

#endif /*kmfolderdir_h*/

