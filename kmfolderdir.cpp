// kmfolderdir.cpp

#include "kmfolderdir.h"
#include "kmacctfolder.h"

#include <assert.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinf.h>


//-----------------------------------------------------------------------------
KMFolderRootDir::KMFolderRootDir(const char* path):
  KMFolderDir(NULL, ".")
{
  setPath(path);
}


//-----------------------------------------------------------------------------
void KMFolderRootDir::setPath(const char* aPath)
{
  mPath = aPath;
  mPath.detach();
}


//-----------------------------------------------------------------------------
const QString KMFolderRootDir::path(void) const
{
  return mPath;
}



//=============================================================================
KMFolderDir::KMFolderDir(KMFolderDir* parent, const char* name):
  KMFolderNode(parent,name), KMFolderNodeList()
{
  setAutoDelete(TRUE);
}


//-----------------------------------------------------------------------------
KMFolderDir::~KMFolderDir()
{
  clear();
}


//-----------------------------------------------------------------------------
bool KMFolderDir::reload(void)
{
  QDir      dir;
  KMFolderDir* folderDir;
  KMAcctFolder* folder;
  QFileInfo* fileInfo;
  QFileInfoList* fiList;
  QString fname;
  QString fldPath;

  clear();
  
  fldPath = path();
  //dir.setFilter(QDir::Files | QDir::Hidden);
  dir.setNameFilter("*");
  
  if (!dir.cd(fldPath, TRUE))
  {
    warning("Cannot enter directory '" + fldPath + "'.\n");
    return FALSE;
  }

  if (!(fiList=(QFileInfoList*)dir.entryInfoList()))
  {
    warning("Directory '" + fldPath + "' is unreadable.\n");
    return FALSE;
  }

  for (fileInfo=fiList->first(); fileInfo; fileInfo=fiList->next())
  {
    fname = fileInfo->fileName();

    if (fname[0]=='.') // skip table of contents files
      continue;

    else if (fileInfo->isDir()) // a directory
    {
      folderDir = new KMFolderDir(this, fname);
      append(folderDir);
    }

    else // all other files are folders (at the moment ;-)
    {
      folder = new KMAcctFolder(this, fname);
      folder->readHeader();
      append(folder);
    }
  }
  //delete fiList;

  return TRUE;
}
