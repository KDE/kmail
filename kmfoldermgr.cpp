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

#include <kapp.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "kmundostack.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include "kmmessage.h"

//-----------------------------------------------------------------------------
KMFolderMgr::KMFolderMgr(const QString& aBasePath):
  KMFolderMgrInherited(), mDir()
{
  initMetaObject();

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

  assert(aBasePath != NULL);

  if (aBasePath[0] == '~')
  {
    mBasePath = QDir::homeDirPath();
    mBasePath.append("/");
    mBasePath.append(aBasePath.mid(1));
  }
  else
  {
    mBasePath = "";
    mBasePath.append(aBasePath);
  }
  

  dir.setPath(mBasePath);
  if (!dir.exists())
  {
    KMessageBox::information(0, i18n("Directory\n%1\ndoes not exist.\n\n"
				  "KMail will create it now.").arg(mBasePath));
    // dir.mkdir(mBasePath, TRUE);
    mkdir(mBasePath.data(), 0700);
    mDir.setPath(mBasePath.local8Bit());
  }

  mDir.setPath(mBasePath.local8Bit());
  mDir.reload();
  emit changed();
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderMgr::createFolder(const QString& fName, bool sysFldr,
				    KMFolderDir *aFolderDir)
{
  KMFolder* fld;
  KMFolderDir *fldDir = aFolderDir;  
 
  if (!aFolderDir)
    fldDir = &mDir;
  fld = fldDir->createFolder(fName, sysFldr);
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
KMFolder* KMFolderMgr::findOrCreate(const QString& aFolderName)
{
  KMFolder* folder = find(aFolderName);

  if (!folder)
  {
    folder = createFolder(aFolderName, TRUE);
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
  QString folderDirLocation = aFolderDir->path();
  KMFolderNode* fN;
  for (fN = aFolderDir->first(); fN != 0; fN = aFolderDir->next()) {
    if (fN->isDir())
      removeDirAux(static_cast<KMFolderDir*>(fN));
    else
      removeFolderAux(static_cast<KMFolder*>(fN));
  }
  aFolderDir->clear();
  aFolderDir->parent()->remove(aFolderDir);
  unlink(folderDirLocation);
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
  createFolderList( str, folders, 0, "" );
  for (unsigned int i = 0; i < str->count() && i < folders->count(); i++)
  {
    if ((*folders->at(i))->isSystemFolder())
      *str->at(i) = i18n(*str->at(i));
  }
}

//-----------------------------------------------------------------------------
void KMFolderMgr::createFolderList(QStringList *str, 
				   QValueList<QGuardedPtr<KMFolder> > *folders,
				   KMFolderDir *adir, 
				   const QString& prefix)
{
  KMFolderNode* cur;
  KMFolderDir* fdir = adir ? adir : &(kernel->folderMgr()->dir());

  for (cur=fdir->first(); cur; cur=fdir->next()) {
    if (cur->isDir())
      continue;

    QGuardedPtr<KMFolder> folder = static_cast<KMFolder*>(cur);
    str->append(prefix + folder->name());
    folders->append( folder );
    if (folder->child())
      createFolderList( str, folders, folder->child(), "  " + prefix );
  }
}

//-----------------------------------------------------------------------------
#include "kmfoldermgr.moc"
