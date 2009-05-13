// kmfolderdir.cpp


#include "kmfolderdir.h"
#include "kmfoldersearch.h"
#include "kmfoldercachedimap.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"

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
  qDeleteAll( begin(), end() ); // we own the pointers to our folders
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
  qDeleteAll( begin(), end() ); // we own the pointers to our folders
  clear();
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

  bool inserted = false;
  QListIterator<KMFolderNode*> it( *this);
  int index = 0;
  while ( it.hasNext() ) {
    KMFolderNode* fNode = it.next();
    if (fNode->name().toLower() > fld->name().toLower()) {
      insert( index, fld );
      inserted = true;
      break;
    }
    ++index;
  }

  if ( !inserted )
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
void KMFolderDir::addDirToParent( const QString &dirName, KMFolder *parentFolder )
{
  KMFolderDir* folderDir = new KMFolderDir( parentFolder, this, dirName, mDirType);
  folderDir->reload();
  append( folderDir );
  parentFolder->setChild( folderDir );
}

// Get the default folder type of the given dir type. This function should only be used when
// needing to find out what the folder type of a missing folder is.
KMFolderType dirTypeToFolderType( KMFolderDirType dirType )
{
  switch( dirType ) {

    // Use maildir for normal folder dirs, as this function is only called when finding a dir
    // without a parent folder, which can only happen with maildir-like folders
    case KMStandardDir: return KMFolderTypeMaildir;

    case KMImapDir: return KMFolderTypeImap;
    case KMDImapDir: return KMFolderTypeCachedImap;
    case KMSearchDir: return KMFolderTypeSearch;
    default: Q_ASSERT( false ); return KMFolderTypeMaildir;
  }
}

//-----------------------------------------------------------------------------
bool KMFolderDir::reload(void)
{
  qDeleteAll( begin(), end() ); // we own the pointers to our folders
  clear();

  const QString fldPath = path();
  QDir dir;
  dir.setFilter(QDir::Files | QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);
  dir.setNameFilters(QStringList("*"));

  if ( !dir.cd(fldPath) )
  {
    QString msg = i18n("<qt>Cannot enter folder <b>%1</b>.</qt>", fldPath);
    KMessageBox::information(0, msg);
    return false;
  }

  if ( !dir.isReadable() )
  {
    QString msg = i18n("<qt>Folder <b>%1</b> is unreadable.</qt>", fldPath);
    KMessageBox::information(0, msg);
    return false;
  }

  QSet<QString> dirs;
  QList<KMFolder*> folderList;
  const QFileInfoList fiList( dir.entryInfoList() ); 
  Q_FOREACH( const QFileInfo& fileInfo, fiList )
  {
    const QString fname = fileInfo.fileName();
    const bool fileIsHidden = fname.startsWith('.');
    if ( fileIsHidden && !fname.endsWith( ".directory" ) ) {
      // ignore all hidden files except our subfolder containers
      continue;
    }
    if ( fname == ".directory"
#ifdef KMAIL_SQLITE_INDEX
    || fname.endsWith( ".index.db" ) 
#endif
    ) {
      // ignore .directory and *.index.db files (not created by us)
      continue;
    }
    // Collect subdirectories.
    if ( fileInfo.isDir() &&
         fileIsHidden && fname.endsWith( ".directory" ) )
    {
       dirs.insert(fname);
       continue;
    }

    // define folder parameters
    QString folderName;
    KMFolderType folderType;
    bool withIndex = true;
    bool exportedSernums = true;

    if ( mDirType == KMImapDir
      && path().startsWith( KMFolderImap::cacheLocation() ) )
    {
       // Is the below needed for dimap as well?
       if ( KMFolderImap::encodeFileName(
                KMFolderImap::decodeFileName( fname ) ) == fname )
       {
          folderName = KMFolderImap::decodeFileName( fname );
          folderType = KMFolderTypeImap;
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
             folderName = fname;
             folderType = KMFolderTypeCachedImap;
          }
       }
    }
    else if ( mDirType == KMSearchDir)
    {
       folderName = fname;
       folderType = KMFolderTypeSearch;
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
             folderName = fname;
             folderType = KMFolderTypeMaildir;
          }
       }
       else
       {
          // all other files are folders (at the moment ;-)
          folderName = fname;
          folderType = KMFolderTypeMbox;
       }
    }

//TODO    if ( &manager()->dir() == this && folderName == QLatin1String( "outbox" ) )
//TODO      withIndex = false;

    // create folder
    if ( !folderName.isEmpty() ) {
      KMFolder* newFolder = new KMFolder( this, folderName, folderType, withIndex, exportedSernums );
      append( newFolder );
      folderList.append( newFolder );
    }
  }

  QSet<QString> dirsWithoutFolder = dirs;
  foreach ( KMFolder* folder, folderList )
  {
    const QString dirName = '.' + folder->fileName() + ".directory";
    if ( dirs.contains( dirName ) )
    {
      dirsWithoutFolder.remove( dirName );
      addDirToParent( dirName, folder );
    }
  }

  // Check if the are any dirs without an associated folder. This can happen if the user messes
  // with the on-disk folder structure, see kolab issue 2972. In that case, we don't want to loose
  // the subfolders as well, so we recreate the folder so the folder/dir hierachy is OK again.
  foreach ( const QString &dirName, dirsWithoutFolder ) {

    // .foo.directory => foo
    QString folderName = dirName;
    int right = folderName.indexOf( ".directory" );
    int left = folderName.indexOf( "." );
    Q_ASSERT( left != -1 && right != -1 );
    folderName = folderName.mid( left + 1, right - 1 );

    kWarning() << "Found dir without associated folder:" << dirName << ", recreating the folder"
               << folderName;

    // Recreate the missing folder
    KMFolder *folder = new KMFolder( this, folderName, KMFolderTypeCachedImap );
    append( folder );
    folderList.append( folder );

    addDirToParent( dirName, folder );
  }
  return true;
}

//-----------------------------------------------------------------------------
KMFolderNode* KMFolderDir::hasNamedFolder(const QString& aName)
{
  QListIterator<KMFolderNode*> it(*this);
  while ( it.hasNext() ) {
      KMFolderNode* node = it.next();
      if ( node && node->name() == aName ) {
          return node;
      }
  }
  return 0;
}


//-----------------------------------------------------------------------------
KMFolderMgr* KMFolderDir::manager() const
{
  return parent()->manager();
}


#include "kmfolderdir.moc"

