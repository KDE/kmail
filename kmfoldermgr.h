/* KMail Folder Manager
 *
 */
#ifndef kmfoldermgr_h
#define kmfoldermgr_h

#include <qstring.h>
#include <qlist.h>
#include <qobject.h>
#include <qguardedptr.h>

#include "kmfolderdir.h"

class KMFolder;

#define KMFolderMgrInherited QObject
class KMFolderMgr: public QObject
{
  Q_OBJECT

public:
  KMFolderMgr(const QString& basePath);
  virtual ~KMFolderMgr();

  /** Returns path to directory where all the folders live. */
  QString basePath() const { return mBasePath; }

  /** Set base path. Also calls reload() on the base directory. */
  virtual void setBasePath(const QString&);

  /** Provides access to base directory */
  KMFolderRootDir& dir();

  /** Searches folder and returns it. Skips directories
    (objects of type KMFolderDir) if foldersOnly is TRUE. */
  virtual KMFolder* find(const QString& folderName, bool foldersOnly=TRUE);

  /** Searches for a folder with the given id, recurses into directories */
  virtual KMFolder* findIdString(const QString& folderId, KMFolderDir *dir=0);

  /** Uses find() to find given folder. If not found the folder is
    created. Directories are skipped. */
  virtual KMFolder* findOrCreate(const QString& folderName, bool sysFldr=TRUE);

  /** Create a mail folder in the root folder directory dir()
    with given name. Returns Folder on success. */
  virtual KMFolder* createFolder(const QString& fName, bool sysFldr=FALSE,
				 KMFolderDir *aFolderDir = 0);

  /** Physically remove given folder and delete the given folder object. */
  virtual void remove(KMFolder* obsoleteFolder);

  /** emits changed() signal */
  virtual void contentsChanged(void);

  /** Reloads all folders, discarding the existing ones. */
  virtual void reload(void);

  /** Create a list of formatted formatted folder labels and corresponding
   folders*/
  virtual void createFolderList( QStringList *str,
				 QValueList<QGuardedPtr<KMFolder> > *folders );

  /** Create a list of formatted formatted folder labels and corresponding
   folders. The system folder names are translated */
  virtual void createI18nFolderList( QStringList *str,
				 QValueList<QGuardedPtr<KMFolder> > *folders );

public slots:
  /** Compacts all folders (they know is it needed) */
  void compactAll();

signals:
  /** Emitted when the list of folders has changed. This signal is a hook
    where clients like the KMFolderTree tree-view can connect. The signal
    is meant to be emitted whenever the code using the folder-manager
    changed things. */
  void changed();

protected:

  /* Auxillary function to faciliate compaction of folders */
  void compactAllAux(KMFolderDir* dir);

  /* Auxillary function to facilitate removal of a folder */
  void removeFolderAux(KMFolder* aFolder);

  /* Auxillary function to facilitate removal of a folder directory */
  void removeDirAux(KMFolderDir* aFolderDir);

  /* Auxillary function to facilitate creating a list of formatted
     folder names, suitable for showing in QComboBox */
  virtual void createFolderList( QStringList *str,
 				 QValueList<QGuardedPtr<KMFolder> > *folders,
  				 KMFolderDir *adir,
  				 const QString& prefix,
				 bool i18nized=FALSE );

  QString mBasePath;
  KMFolderRootDir mDir;
};

#endif /*kmfoldermgr_h*/
