/* KMail Folder Manager
 *
 */
#ifndef kmfoldermgr_h
#define kmfoldermgr_h

#include <qcstring.h>
#include <qlist.h>
#include <qobject.h>
#include <qvaluelist.h>

#include "kmfolderdir.h"

class KMFolder;
typedef QValueList<QCString> KMCStringList;


#define KMFolderMgrInherited QObject
class KMFolderMgr: public QObject
{
  Q_OBJECT

  friend class KMFolder;

public:
  KMFolderMgr(const QCString& basePath);
  virtual ~KMFolderMgr();

  /** Returns path to directory where all the folders live. */
  const QCString& basePath() const { return mBasePath; }

  /** Set base path. Also calls reload() on the base directory. */
  virtual void setBasePath(const QCString&);

  /** Provides access to base directory */
  KMFolderRootDir& dir();

  /** Searches folder and returns it. Skips directories 
    (objects of type KMFolderDir) if foldersOnly is TRUE. */
  virtual KMFolder* find(const char* folderName, bool foldersOnly=TRUE);

  /** Uses find() to find given folder. If not found the folder is
    created. Directories are skipped. */
  virtual KMFolder* findOrCreate(const char* folderName);

  /** Create a mail folder in the root folder directory dir()
    with given name. Returns Folder on success. */
  virtual KMFolder* createFolder(const char* fName, bool sysFldr=FALSE);

  /** Create a directory in the root folder directory dir()
    with given name. Returns true on success. */
  virtual bool createDirectory(const char* fName);

  /** Physically remove given folder and delete the given folder object. */
  virtual void remove(KMFolder* obsoleteFolder);

  /** emits changed() signal */
  virtual void contentsChanged();

  /** Reloads all folders, discarding the existing ones. */
  virtual void reload();

  /** Returns a list of all folder paths, relative to the base path. */
  virtual KMCStringList& folderList();

signals:
  /** Emitted when the list of folders has changed. This signal is a hook
    where clients like the KMFolderTree tree-view can connect. The signal
    is meant to be emitted whenever the code using the folder-manager
    changed things. */
  void changed();

  /** Emitted when the number of unread messages of a folder has changed. */
  void unreadChanged(KMFolder*);

private:
  /** Recursively collects all folders. Called by folderList(). */
  void folderListRecursive(KMCStringList*, KMFolderDir*, const QCString& path);

protected:
  QCString mBasePath;
  KMFolderRootDir mDir;
};

#endif /*kmfoldermgr_h*/
