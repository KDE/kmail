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
#include "kmfoldermgr.h"
#include "kmacctfolder.h"
#include "kmglobal.h"
#include <klocale.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>


//-----------------------------------------------------------------------------
KMFolderMgr::KMFolderMgr(const char* aBasePath)
{
  assert(aBasePath != NULL);
  setBasePath(aBasePath);
}


//-----------------------------------------------------------------------------
KMFolderMgr::~KMFolderMgr()
{
  mBasePath = NULL;
}


//-----------------------------------------------------------------------------
void KMFolderMgr::setBasePath(const char* aBasePath)
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

  mBasePath.detach();

  dir.setPath(mBasePath);
  if (!dir.exists())
  {
    KMFolder fld(&mDir);

    warning("Directory\n"+mBasePath+"\ndoes not exist.\n\n"
	    "KMail will create it now.");
    // dir.mkdir(mBasePath, TRUE);
    mkdir(mBasePath.data(), 0700);

    fld.setName("inbox");
    fld.create();
    fld.close();

    fld.setName("outbox");
    fld.create();
    fld.close();

    fld.setName("trash");
    fld.create();
    fld.close();
  }

  mDir.setPath(mBasePath);
  mDir.reload();
}


//-----------------------------------------------------------------------------
KMAcctFolder* KMFolderMgr::createFolder(const char* fName, bool sysFldr)
{
  return mDir.createFolder(fName, sysFldr);
}


//-----------------------------------------------------------------------------
KMAcctFolder* KMFolderMgr::find(const char* folderName, bool foldersOnly)
{
  KMFolderNode* node;

  for (node=mDir.first(); node; node=mDir.next())
  {
    if (node->isDir() && foldersOnly) continue;
    if (node->name()==folderName) return (KMAcctFolder*)node;
  }
  return NULL;
}


//-----------------------------------------------------------------------------
KMAcctFolder* KMFolderMgr::findOrCreate(const char* aFolderName)
{
  KMAcctFolder* folder = find(aFolderName);

  if (!folder)
  {
    warning(nls->translate("Creating missing folder\n`%s'"), aFolderName);

    folder = createFolder(aFolderName, TRUE);
    if (!folder) fatal(nls->translate("Cannot create folder `%s'\nin %s"),
		       aFolderName, (const char*)mBasePath);
  }
  return folder;
}
