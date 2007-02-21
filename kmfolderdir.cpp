// kmfolderdir.cpp

#include <config.h>

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

#include <QDir>
#include <QList>
#include <QByteArray>

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
QString KMFolderRootDir::prettyUrl() const
{
  if ( !mBaseURL.isEmpty() )
    return i18n( mBaseURL.data() );
  else
    return QString();
}


//-----------------------------------------------------------------------------
void KMFolderRootDir::setBaseURL( const QByteArray &baseURL )
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
{}


//-----------------------------------------------------------------------------
KMFolderDir::~KMFolderDir()
{
  clear();
  qDeleteAll( *this );
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderDir::createFolder(const QString& aFolderName, bool aSysFldr, KMFolderType aFolderType)
{
  KMFolder* fld;

  assert(!aFolderName.isEmpty());
  // FIXME urgs, is this still needed
  if (mDirType == KMImapDir)
    fld = new KMFolder( this, aFolderName, KMFolderTypeImap );
  else
    fld = new KMFolder( this, aFolderName, aFolderType );

  assert(fld != 0);
  fld->setSystemFolder(aSysFldr);

  KMFolderNode* fNode;
  QList<KMFolderNode*>::const_iterator it;
  int index = 0;
  for ( it = begin();
      ( ( fNode = *it ) && it != end() );
      ++it ) {
    if (fNode->name().toLower() > fld->name().toLower()) {
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
    return QString();
}


//-----------------------------------------------------------------------------
QString KMFolderDir::prettyUrl() const
{
  QString parentUrl;
  if ( parent() )
    parentUrl = parent()->prettyUrl();
  if ( !parentUrl.isEmpty() )
    return parentUrl + '/' + label();
  else
    return label();
}


//-----------------------------------------------------------------------------
bool KMFolderDir::reload(void)
{
  QDir             dir;
  KMFolder*        folder;
  QFileInfo        fileInfo;
  QStringList      diList;
  QList<KMFolder*> folderList;

  clear();

  const QString fldPath = path();
  dir.setFilter(QDir::Files | QDir::Dirs | QDir::Hidden);
  dir.setNameFilters(QStringList("*"));

  if (!dir.cd(fldPath))
  {
    QString msg = i18n("<qt>Cannot enter folder <b>%1</b>.</qt>", fldPath);
    KMessageBox::information(0, msg);
    return false;
  }

  QFileInfoList fiList = dir.entryInfoList();
  if (fiList.isEmpty())
  {
    QString msg = i18n("<qt>Folder <b>%1</b> is unreadable.</qt>", fldPath);
    KMessageBox::information(0, msg);
    return false;
  }

  Q_FOREACH( fileInfo, fiList )
  {
    const QString fname = fileInfo.fileName();
    if( ( fname[0] == '.' ) && !fname.endsWith( ".directory" ) ) {
      // ignore all hidden files except our subfolder containers
      continue;
    }
    if( fname == ".directory" ) {
      // ignore .directory files (not created by us)
      continue;
    }
    // Collect subdirectories.
    if ( fileInfo.isDir() &&
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
       if (fileInfo.isDir()) // a directory
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
       if (fileInfo.isDir())
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

  QList<KMFolder*>::const_iterator jt;
  for ( jt = folderList.begin();
      ( ( folder = *jt ) && ( jt != folderList.end() ) ); ++jt )
  {
    for(QStringList::Iterator it = diList.begin();
        it != diList.end();
        ++it)
      if (*it == '.' + folder->fileName() + ".directory")
      {
        KMFolderDir* folderDir = new KMFolderDir( folder, this, *it, mDirType);
        folderDir->reload();
        append(folderDir);
        folder->setChild(folderDir);
        break;
      }
  }
  return true;
}


//-----------------------------------------------------------------------------
KMFolderNode* KMFolderDir::hasNamedFolder(const QString& aName)
{
  QList<KMFolderNode*>::const_iterator it;
  for ( it = begin(); it != end(); ++it ) {
    if ( (*it) && (*it)->name() == aName)
      return (*it);
  }
  return 0;
}


//-----------------------------------------------------------------------------
KMFolderMgr* KMFolderDir::manager() const
{
  return parent()->manager();
}


#include "kmfolderdir.moc"

