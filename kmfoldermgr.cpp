// kmfoldermgr.cpp

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <sys/types.h>

#ifdef HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <qdir.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kapplication.h>

#include "kmmainwin.h"
#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "undostack.h"
#include "kmmsgdict.h"
#include "folderstorage.h"
#include "renamejob.h"
using KMail::RenameJob;

//-----------------------------------------------------------------------------
KMFolderMgr::KMFolderMgr(const QString& aBasePath, KMFolderDirType dirType):
  QObject(), mDir(this, QString::null, dirType)
{
  if ( dirType == KMStandardDir )
    mDir.setBaseURL( I18N_NOOP("Local") );
  mQuiet = 0;
  mChanged = FALSE;
  setBasePath(aBasePath);
  mRemoveOrig = 0;
}


//-----------------------------------------------------------------------------
KMFolderMgr::~KMFolderMgr()
{
  if  (kmkernel->undoStack())
    kmkernel->undoStack()->clear(); // Speed things up a bit.
  mBasePath = QString::null;
}


//-----------------------------------------------------------------------------
void KMFolderMgr::expireAll() {
  KConfig             *config = KMKernel::config();
  KConfigGroupSaver   saver(config, "General");
  int                 ret = KMessageBox::Continue;

  if (config->readBoolEntry("warn-before-expire", true)) {
    ret = KMessageBox::warningContinueCancel(KMainWindow::memberList->first(),
			 i18n("Are you sure you want to expire old messages?"),
			 i18n("Expire Old Messages?"), i18n("Expire"));
  }

  if (ret == KMessageBox::Continue) {
    expireAllFolders( true /*immediate*/ );
  }

}

#define DO_FOR_ALL(function, folder_code) \
  KMFolderNode* node; \
  QPtrListIterator<KMFolderNode> it(*dir); \
  for ( ; (node = it.current()); ) { \
    ++it; \
    if (node->isDir()) continue; \
    KMFolder *folder = static_cast<KMFolder*>(node); \
    folder_code \
    KMFolderDir *child = folder->child(); \
    if (child) \
       function \
  }

int KMFolderMgr::folderCount(KMFolderDir *dir)
{
  int count = 0;
  if (dir == 0)
    dir = &mDir;
  DO_FOR_ALL(
        {
          count += folderCount( child );
        },
        {
          count++;
        }
  )

  return count;
}



//-----------------------------------------------------------------------------
void KMFolderMgr::compactAllFolders(bool immediate, KMFolderDir* dir)
{
  if (dir == 0)
    dir = &mDir;
  DO_FOR_ALL(
        {
          compactAllFolders( immediate, child );
        },
        {
          if ( folder->needsCompacting() )
              folder->compact( immediate ? KMFolder::CompactNow : KMFolder::CompactLater );
        }
  )
}


