// kmfoldermgr.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <qdir.h>
#include <assert.h>
#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include <kapp.h>
#include <klocale.h>


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
  mBasePath = QString::null;;
}


//-----------------------------------------------------------------------------
void KMFolderMgr::compactAll()
{
  KMFolderNode* node;
  for (node=mDir.first(); node; node=mDir.next())
  {
    if (node->isDir()) continue;
    ((KMFolder*)node)->compact(); // compact know if it's needed
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
    mBasePath.append(aBasePath+1);
  }
  else
  {
    mBasePath = "";
    mBasePath.append(aBasePath);
  }
  

  dir.setPath(mBasePath);
  if (!dir.exists())
  {
    KMFolder fld(&mDir);

    warning("Directory\n"+mBasePath+"\ndoes not exist.\n\n"
	    "KMail will create it now.");
    // dir.mkdir(mBasePath, TRUE);
    mkdir(mBasePath.data(), 0700);
    mDir.setPath(mBasePath.local8Bit());

    fld.setName("inbox");
    fld.create();
    fld.close();

    fld.setName("outbox");
    fld.create();
    fld.close();

    fld.setName("sent-mail");
    fld.create();
    fld.close();

    fld.setName("trash");
    fld.create();
    fld.close();
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
  if (fld) emit changed();

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
    // Are these const char* casts really necessary? -sanders
    warning(i18n("Creating missing folder\n`%s'"), (const char*)aFolderName);

    folder = createFolder(aFolderName, TRUE);
    if (!folder) fatal(i18n("Cannot create folder `%s'\nin %s"),
		       (const char*)aFolderName, (const char*)mBasePath);
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
  if (filterMgr) filterMgr->folderRemoved(aFolder,NULL);
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
void KMFolderMgr::createFolderList( QStringList *str, 
				    QList<KMFolder> *folders )
{
  createFolderList( str, folders, 0, "" );
}

//-----------------------------------------------------------------------------
void KMFolderMgr::createFolderList( QStringList *str, 
				    QList<KMFolder> *folders,
				    KMFolderDir *adir, 
				    const QString& prefix)
{
  KMFolderNode* cur;
  KMFolderDir* fdir = adir ? adir : &(folderMgr->dir());

  for (cur=fdir->first(); cur; cur=fdir->next()) {
    if (cur->isDir())
      continue;

    KMFolder* folder = static_cast<KMFolder*>(cur);
    str->append(prefix + folder->name());
    folders->append( folder );
    if (folder->child())
      createFolderList( str, folders, folder->child(), "  " + prefix );
  }
}

//-----------------------------------------------------------------------------
#include "kmfoldermgr.moc"
