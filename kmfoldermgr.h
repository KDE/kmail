/* KMail Folder Manager
 *
 */
#ifndef kmfoldermgr_h
#define kmfoldermgr_h

#include <qstring.h>
#include <qvaluelist.h>
#include <qobject.h>
#include <qguardedptr.h>

#include "kmfolderdir.h"

class KMFolder;
class KMMsgDict;

class KMFolderMgr: public QObject
{
  Q_OBJECT

public:
  KMFolderMgr(const QString& basePath, KMFolderDirType dirType = KMStandardDir);
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
  virtual KMFolder* findIdString(const QString& folderId,
     const uint id = 0, KMFolderDir *dir = 0);

  /** Uses find() to find given folder. If not found the folder is
   * created. Directories are skipped.
   * If an id is passed this searches for it
   */
  virtual KMFolder* findOrCreate(const QString& folderName, bool sysFldr=TRUE,
      const uint id = 0);

  /** Searches folder by id and returns it. Skips directories
    (objects of type KMFolderDir) */
  virtual KMFolder* findById(const uint id);

  virtual void        getFolderURLS( QStringList& flist,
                                     const QString& prefix=QString::null,
                                     KMFolderDir *adir=0 );
  virtual KMFolder*   getFolderByURL( const QString& vpath,
                                      const QString& prefix=QString::null,
                                      KMFolderDir *adir=0 );

  /** Create a mail folder in the root folder directory dir()
    with given name. Returns Folder on success. */
  virtual KMFolder* createFolder(const QString& fName, bool sysFldr=FALSE,
				 KMFolderType aFolderType=KMFolderTypeMbox,
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

  /** Auxillary function to facilitate creating a list of formatted
      folder names, suitable for showing in @see QComboBox */
  virtual void createFolderList( QStringList *str,
 				 QValueList<QGuardedPtr<KMFolder> > *folders,
  				 KMFolderDir *adir,
  				 const QString& prefix,
				 bool i18nized=FALSE );

  /** Create a list of formatted formatted folder labels and corresponding
   folders. The system folder names are translated */
  virtual void createI18nFolderList( QStringList *str,
				 QValueList<QGuardedPtr<KMFolder> > *folders );

  /** fsync all open folders to disk */
  void syncAllFolders( KMFolderDir *adir = 0 );

  /** Compact all folders that need to be, either immediately or scheduled as a background task */
  void compactAllFolders( bool immediate, KMFolderDir *adir = 0 );

  /** Expire old messages in all folders, either immediately or scheduled as a background task */
  void expireAllFolders( bool immediate, KMFolderDir *adir = 0 );

  /** Inserts messages into the message dictionary.  Called during
    kernel initialization. */
  void readMsgDict(KMMsgDict *dict, KMFolderDir *dir=0, int pass = 1);

  /** Writes message serial on disk.  Called during kernel shutdown. */
  void writeMsgDict(KMMsgDict *dict, KMFolderDir *dir=0);

  /** Enable, disable changed() signals */
  void quiet(bool);

  /** Number of folders for purpose of progres report */
  int folderCount(KMFolderDir *dir=0);

  /** Called when serial numbers for a folder are invalidated,
      invalidates/recreates data structures dependent on the
      serial numbers for this folder */
  void invalidateFolder(KMMsgDict *dict, KMFolder *folder);

  /** Try closing @p folder if possible, something is attempting an exclusive access to it.
      Currently used for KMFolderSearch and the background tasks like expiry */
  void tryReleasingFolder(KMFolder* folder, KMFolderDir *Dir=0);

  /** Create a new unique ID */
  uint createId();

  /** Rename or move a folder */
  void renameFolder( KMFolder* folder, const QString& newName, 
      KMFolderDir* newParent = 0 );

public slots:
  /** GUI action: compact all folders that need to be compacted */
  void compactAll() { compactAllFolders( true ); }

  /** GUI action: expire all folders configured as such */
  void expireAll();

  /** Called from KMFolder::remove when the folderstorage was removed */
  void removeFolderAux(KMFolder* obsoleteFolder, bool success);

  /** Called when the renaming of a folder is done */
  void slotRenameDone( QString newName, bool success );

signals:
  /** Emitted when the list of folders has changed. This signal is a hook
    where clients like the KMFolderTree tree-view can connect. The signal
    is meant to be emitted whenever the code using the folder-manager
    changed things. */
  void changed();

  /** Emitted, when a folder is about to be removed. */
  void folderRemoved(KMFolder*);

  /** Emitted, when a folder has been added. */
  void folderAdded(KMFolder*);

  /** Emitted, when serial numbers for a folder have been invalidated. */
  void folderInvalidated(KMFolder*);

  /** Emitted, when a message has been appended to a folder */
  void msgAdded(KMFolder*, Q_UINT32);

  /** Emitted, when a message has been removed from a folder */
  void msgRemoved(KMFolder*, Q_UINT32);

  /** Emitted, when the status of a message is changed */
  void msgChanged(KMFolder*, Q_UINT32, int delta);

  /** Emitted when a field of the header of a specific message changed. */
  void msgHeaderChanged(KMFolder*, int idx);

protected:

  /** Auxillary function to facilitate removal of a folder */
  void removeFolder(KMFolder* aFolder);

  /** Auxillary function to facilitate removal of a folder directory */
  void removeDirAux(KMFolderDir* aFolderDir);

  QString mBasePath;
  KMFolderRootDir mDir;
  int mQuiet;
  bool mChanged;
  KMFolder* mRemoveOrig;
};

#endif /*kmfoldermgr_h*/
