// kmfolderdir.cpp

#include <config.h>
#include <qdir.h>

#include "kmfolderdir.h"
#include "kmfoldersearch.h"
#include "kmfoldercachedimap.h"
#include "kmfolder.h"

#include <assert.h>
#include <errno.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kstandarddirs.h>


//=============================================================================
//=============================================================================
KMFolderRootDir::KMFolderRootDir(KMFolderMgr* manager, const QString& path,
                                 KMFolderDirType dirType)
  : KMFolderDir( 0, 0, path, dirType ),
    mPath( path ),
    mManager( manager )
{
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


//-----------------------------------------------------------------------------
QString KMFolderRootDir::prettyURL() const
{
  if ( !mBaseURL.isEmpty() )
    return i18n( mBaseURL.data() );
  else
    return QString::null;
}


//-----------------------------------------------------------------------------
void KMFolderRootDir::setBaseURL( const QCString &baseURL )
{
  mBaseURL = baseURL;
}


//-----------------------------------------------------------------------------
KMFolderMgr* KMFolderRootDir::manager() const
{
  return mManager;
}


//=============================================================================
//=============================================================================
KMFolderDir::KMFolderDir( KMFolder * owner, KMFolderDir* parent,
                          const QString& name, KMFolderDirType dirType )
  : KMFolderNode( parent, name ), KMFolderNodeList(),
    mOwner( owner ), mDirType( dirType )
{
  setAutoDelete( true );
  setType( "dir" );
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
  if (mDirType == KMImapDir)
    fld = new KMFolder( this, aFolderName, KMFolderTypeImap );
  else
    fld = new KMFolder( this, aFolderName, aFolderType );
  assert(fld != 0);

  fld->setSystemFolder(aSysFldr);

  rc = fld->create(mDirType == KMImapDir);
  if (rc)
  {
    QString msg = i18n("<qt>Error while creating file <b>%1</b>:<br>%2</qt>").arg(aFolderName).arg(strerror(rc));
    KMessageBox::information(0, msg);
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


//----------------------------------------------------------------------------
QString KMFolderDir::label() const
{
  if ( mOwner )
    return mOwner->label();
  else
    return QString::null;
}


//-----------------------------------------------------------------------------
QString KMFolderDir::prettyURL() const
{
  QString parentUrl;
  if ( parent() )
    parentUrl = parent()->prettyURL();
  if ( !parentUrl.isEmpty() )
    return parentUrl + '/' + label();
  else
    return label();
}


//-----------------------------------------------------------------------------
bool KMFolderDir::reload(void)
{
  QDir               dir;
  KMFolder*          folder;
  QFileInfo*         fileInfo;
  QStringList        diList;
  QPtrList<KMFolder> folderList;

  clear();

  const QString fldPath = path();
  dir.setFilter(QDir::Files | QDir::Dirs | QDir::Hidden);
  dir.setNameFilter("*");

  if (!dir.cd(fldPath, TRUE))
  {
    QString msg = i18n("<qt>Cannot enter folder <b>%1</b>.</qt>").arg(fldPath);
    KMessageBox::information(0, msg);
    return FALSE;
  }

  QFileInfoList* fiList=(QFileInfoList*)dir.entryInfoList();
  if (!fiList)
  {
    QString msg = i18n("<qt>Folder <b>%1</b> is unreadable.</qt>").arg(fldPath);
    KMessageBox::information(0, msg);
    return FALSE;
  }

  for (fileInfo=fiList->first(); fileInfo; fileInfo=fiList->next())
  {
    const QString fname = fileInfo->fileName();
    if( ( fname[0] == '.' ) && !fname.endsWith( ".directory" ) ) {
      // ignore all hidden files except our subfolder containers
      continue;
    }
    if( fname == ".directory" ) {
      // ignore .directory files (not created by us)
      continue;
    }
    // Collect subdirectories.
    if ( fileInfo->isDir() &&
         fname.startsWith( "." ) && fname.endsWith( ".directory" ) ) {
       diList.append(fname);
       continue;
    }

    if ( mDirType == KMImapDir
      && path().startsWith( KMFolderImap::cacheLocation() ) )
    {
       // Is the below needed for dimap as well?
       if ( KMFolderImap::encodeFileName(
                KMFolderImap::decodeFileName( fname ) ) == fname )
       {
          folder = new KMFolder(  this, KMFolderImap::decodeFileName( fname ),
                                  KMFolderTypeImap );
          append(folder);
          folderList.append(folder);
       }
    }
    else if ( mDirType == KMDImapDir
           && path().startsWith( KMFolderCachedImap::cacheLocation() ) )
    {
       if (fileInfo->isDir()) // a directory
       {
          // For this to be a cached IMAP folder, it must be in the KMail dimap
          // subdir and must be have a uidcache file or be a maildir folder
          QString maildir(fname + "/new");
          QString imapcachefile = QString::fromLatin1(".%1.uidcache").arg(fname);
          if ( dir.exists( imapcachefile) || dir.exists( maildir ) )
          {
             folder = new KMFolder( this, fname, KMFolderTypeCachedImap );
             append(folder);
             folderList.append(folder);
          }
       }
    }
    else if ( mDirType == KMSearchDir)
    {
       folder = new KMFolder( this, fname, KMFolderTypeSearch );
       append(folder);
       folderList.append(folder);
    }
    else if ( mDirType == KMStandardDir )
    {
       // This is neither an imap, dimap nor a search folder. Can be either
       // mbox or maildir.
       if (fileInfo->isDir())
       {
          // Maildir folder
          if( dir.exists( fname + "/new" ) )
          {
             folder = new KMFolder( this, fname, KMFolderTypeMaildir );
             append(folder);
             folderList.append(folder);
          }
       }
       else
       {
          // all other files are folders (at the moment ;-)
          folder = new KMFolder( this, fname, KMFolderTypeMbox );
          append(folder);
          folderList.append(folder);
       }
    }
  }

  for (folder=folderList.first(); folder; folder=folderList.next())
  {
    for(QStringList::Iterator it = diList.begin();
        it != diList.end();
        ++it)
      if (*it == "." + folder->fileName() + ".directory")
      {
        KMFolderDir* folderDir = new KMFolderDir( folder, this, *it, mDirType);
        folderDir->reload();
        append(folderDir);
        folder->setChild(folderDir);
        break;
      }
  }
  return TRUE;
}


//-----------------------------------------------------------------------------
KMFolderNode* KMFolderDir::hasNamedFolder(const QString& aName)
{
  KMFolderNode* fNode;
  for (fNode=first(); fNode; fNode=next()) {
    if (fNode->name() == aName)
      return fNode;
  }
  return 0;
}


//-----------------------------------------------------------------------------
KMFolderMgr* KMFolderDir::manager() const
{
  return parent()->manager();
}


#include "kmfolderdir.moc"

