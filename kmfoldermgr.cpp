// kmfoldermgr.cpp
// $Id$

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
#include <kmmainwin.h>
#include <kapplication.h>

#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "kmundostack.h"

//-----------------------------------------------------------------------------
KMFolderMgr::KMFolderMgr(const QString& aBasePath, bool aImap):
  KMFolderMgrInherited(), mDir(QString::null, aImap)
{
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
  KConfig             *config = kapp->config();
  KConfigGroupSaver   saver(config, "General");
  int                 ret = KMessageBox::Continue;

  if (config->readBoolEntry("warn-before-expire")) {
    ret = KMessageBox::warningContinueCancel(KMainWindow::memberList->first(),     
			 i18n("Are you sure you want to expire old messages?"),
			 i18n("Expire old messages?"), i18n("Expire"));
  }
     
  if (ret == KMessageBox::Continue) {
    expireAllFolders(NULL);
  }
    
}


//-----------------------------------------------------------------------------
void KMFolderMgr::compactAllAux(KMFolderDir* dir)
{
  KMFolderNode* node;
  if (dir == 0)
    return;
  for (node = dir->first(); node; node = dir->next())
  {
    if (node->isDir()) {
      KMFolderDir *child = static_cast<KMFolderDir*>(node);
      compactAllAux( child );
    }
    else
      ((KMFolder*)node)->compact(); // compact now if it's needed
  }
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
    KMessageBox::information(0, i18n("Directory\n%1\ndoes not exist.\n\n"
				  "KMail will create it now.").arg(mBasePath));
    // FIXME: mkdir can fail!
    mkdir(QFile::encodeName(mBasePath), 0700);
    mDir.setPath(mBasePath);
  }

  mDir.setPath(mBasePath);
  mDir.reload();
  emit changed();
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
    emit changed();
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
  return NULL;
}

//-----------------------------------------------------------------------------
KMFolder* KMFolderMgr::findIdString(const QString& folderId, KMFolderDir *dir)
{
  KMFolderNode* node;
  KMFolder* folder;
  if (!dir)
    dir = static_cast<KMFolderDir*>(&mDir);

  for (node=dir->first(); node; node=dir->next())
  {
    if (node->isDir()) {
      folder = findIdString( folderId, static_cast<KMFolderDir*>(node) );
      if (folder)
	return folder;
    }
    else {
      folder = static_cast<KMFolder*>(node);
      if (folder->idString()==folderId)
	return folder;
    }
  }
  return 0;
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderMgr::findOrCreate(const QString& aFolderName, bool sysFldr)
{
  KMFolder* folder = find(aFolderName);

  if (!folder)
  {
    static bool know_type = false;
    static KMFolderType type = KMFolderTypeMbox;
    if (know_type == false)
    {
      know_type = true;
      KConfig *config = kapp->config();
      KConfigGroupSaver saver(config, "General");
      if (config->hasKey("default-mailbox-format"))
      {
        if (config->readNumEntry("default-mailbox-format", 0) == 1)
          type = KMFolderTypeMaildir;
          
      }
      else
      {
        QCString MAIL(getenv("MAIL"));
        if (!MAIL.isEmpty() && !MAIL.isNull())
        {
          // if the contents of $MAIL is a directory, then we likely want
          // Maildir as our default
          QFileInfo info(MAIL);
          if (info.isDir())
            type = KMFolderTypeMaildir;
        }
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
  assert(aFolder != NULL);

  emit removed(aFolder);
  removeFolderAux(aFolder);

  emit changed();
}

void KMFolderMgr::removeFolderAux(KMFolder* aFolder)
{
  KMFolderDir* fdir = aFolder->parent();
  KMFolderNode* fN;
  for (fN = fdir->first(); fN != 0; fN = fdir->next())
    if (fN->isDir() && (fN->name() == "." + aFolder->name() + ".directory")) {
      removeDirAux(static_cast<KMFolderDir*>(fN));
      break;
    }
  aFolder->remove();
  aFolder->parent()->remove(aFolder);
  //  mDir.remove(aFolder);
  if (kernel->filterMgr()) kernel->filterMgr()->folderRemoved(aFolder,NULL);
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
  emit changed();
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
  KMFolderNode* cur;
  KMFolderDir* fdir = adir ? adir : &mDir;

  for (cur=fdir->first(); cur; cur=fdir->next()) {
    if (cur->isDir())
      continue;

    QGuardedPtr<KMFolder> folder = static_cast<KMFolder*>(cur);
    if (i18nized)
      str->append(prefix + folder->label());
    else
      str->append(prefix + folder->name());
    folders->append( folder );
    if (folder->child())
      createFolderList( str, folders, folder->child(), "  " + prefix,
        i18nized );
  }
}

//-----------------------------------------------------------------------------
void KMFolderMgr::syncAllFolders( KMFolderDir *adir )
{
  KMFolderNode* cur;
  KMFolderDir* fdir = adir ? adir : &mDir;

  for (cur=fdir->first(); cur; cur=fdir->next()) {
    if (cur->isDir())
      continue;

    KMFolder *folder = static_cast<KMFolder*>(cur);  
    if (folder->isOpened())
	folder->sync();
    if (folder->child())
      syncAllFolders( folder->child() );
  }
}


//-----------------------------------------------------------------------------
/**
 * Check each folder in turn to see if it is configured to
 * AutoExpire. If so, expire old messages.
 *
 * Should be called with NULL first time around.
 */
void KMFolderMgr::expireAllFolders(KMFolderDir *adir) {
  KMFolderNode  *cur = NULL;
  KMFolderDir   *fdir = adir ? adir : &mDir;

  for (cur = fdir->first(); cur; cur = fdir->next()) {
    if (cur->isDir()) {
	  continue;
    }

    KMFolder *folder = static_cast<KMFolder*>(cur);
    if (folder->isAutoExpire()) {
      folder->expireOldMessages();
    }
    if (folder->child()) {
      expireAllFolders( folder->child() );
    }
  }
}

//-----------------------------------------------------------------------------
#include "kmfoldermgr.moc"
