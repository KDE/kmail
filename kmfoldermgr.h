/* KMail Folder Manager
 *
 */
#ifndef kmfoldermgr_h
#define kmfoldermgr_h

#include <qstring.h>
#include <qlist.h>
#include <qobject.h>

#include "kmfolderdir.h"

class KMFolder;

#define KMFolderMgrInherited QObject
class KMFolderMgr: public QObject
{
  Q_OBJECT

  friend class KMFolder;

public:
  KMFolderMgr(const char* basePath);
  virtual ~KMFolderMgr();

  /** Returns path to directory where all the folders live. */
  const QString& basePath(void) const { return mBasePath; }

  /** Set base path. Also calls reload() on the base directory. */
  virtual void setBasePath(const char*);

  /** Provides access to base directory */
  KMFolderRootDir& dir(void);

  /** Searches folder and returns it. Skips directories 
    (objects of type KMFolderDir) if foldersOnly is TRUE. */
  virtual KMFolder* find(const char* folderName, bool foldersOnly=TRUE);

  /** Uses find() to find given folder. If not found the folder is
    created. Directories are skipped. */
  virtual KMFolder* findOrCreate(const char* folderName);

  /** Create a mail folder in the root folder directory dir()
    with given name. Returns Folder on success. */
  virtual KMFolder* createFolder(const char* fName, bool sysFldr=FALSE);

  /** Physically remove given folder and delete the given folder object. */
  virtual void remove(KMFolder* obsoleteFolder);

  /** emits changed() signal */
  virtual void contentsChanged(void);

  /** Reloads all folders, discarding the existing ones. */
  virtual void reload(void);

signals:
  /** Emitted when the list of folders has changed. This signal is a hook
    where clients like the KMFolderTree tree-view can connect. The signal
    is meant to be emitted whenever the code using the folder-manager
    changed things. */
  void changed();

  /** Emitted when the number of unread messages of a folder has changed. */
  void unreadChanged(KMFolder*);

protected:
  QString mBasePath;
  KMFolderRootDir mDir;
};

#endif /*kmfoldermgr_h*/
