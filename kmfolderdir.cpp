// kmfolderdir.cpp

#include <config.h>
#include <qdir.h>

#include "kmfolderdir.h"
#include "kmfoldermaildir.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"

#include <assert.h>
#include <errno.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kstandarddirs.h>


//=============================================================================
//=============================================================================
KMFolderRootDir::KMFolderRootDir(const QString& path, bool imap):
  KMFolderDir(0, path, imap)
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
KMFolderDir::KMFolderDir(KMFolderDir* parent, const QString& name, bool imap):
  KMFolderNode(parent,name), KMFolderNodeList()
{

  setAutoDelete(TRUE);

  mType = "dir";
  mImap = imap;
}


//-----------------------------------------------------------------------------
KMFolderDir::~KMFolderDir()
{
  clear();
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderDir::createFolder(const QString& aFolderName, bool aSysFldr, KMFolderType aFolderType)
{
  KMFolder* fld;
  int rc;

  assert(!aFolderName.isEmpty());
  if (mImap)
    fld = new KMFolderImap(this, aFolderName);
  else if (aFolderType == KMFolderTypeCachedImap )
    fld = new KMFolderCachedImap(this, aFolderName);
  else if (aFolderType == KMFolderTypeMaildir)
    fld = new KMFolderMaildir(this, aFolderName);
  else
    fld = new KMFolderMbox(this, aFolderName);
  assert(fld != 0);

  fld->setSystemFolder(aSysFldr);

  rc = fld->create(mImap);
  if (rc)
  {
    QString wmsg = i18n("Error while creating file `%1':\n%2").arg(aFolderName).arg(strerror(rc));
    KMessageBox::information(0,wmsg );
    delete fld;
    fld = 0;
    return 0;
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
  QDir               dir;
  KMFolder*          folder;
  QFileInfo*         fileInfo;
  QFileInfoList*     fiList;
  QStringList        diList;
  QPtrList<KMFolder> folderList;
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
    {
      QString maildir(fname + "/new");
      QString imapcachefile = QString::fromLatin1(".%1.uidcache").arg(fname);

      // For this to be a cached IMAP folder, it must be in the KMail imap
      // subdir and must be have a uidcache file or be a maildir folder
      if( path().startsWith( locateLocal("appdata", "imap") )
	  && ( dir.exists( imapcachefile) || dir.exists( maildir ) ) )
      {
	kdDebug() << "KMFolderDir creating new CachedImap folder with name " << fname << endl;
	folder = new KMFolderCachedImap(this, fname);
        append(folder);
        folderList.append(folder);
      } else {
	kdDebug() << fname << " is *not* a cached imap dir" << endl;
	// see if this is a maildir before assuming a subdir
	if (!mImap && dir.exists(maildir))
	{
	  folder = new KMFolderMaildir(this, fname);
	  append(folder);
	  folderList.append(folder);
	}
	else
	  diList.append(fname);
      }
    }
    else if (mImap)
    {
      if (KMFolderImap::encodeFileName(KMFolderImap::decodeFileName(fname))
          == fname)
      {
        folder = new KMFolderImap(this, KMFolderImap::decodeFileName(fname));
        append(folder);
        folderList.append(folder);
      }
    }
    else // all other files are folders (at the moment ;-)
    {
      folder = new KMFolderMbox(this, fname);
      append(folder);
      folderList.append(folder);
    }
  }

  for (folder=folderList.first(); folder; folder=folderList.next())
  {
    for(QStringList::Iterator it = diList.begin();
        it != diList.end();
        ++it)
      if (*it == "." + folder->fileName() + ".directory")
      {
        KMFolderDir* folderDir = new KMFolderDir(this, *it, mImap);
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

