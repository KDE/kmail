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
#include <kmessagebox.h>


//=============================================================================
//=============================================================================
KMFolderRootDir::KMFolderRootDir(const QString& path):
  KMFolderDir(NULL, path)
{

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
void KMFolderRootDir::setPath(const QString& aPath)
{
  mPath = aPath;

}


//-----------------------------------------------------------------------------
QString KMFolderRootDir::path() const
{
  return mPath;
}



//=============================================================================
//=============================================================================
KMFolderDir::KMFolderDir(KMFolderDir* parent, const QString& name):
  KMFolderNode(parent,name), KMFolderNodeList()
{

  setAutoDelete(TRUE);

  mType = "dir";
}


//-----------------------------------------------------------------------------
KMFolderDir::~KMFolderDir()
{
  clear();
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderDir::createFolder(const QString& aFolderName, bool aSysFldr)
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
    QString wmsg = i18n("Error while creating file `%1':\n%2").arg(aFolderName).arg(strerror(rc));
    KMessageBox::information(0,wmsg );
    delete fld;
    return NULL;
  }

  KMFolderNode* fNode;
  int index = 0;
  for (fNode=first(); fNode; fNode=next()) {
    if (fNode->name().lower() > fld->name().lower()) {
      insert( index, fld );
      break;
    }
    ++index;
  }

  if (!fNode)
    append(fld);

  fld->correctUnreadMsgsCount();
  return fld;
}


//----------------------------------------------------------------------------
QString KMFolderDir::path() const
{
  static QString p;

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
bool KMFolderDir::reload(void)
{
  QDir      dir;
  KMFolder* folder;
  QFileInfo* fileInfo;
  QFileInfoList* fiList;
  QStringList diList;
  QList<KMFolder> folderList;
  QString fname;
  QString fldPath;

  clear();

  fldPath = path();
  dir.setFilter(QDir::Files | QDir::Dirs | QDir::Hidden);
  dir.setNameFilter("*");

  if (!dir.cd(fldPath, TRUE))
  {
    QString msg = i18n("Cannot enter directory '%1'.\n").arg(fldPath);
    KMessageBox::information(0, msg );
    return FALSE;
  }

  if (!(fiList=(QFileInfoList*)dir.entryInfoList()))
  {
    QString msg = i18n("Directory '%1' is unreadable.\n").arg(fldPath);
    KMessageBox::information(0,msg);
    return FALSE;
  }

  for (fileInfo=fiList->first(); fileInfo; fileInfo=fiList->next())
  {
    fname = fileInfo->fileName();

    if ((fname[0]=='.') &&
	!(fname.right(10)==".directory"))
      continue;
    else if (fname == ".directory")
      continue;
    else if (fileInfo->isDir()) // a directory
      diList.append(fname);

    else // all other files are folders (at the moment ;-)
    {
      folder = new KMFolder(this, fname);
      append(folder);
      folderList.append(folder);
    }
  }

  for (folder=folderList.first(); folder; folder=folderList.next())
  {
    for(QStringList::Iterator it = diList.begin();
	it != diList.end();
	++it)
      if (*it == "." + folder->name() + ".directory") {
	KMFolderDir* folderDir = new KMFolderDir(this, *it);
	folderDir->reload();
	append(folderDir);
	folder->setChild(folderDir);
	break;
      }
  }

  return TRUE;
}

KMFolderNode* KMFolderDir::hasNamedFolder(const QString& aName)
{
  KMFolderNode* fNode;
  for (fNode=first(); fNode; fNode=next()) {
    if (fNode->name() == aName)
      return fNode;
  }
  return 0;
}

#include "kmfolderdir.moc"

