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

#include "kmmainwin.h"
#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "undostack.h"
#include "kmmsgdict.h"

//-----------------------------------------------------------------------------
KMFolderMgr::KMFolderMgr(const QString& aBasePath, KMFolderDirType dirType):
  KMFolderMgrInherited(), mDir(this, QString::null, dirType)
{
  mQuiet = 0;
  mChanged = FALSE;
  setBasePath(aBasePath);
}


//-----------------------------------------------------------------------------
KMFolderMgr::~KMFolderMgr()
{
  if  (kernel->undoStack())
    kernel->undoStack()->clear(); // Speed things up a bit.
  mBasePath = QString::null;;
}


//-----------------------------------------------------------------------------
void KMFolderMgr::compactAll()
{
  compactAllAux( &mDir );
}

//-----------------------------------------------------------------------------
void KMFolderMgr::expireAll() {
  KConfig             *config = KMKernel::config();
  KConfigGroupSaver   saver(config, "General");
  int                 ret = KMessageBox::Continue;

  if (config->readBoolEntry("warn-before-expire")) {
    ret = KMessageBox::warningContinueCancel(KMainWindow::memberList->first(),
			 i18n("Are you sure you want to expire old messages?"),
			 i18n("Expire old messages?"), i18n("Expire"));
  }

  if (ret == KMessageBox::Continue) {
    expireAllFolders(0);
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
void KMFolderMgr::compactAllAux(KMFolderDir* dir)
{
  DO_FOR_ALL(
        {
          compactAllAux(child);
        },
        {
          folder->compact(); // compact now if it's needed
          emit progress();
        }
  )
}


//-----------------------------------------------------------------------------
void KMFolderMgr::setBasePath(const QString& aBasePath)
{
  QDir dir;

  assert(!aBasePath.isNull());

  if (aBasePath[0] == '~')
  {
    mBasePath = QDir::homeDirPath();
    mBasePath.append("/");
    mBasePath.append(aBasePath.mid(1));
  }
  else
    mBasePath = aBasePath;


  dir.setPath(mBasePath);
  if (!dir.exists())
  {
    // FIXME: mkdir can fail!
    mkdir(QFile::encodeName(mBasePath), 0700);
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
    contentsChanged();
    emit folderAdded(fld);
    if (kernel->filterMgr())
      kernel->filterMgr()->folderCreated(fld);
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
KMFolder* KMFolderMgr::findIdString(const QString& folderId, KMFolderDir *dir)
{
  if (!dir)
    dir = static_cast<KMFolderDir*>(&mDir);

  DO_FOR_ALL(
        {
          KMFolder *folder = findIdString( folderId, child);
          if (folder)
             return folder;
        },
        {
          if (folder->idString() == folderId)
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
KMFolder* KMFolderMgr::findOrCreate(const QString& aFolderName, bool sysFldr)
{
  KMFolder* folder = find(aFolderName);

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
  }
  return folder;
}


//-----------------------------------------------------------------------------
void KMFolderMgr::remove(KMFolder* aFolder)
{
  assert(aFolder != 0);

  emit folderRemoved(aFolder);
  removeFolderAux(aFolder);

  contentsChanged();
}

void KMFolderMgr::removeFolderAux(KMFolder* aFolder)
{
  KMFolderDir* fdir = aFolder->parent();
  KMFolderNode* fN;
  for (fN = fdir->first(); fN != 0; fN = fdir->next())
    if (fN->isDir() && (fN->name() == "." + aFolder->fileName() + ".directory")) {
      removeDirAux(static_cast<KMFolderDir*>(fN));
      break;
    }
  aFolder->remove();
  aFolder->parent()->remove(aFolder);
  //  mDir.remove(aFolder);
  if (kernel->filterMgr()) kernel->filterMgr()->folderRemoved(aFolder,0);
}

void KMFolderMgr::removeDirAux(KMFolderDir* aFolderDir)
{
  QDir dir;
  QString folderDirLocation = aFolderDir->path();
  KMFolderNode* fN;
  for (fN = aFolderDir->first(); fN != 0; fN = aFolderDir->first()) {
    if (fN->isDir())
      removeDirAux(static_cast<KMFolderDir*>(fN));
    else
      removeFolderAux(static_cast<KMFolder*>(fN));
  }
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
void KMFolderMgr::expireAllFolders(KMFolderDir *adir) {
  KMFolderDir   *dir = adir ? adir : &mDir;

  DO_FOR_ALL(
             {
               expireAllFolders(child);
             },
             {
               if (folder->isAutoExpire()) {
                 folder->expireOldMessages();
               }
               emit progress();
             }
  )
}

//-----------------------------------------------------------------------------
void KMFolderMgr::invalidateFolder(KMMsgDict *dict, KMFolder *folder)
{
    unlink(folder->indexLocation().local8Bit() + ".sorted");
    unlink(folder->indexLocation().local8Bit() + ".ids");
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
#include "kmfoldermgr.moc"
