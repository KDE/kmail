// kmfoldermgr.cpp

#include <qdir.h>
#include <assert.h>
#include "kmfoldermgr.h"

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
    warning("Directory\n"+mBasePath+"\ndoes not exist.\n\n"
	    "KMail will create it now.");
    dir.mkdir(mBasePath, TRUE);
  }

  mDir.setPath(mBasePath);
  mDir.reload();
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderMgr::find(const char* folderName, bool foldersOnly)
{
  KMFolderNode* node;

  for (node=mDir.first(); node; node=mDir.next())
  {
    if (node->isDir() && foldersOnly) continue;
    if (node->name()==folderName) return (KMFolder*)node;
  }
  return NULL;
}