//-----------------------------------------------------------------------------
void KMFolderMgr::setBasePath(const QString& aBasePath)
{
  assert(!aBasePath.isNull());

  if (aBasePath[0] == '~')
  {
    mBasePath = QDir::homeDirPath();
    mBasePath.append("/");
    mBasePath.append(aBasePath.mid(1));
  }
  else
    mBasePath = aBasePath;

  QFileInfo info( mBasePath );

  // FIXME We should ask for an alternative dir, rather than bailing out,
  // I guess - till
  if ( info.exists() ) {
   if ( !info.isDir() ) {
      KMessageBox::sorry(0, i18n("'%1' does not appear to be a folder.\n"
                                 "Please move the file out of the way.")
                            .arg( mBasePath ) );
      ::exit(-1);
    }
    if ( !info.isReadable() || !info.isWritable() ) {
      KMessageBox::sorry(0, i18n("The permissions of the folder '%1' are "
                               "incorrect;\n"
                               "please make sure that you can view and modify "
                               "the content of this folder.")
                            .arg( mBasePath ) );
      ::exit(-1);
    }
   } else {
    // ~/Mail (or whatever the user specified) doesn't exist, create it
    if ( ::mkdir( QFile::encodeName( mBasePath ) , S_IRWXU ) == -1 ) {
      KMessageBox::sorry(0, i18n("KMail could not create folder '%1';\n"
                                 "please make sure that you can view and "
                                 "modify the content of the folder '%2'.")
                            .arg( mBasePath ).arg( QDir::homeDirPath() ) );
      ::exit(-1);
    }
  }
  mDir.setPath(mBasePath);
  mDir.reload();
  contentsChanged();
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderMgr::createFolder(const QString& fName, bool sysFldr,
				    KMFolderType aFolderType,
				    KMFolderDir *aFolderDir)
{
  KMFolder* fld;
  KMFolderDir *fldDir = aFolderDir;

  if (!aFolderDir)
    fldDir = &mDir;
  fld = fldDir->createFolder(fName, sysFldr, aFolderType);
  if (fld) {
    if ( fld->id() == 0 )
      fld->setId( createId() );
    contentsChanged();
    emit folderAdded(fld);
    if (kmkernel->filterMgr())
      kmkernel->filterMgr()->folderCreated(fld);
  }

  return fld;
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderMgr::find(const QString& folderName, bool foldersOnly)
{
  KMFolderNode* node;

  for (node=mDir.first(); node; node=mDir.next())
  {
    if (node->isDir() && foldersOnly) continue;
    if (node->name()==folderName) return (KMFolder*)node;
  }
  return 0;
}

//-----------------------------------------------------------------------------
KMFolder* KMFolderMgr::findById(const uint id)
{
  return findIdString( QString::null, id );
}

//-----------------------------------------------------------------------------
KMFolder* KMFolderMgr::findIdString( const QString& folderId,
                                     const uint id,
                                     KMFolderDir *dir )
{
  if (!dir)
    dir = &mDir;

  DO_FOR_ALL(
        {
          KMFolder *folder = findIdString( folderId, id, child );
          if ( folder )
             return folder;
        },
        {
          if ( ( !folderId.isEmpty() && folder->idString() == folderId ) ||
               ( id != 0 && folder->id() == id ) )
             return folder;
        }
  )

  return 0;
}

void KMFolderMgr::getFolderURLS( QStringList& flist, const QString& prefix,
                                 KMFolderDir *adir )
{
  KMFolderDir* dir = adir ? adir : &mDir;

  DO_FOR_ALL(
             {
               getFolderURLS( flist, prefix + "/" + folder->name(), child );
             },
             {
               flist << prefix + "/" + folder->name();
             }
             )
}

KMFolder* KMFolderMgr::getFolderByURL( const QString& vpath,
                                       const QString& prefix,
                                       KMFolderDir *adir )
{
  KMFolderDir* dir = adir ? adir : &mDir;
  DO_FOR_ALL(
        {
          QString a = prefix + "/" + folder->name();
          KMFolder * mfolder = getFolderByURL( vpath, a,child );
          if ( mfolder )
            return mfolder;
        },
        {
          QString comp = prefix + "/" + folder->name();
          if ( comp  == vpath )
            return folder;
        }
  )
  return 0;
}

//-----------------------------------------------------------------------------
KMFolder* KMFolderMgr::findOrCreate(const QString& aFolderName, bool sysFldr,
                                    const uint id)
{
  KMFolder* folder = 0;
  if ( id == 0 )
    folder = find(aFolderName);
  else
    folder = findById(id);

  if (!folder)
  {
    static bool know_type = false;
    static KMFolderType type = KMFolderTypeMaildir;
    if (know_type == false)
    {
      know_type = true;
      KConfig *config = KMKernel::config();
      KConfigGroupSaver saver(config, "General");
      if (config->hasKey("default-mailbox-format"))
      {
        if (config->readNumEntry("default-mailbox-format", 1) == 0)
          type = KMFolderTypeMbox;

      }
    }

    folder = createFolder(aFolderName, sysFldr, type);
    if (!folder) {
      KMessageBox::error(0,(i18n("Cannot create file `%1' in %2.\nKMail cannot start without it.").arg(aFolderName).arg(mBasePath)));
      exit(-1);
    }
    if ( id > 0 )
      folder->setId( id );
  }
  return folder;
}


//-----------------------------------------------------------------------------
void KMFolderMgr::remove(KMFolder* aFolder)
{
  if (!aFolder) return;
  // remember the original folder to trigger contentsChanged later
  if (!mRemoveOrig) mRemoveOrig = aFolder;
  if (aFolder->child())
  {
    // call remove for every child
    KMFolderNode* node;
    QPtrListIterator<KMFolderNode> it(*aFolder->child());
    for ( ; (node = it.current()); )
    {
      ++it;
      if (node->isDir()) continue;
      KMFolder *folder = static_cast<KMFolder*>(node);
      remove(folder);
    }
  }
  emit folderRemoved(aFolder);
  removeFolder(aFolder);
}

void KMFolderMgr::removeFolder(KMFolder* aFolder)
{
  connect(aFolder, SIGNAL(removed(KMFolder*, bool)),
      this, SLOT(removeFolderAux(KMFolder*, bool)));
  aFolder->remove();
}

void KMFolderMgr::removeFolderAux(KMFolder* aFolder, bool success)
{
  if (!success) {
    mRemoveOrig = 0;
    return;
  }

  KMFolderDir* fdir = aFolder->parent();
  KMFolderNode* fN;
  for (fN = fdir->first(); fN != 0; fN = fdir->next()) {
    if (fN->isDir() && (fN->name() == "." + aFolder->fileName() + ".directory")) {
      removeDirAux(static_cast<KMFolderDir*>(fN));
      break;
    }
  }
  aFolder->parent()->remove(aFolder);
  // find the parent folder by stripping "." and ".directory" from the name
  QString parentName = fdir->name();
  parentName = parentName.mid( 1, parentName.length()-11 );
  KMFolderNode* parent = fdir->hasNamedFolder( parentName );
  if ( !parent && fdir->parent() ) // dimap obviously has a different structure
    parent = fdir->parent()->hasNamedFolder( parentName );
  // update the children state
  if ( parent )
    static_cast<KMFolder*>(parent)->storage()->updateChildrenState();
  else
    kdWarning(5006) << "Can not find parent folder" << endl;

  if (aFolder == mRemoveOrig) {
    // call only if we're removing the original parent folder
    contentsChanged();
    mRemoveOrig = 0;
  }
}

void KMFolderMgr::removeDirAux(KMFolderDir* aFolderDir)
{
  QDir dir;
  QString folderDirLocation = aFolderDir->path();
  aFolderDir->clear();
  aFolderDir->parent()->remove(aFolderDir);
  dir.rmdir(folderDirLocation);
}

//-----------------------------------------------------------------------------
KMFolderRootDir& KMFolderMgr::dir(void)
{
  return mDir;
}


//-----------------------------------------------------------------------------
void KMFolderMgr::contentsChanged(void)
{
  if (mQuiet) mChanged = TRUE;
  else emit changed();
}


//-----------------------------------------------------------------------------
void KMFolderMgr::reload(void)
{
}

//-----------------------------------------------------------------------------
void KMFolderMgr::createFolderList(QStringList *str,
				   QValueList<QGuardedPtr<KMFolder> > *folders)
{
  createFolderList( str, folders, 0, "" );
}

//-----------------------------------------------------------------------------
void KMFolderMgr::createI18nFolderList(QStringList *str,
				   QValueList<QGuardedPtr<KMFolder> > *folders)
{
  createFolderList( str, folders, 0, QString::null, true );
}

//-----------------------------------------------------------------------------
void KMFolderMgr::createFolderList(QStringList *str,
				   QValueList<QGuardedPtr<KMFolder> > *folders,
				   KMFolderDir *adir,
				   const QString& prefix,
				   bool i18nized)
{
  KMFolderDir* dir = adir ? adir : &mDir;

  DO_FOR_ALL(
        {
          createFolderList(str, folders, child, "  " + prefix, i18nized );
        },
        {
          if (i18nized)
            str->append(prefix + folder->label());
          else
            str->append(prefix + folder->name());
          folders->append( folder );
        }
  )
}

//-----------------------------------------------------------------------------
void KMFolderMgr::syncAllFolders( KMFolderDir *adir )
{
  KMFolderDir* dir = adir ? adir : &mDir;
  DO_FOR_ALL(
             {
               syncAllFolders(child);
             },
             {
               if (folder->isOpened())
	         folder->sync();
             }
  )
}


//-----------------------------------------------------------------------------
/**
 * Check each folder in turn to see if it is configured to
 * AutoExpire. If so, expire old messages.
 *
 * Should be called with 0 first time around.
 */
void KMFolderMgr::expireAllFolders(bool immediate, KMFolderDir *adir) {
  KMFolderDir   *dir = adir ? adir : &mDir;

  DO_FOR_ALL(
             {
               expireAllFolders(immediate, child);
             },
             {
               if (folder->isAutoExpire()) {
                 folder->expireOldMessages( immediate );
               }
             }
  )
}

//-----------------------------------------------------------------------------
void KMFolderMgr::invalidateFolder(KMMsgDict *dict, KMFolder *folder)
{
    unlink(QFile::encodeName(folder->indexLocation()) + ".sorted");
    unlink(QFile::encodeName(folder->indexLocation()) + ".ids");
    if (dict) {
	folder->fillMsgDict(dict);
	dict->writeFolderIds(folder);
    }
    emit folderInvalidated(folder);
}

//-----------------------------------------------------------------------------
void KMFolderMgr::readMsgDict(KMMsgDict *dict, KMFolderDir *dir, int pass)
{
  bool atTop = false;
  if (!dir) {
    dir = &mDir;
    atTop = true;
  }

  DO_FOR_ALL(
             {
               readMsgDict(dict, child, pass);
             },
             {
	       if (pass == 1) {
                 dict->readFolderIds(folder);
               } else if (pass == 2) {
                 if (!dict->hasFolderIds(folder)) {
		   invalidateFolder(dict, folder);
                 }
               }
             }
  )

  if (pass == 1 && atTop)
    readMsgDict(dict, dir, pass + 1);
}

//-----------------------------------------------------------------------------
void KMFolderMgr::writeMsgDict(KMMsgDict *dict, KMFolderDir *dir)
{
  if (!dir)
    dir = &mDir;

  DO_FOR_ALL(
             {
               writeMsgDict(dict, child);
             },
             {
               folder->writeMsgDict(dict);
             }
  )
}

//-----------------------------------------------------------------------------
void KMFolderMgr::quiet(bool beQuiet)
{
  if (beQuiet)
    mQuiet++;
  else {
    mQuiet--;
    if (mQuiet <= 0)
    {
      mQuiet = 0;
      if (mChanged) emit changed();
      mChanged = FALSE;
    }
  }
}

//-----------------------------------------------------------------------------
void KMFolderMgr::tryReleasingFolder(KMFolder* f, KMFolderDir* adir)
{
  KMFolderDir* dir = adir ? adir : &mDir;
  DO_FOR_ALL(
             {
               tryReleasingFolder(f, child);
             },
             {
               if (folder->isOpened())
	         folder->storage()->tryReleasingFolder(f);
             }
  )
}

//-----------------------------------------------------------------------------
uint KMFolderMgr::createId()
{
  int newId;
  do
  {
    newId = kapp->random();
  } while ( findById( newId ) != 0 );

  return newId;
}

//-----------------------------------------------------------------------------
void KMFolderMgr::renameFolder( KMFolder* folder, const QString& newName, 
                                KMFolderDir *newParent )
{
  RenameJob* job = new RenameJob( folder->storage(), newName, newParent );
  connect( job, SIGNAL( renameDone( QString, bool ) ),
      this, SLOT( slotRenameDone( QString, bool ) ) );
  job->start();
}

//-----------------------------------------------------------------------------
void KMFolderMgr::slotRenameDone( QString, bool success )
{
  kdDebug(5006) << k_funcinfo << success << endl;
}

#include "kmfoldermgr.moc"
