// kmfolderdir.cpp

#include <qdir.h>

#include "kmfolderdir.h"
#include "kmfolder.h"
#include <kapp.h>

#include <assert.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <errno.h>
#include <klocale.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>


//=============================================================================
//=============================================================================
KMFolderRootDir::KMFolderRootDir(const QCString& path):
  KMFolderDir(NULL, path)
{
  initMetaObject();
  setPath(path);
}

//-----------------------------------------------------------------------------
KMFolderRootDir::~KMFolderRootDir()
{
  // WABA: We can't let KMFolderDir do this because by the time its
  // desctructor gets called, KMFolderRootDir is already destructed
  // Most notably the path.
  clear();
}

//-----------------------------------------------------------------------------
void KMFolderRootDir::setPath(const QCString& aPath)
{
  mPath = aPath;  
}


//-----------------------------------------------------------------------------
const QCString KMFolderRootDir::path() const
{
  return mPath;
}



//=============================================================================
//=============================================================================
KMFolderDir::KMFolderDir(KMFolderDir* parent, const QCString& name):
  KMFolderNode(parent,name), KMFolderNodeList()
{
  initMetaObject();
  setAutoDelete(TRUE);
  mType = "dir";
}


//-----------------------------------------------------------------------------
KMFolderDir::~KMFolderDir()
{
  clear();
}


//-----------------------------------------------------------------------------
bool KMFolderDir::createDirectory(const QCString& aDirName)
{
  int rc;
  QCString p;

  assert(!aDirName.isEmpty());

  p = path() + '/' + aDirName;
  if (mkdir((const char*)p, 0700) != 0)
  {
    warning(i18n("Cannot create directory %s:\n%s"),
	    (const char*)p, strerror(errno));
    return false;
  }

  return true;
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderDir::createFolder(const QCString& aFolderName, bool aSysFldr)
{
  KMFolder* fld;
  int rc;

  assert(!aFolderName.isEmpty());

  fld = new KMFolder(this, aFolderName);
  assert(fld != NULL);

  fld->setSystemFolder(aSysFldr);

  rc = fld->create();
  if (rc)
  {
    warning(i18n("Error while creating folder `%s':\n%s"),
	    (const char*)aFolderName, strerror(rc));
    delete fld;
    return NULL;
  }

  append(fld);
  return fld;
}


//-----------------------------------------------------------------------------
int KMFolderDir::rename(const QCString& aName)
{
  QCString oldPath = path() + '/' + name();
  QCString newPath = path() + '/' + aName;
  int rc = ::rename(oldPath, newPath);
  if (!rc)
  {
    setName(aName);
    return 0;
  }
  return errno;
}


//-----------------------------------------------------------------------------
const QCString KMFolderDir::path() const
{
  QCString p;

  if (parent())
  {
    p = parent()->path();
    p.append("/");
    p.append(name());
  }
  else p = "";

  return p;
}


//-----------------------------------------------------------------------------
bool KMFolderDir::reload()
{
  clear();
  return loadFolders();
}


//-----------------------------------------------------------------------------
bool KMFolderDir::loadFolders(const QCString aPath, int aDepth)
{
  QDir dir;
  KMFolder* folder;
  QFileInfo* fileInfo;
  QFileInfoList* fiList;
  QCString p, fname;

  if (aDepth >= 10) return false;

  if (aPath.isEmpty()) p = path();
  else p = path() + '/' + aPath;

  dir.setNameFilter("*");
  if (!dir.cd(p, true))
  {
    warning(i18n("Cannot enter directory %s."), (const char*)p);
    return false;
  }

  if (!(fiList=(QFileInfoList*)dir.entryInfoList()))
  {
    warning(i18n("Directory %s/%s is unreadable."), (const char*)p);
    return false;
  }

  for (fileInfo=fiList->first(); fileInfo; fileInfo=fiList->next())
  {
    fname = fileInfo->fileName();

    if (fname[0]=='.') // skip administrative files
      continue;

    else if (fileInfo->isDir()) // a directory
    {
      if (!aPath.isEmpty()) fname = aPath + '/' + fname;
      append(new KMFolderDir(this, fname));
      loadFolders(fname, aDepth+1);
    }

    else // all other files are considered folders
    {
      if (!aPath.isEmpty()) fname = aPath + '/' + fname;
      folder = new KMFolder(this, fname);
      append(folder);
      debug("folder found: %s", (const char*)folder->location());
    }
  }
  return true;
}


#include "kmfolderdir.moc"
