// kmfoldertree.cpp
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfoldertree.h"

#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmfolderdia.h"
#include "kmheaders.h"
#include "kmmainwidget.h"
#include "kmailicalifaceimpl.h"
#include "accountmanager.h"
using KMail::AccountManager;
#include "globalsettings.h"
#include "kmcommands.h"
#include "foldershortcutdialog.h"
#include "expirypropertiesdialog.h"
#include "newfolderdialog.h"
#include "acljobs.h"
#include "messagecopyhelper.h"
using KMail::MessageCopyHelper;
#include "favoritefolderview.h"
#include "folderviewtooltip.h"
using KMail::FolderViewToolTip;

#include <maillistdrag.h>
using namespace KPIM;

#include <kapplication.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kpopupmenu.h>
#include <kdebug.h>

#include <qpainter.h>
#include <qcursor.h>
#include <qregexp.h>
#include <qpopupmenu.h>

#include <unistd.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <fixx11h.h>

//=============================================================================

KMFolderTreeItem::KMFolderTreeItem( KFolderTree *parent, const QString & name,
                                    KFolderTreeItem::Protocol protocol )
  : QObject( parent, name.latin1() ),
    KFolderTreeItem( parent, name, protocol, Root ),
    mFolder( 0 ), mNeedsRepaint( true )
{
  init();
  setPixmap( 0, normalIcon( iconSize() ) );
}

//-----------------------------------------------------------------------------
KMFolderTreeItem::KMFolderTreeItem( KFolderTree *parent, const QString & name,
                    KMFolder* folder )
  : QObject( parent, name.latin1() ),
    KFolderTreeItem( parent, name ),
    mFolder( folder ), mNeedsRepaint( true )
{
  init();
  setPixmap( 0, normalIcon( iconSize() ) );
}

//-----------------------------------------------------------------------------
KMFolderTreeItem::KMFolderTreeItem( KFolderTreeItem *parent, const QString & name,
                    KMFolder* folder )
  : QObject( 0, name.latin1() ),
    KFolderTreeItem( parent, name ),
    mFolder( folder ), mNeedsRepaint( true )
{
  init();
  setPixmap( 0, normalIcon( iconSize() ) );
}

KMFolderTreeItem::~KMFolderTreeItem()
{
}

static KFolderTreeItem::Protocol protocolFor( KMFolderType t ) {
  switch ( t ) {
  case KMFolderTypeImap:
    return KFolderTreeItem::Imap;
  case KMFolderTypeCachedImap:
    return KFolderTreeItem::CachedImap;
  case KMFolderTypeMbox:
  case KMFolderTypeMaildir:
    return KFolderTreeItem::Local;
  case KMFolderTypeSearch:
    return KFolderTreeItem::Search;
  default:
    return KFolderTreeItem::NONE;
  }
}

QPixmap KMFolderTreeItem::normalIcon(int size) const
{
  QString icon;
  if ( (!mFolder && type() == Root) || useTopLevelIcon() ) {
    switch ( protocol() ) {
      case KFolderTreeItem::Imap:
      case KFolderTreeItem::CachedImap:
      case KFolderTreeItem::News:
        icon = "server"; break;
      case KFolderTreeItem::Search:
        icon = "viewmag";break;
      default:
        icon = "folder";break;
    }
  } else {
    // special folders
    switch ( type() ) {
      case Inbox: icon = "folder_inbox"; break;
      case Outbox: icon = "folder_outbox"; break;
      case SentMail: icon = "folder_sent_mail"; break;
      case Trash: icon = "trashcan_empty"; break;
      case Drafts: icon = "edit"; break;
      case Templates: icon = "filenew"; break;
      default:
      {
        //If not a resource folder don't try to use icalIface folder pixmap
        if(kmkernel->iCalIface().isResourceFolder( mFolder ))
          icon = kmkernel->iCalIface().folderPixmap( type() );
        break;
      }
    }
    // non-root search folders
    if ( protocol() == KMFolderTreeItem::Search ) {
      icon = "mail_find";
    }
    if ( mFolder && mFolder->noContent() ) {
      icon = "folder_grey";
    }
  }

  if ( icon.isEmpty() )
    icon = "folder";

  if (mFolder && mFolder->useCustomIcons() ) {
    icon = mFolder->normalIconPath();
  }
  KIconLoader * il = KGlobal::instance()->iconLoader();
  QPixmap pm = il->loadIcon( icon, KIcon::Small, size,
                             KIcon::DefaultState, 0, true );
  if ( mFolder && pm.isNull() ) {
      pm = il->loadIcon( mFolder->normalIconPath(), KIcon::Small, size,
                         KIcon::DefaultState, 0, true );
  }

  return pm;
}

QPixmap KMFolderTreeItem::unreadIcon(int size) const
{
  QPixmap pm;

  if ( !mFolder || useTopLevelIcon() || mFolder->isSystemFolder() ||
       kmkernel->folderIsTrash( mFolder ) ||
       kmkernel->folderIsTemplates( mFolder ) ||
       kmkernel->folderIsDraftOrOutbox( mFolder ) )
    pm = normalIcon( size );

  KIconLoader * il = KGlobal::instance()->iconLoader();
  if ( mFolder && mFolder->useCustomIcons() ) {
    pm = il->loadIcon( mFolder->unreadIconPath(), KIcon::Small, size,
                       KIcon::DefaultState, 0, true );
    if ( pm.isNull() )
      pm = il->loadIcon( mFolder->normalIconPath(), KIcon::Small, size,
                         KIcon::DefaultState, 0, true );
  }
  if ( pm.isNull() ) {
    if ( mFolder && mFolder->noContent() ) {
      pm = il->loadIcon( "folder_grey_open", KIcon::Small, size,
                         KIcon::DefaultState, 0, true );
    } else {
      if( kmkernel->iCalIface().isResourceFolder( mFolder ) )
        pm = il->loadIcon( kmkernel->iCalIface().folderPixmap( type() ),
                         KIcon::Small, size, KIcon::DefaultState, 0, true );
      if ( pm.isNull() )
        pm = il->loadIcon( "folder_open", KIcon::Small, size,
                           KIcon::DefaultState, 0, true );
    }
  }

  return pm;
}

void KMFolderTreeItem::init()
{
  if ( !mFolder )
    return;

  setProtocol( protocolFor( mFolder->folderType() ) );

  if ( useTopLevelIcon() )
    setType(Root);
  else {
    if ( mFolder == kmkernel->inboxFolder() )
      setType( Inbox );
    else if ( kmkernel->folderIsDraftOrOutbox( mFolder ) ) {
      if ( mFolder == kmkernel->outboxFolder() )
        setType( Outbox );
      else
        setType( Drafts );
    }
    else if ( kmkernel->folderIsSentMailFolder( mFolder ) )
      setType( SentMail );
    else if ( kmkernel->folderIsTrash( mFolder ) )
      setType( Trash );
    else if ( kmkernel->folderIsTemplates( mFolder ) )
      setType( Templates );
    else if( kmkernel->iCalIface().isResourceFolder(mFolder) )
      setType( kmkernel->iCalIface().folderType(mFolder) );
    // System folders on dimap or imap which are not resource folders are
    // inboxes. Urgs.
    if ( mFolder->isSystemFolder() &&
        !kmkernel->iCalIface().isResourceFolder( mFolder) &&
         ( mFolder->folderType() == KMFolderTypeImap
        || mFolder->folderType() == KMFolderTypeCachedImap ) )
      setType( Inbox );
  }
  if ( !mFolder->isSystemFolder() )
    setRenameEnabled( 0, false );

  KMFolderTree* tree = dynamic_cast<KMFolderTree*>( listView() );
  if ( tree )
    tree->insertIntoFolderToItemMap( mFolder, this );
}

void KMFolderTreeItem::adjustUnreadCount( int newUnreadCount ) {
  // adjust the icons if the folder is now newly unread or
  // now newly not-unread
  if ( newUnreadCount != 0 && unreadCount() == 0 )
    setPixmap( 0, unreadIcon( iconSize() ) );
  if ( unreadCount() != 0 && newUnreadCount == 0 )
    setPixmap( 0, normalIcon( iconSize() ) );

  setUnreadCount( newUnreadCount );
}

void KMFolderTreeItem::slotIconsChanged()
{
  kdDebug(5006) << k_funcinfo << endl;
  // this is prone to change, so better check
  KFolderTreeItem::Type newType = type();
  if( kmkernel->iCalIface().isResourceFolder( mFolder ) )
    newType = kmkernel->iCalIface().folderType(mFolder);

  // reload the folder tree if the type changed, needed because of the
  // various type-dependent folder hiding options
  if ( type() != newType )
    static_cast<KMFolderTree*>( listView() )->delayedReload();
  setType( newType );

  if ( unreadCount() > 0 )
    setPixmap( 0, unreadIcon( iconSize() ) );
  else
    setPixmap( 0, normalIcon( iconSize() ) );
  emit iconChanged( this );
  repaint();
}

void KMFolderTreeItem::slotNameChanged()
{
  setText( 0, mFolder->label() );
  emit nameChanged( this );
  repaint();
}

void KMFolderTreeItem::slotNoContentChanged()
{
  // reload the folder tree if the no content state changed, needed because
  // we hide no-content folders if their child nodes are hidden
  QTimer::singleShot( 0, static_cast<KMFolderTree*>( listView() ), SLOT(reload()) );
}

//-----------------------------------------------------------------------------
bool KMFolderTreeItem::acceptDrag(QDropEvent* e) const
{
  // Do not allow drags from the favorite folder view, as they don't really
  // make sense and do not work.
  KMMainWidget *mainWidget = static_cast<KMFolderTree*>( listView() )->mainWidget();
  assert( mainWidget );
  if ( mainWidget->favoriteFolderView() &&
       e->source() == mainWidget->favoriteFolderView()->viewport() )
    return false;

  if ( protocol() == KFolderTreeItem::Search )
    return false; // nothing can be dragged into search folders

  if ( e->provides( KPIM::MailListDrag::format() ) ) {
    if ( !mFolder || mFolder->moveInProgress() || mFolder->isReadOnly() ||
        (mFolder->noContent() && childCount() == 0) ||
        (mFolder->noContent() && isOpen()) ) {
      return false;
    }
    else {
      return true;
    }
  } else if ( e->provides("application/x-qlistviewitem") ) {
    // wtf: protocol() is NONE instead of Local for the local root folder
    if ( !mFolder && protocol() == KFolderTreeItem::NONE && type() == KFolderTreeItem::Root )
      return true; // local top-level folder
    if ( !mFolder || mFolder->isReadOnly() || mFolder->noContent() )
      return false;
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void KMFolderTreeItem::slotShowExpiryProperties()
{
  if ( !mFolder )
    return;

  KMFolderTree* tree = static_cast<KMFolderTree*>( listView() );
  KMail::ExpiryPropertiesDialog *dlg =
    new KMail::ExpiryPropertiesDialog( tree, mFolder );
  dlg->show();
}


//-----------------------------------------------------------------------------
void KMFolderTreeItem::properties()
{
  if ( !mFolder )
    return;

  KMail::FolderTreeBase* tree = static_cast<KMail::FolderTreeBase*>( listView() );
  tree->mainWidget()->modifyFolder( this );
  //Nothing here the above may actually delete this KMFolderTreeItem
}

//-----------------------------------------------------------------------------
void KMFolderTreeItem::assignShortcut()
{
  if ( !mFolder )
    return;

  KMail::FolderShortcutDialog *shorty =
    new KMail::FolderShortcutDialog( mFolder,
              kmkernel->getKMMainWidget(),
              listView() );
  shorty->exec();
  return;
}

//-----------------------------------------------------------------------------
void KMFolderTreeItem::updateCount()
{
    if ( !folder() ) {
      setTotalCount( -1 );
      return;
    }
    KMail::FolderTreeBase* tree = dynamic_cast<KMail::FolderTreeBase*>( listView() );
    if ( !tree ) return;

    tree->slotUpdateCounts( folder(), true /* force update */ );
}


//=============================================================================


KMFolderTree::KMFolderTree( KMMainWidget *mainWidget, QWidget *parent,
                            const char *name )
  : KMail::FolderTreeBase( mainWidget, parent, name )
  , mUpdateTimer( 0, "mUpdateTimer" )
  , autoopen_timer( 0, "autoopen_timer" )
{
  oldSelected = 0;
  oldCurrent = 0;
  mLastItem = 0;
  mMainWidget = mainWidget;
  mReloading = false;
  mCutFolder = false;

  mUpdateCountTimer= new QTimer( this, "mUpdateCountTimer" );

  setDragEnabled( true );
  addAcceptableDropMimetype( "application/x-qlistviewitem", false );

  setSelectionModeExt( Extended );

  int namecol = addColumn( i18n("Folder"), 250 );
  header()->setStretchEnabled( true, namecol );

  // connect
  connectSignals();

  // popup to switch columns
  header()->setClickEnabled(true);
  header()->installEventFilter(this);
  mPopup = new KPopupMenu(this);
  mPopup->insertTitle(i18n("View Columns"));
  mPopup->setCheckable(true);
  mUnreadPop = mPopup->insertItem(i18n("Unread Column"), this, SLOT(slotToggleUnreadColumn()));
  mTotalPop = mPopup->insertItem(i18n("Total Column"), this, SLOT(slotToggleTotalColumn()));
  mSizePop = mPopup->insertItem(i18n("Size Column"), this, SLOT(slotToggleSizeColumn()));

  connect( this, SIGNAL( triggerRefresh() ),
           this, SLOT( refresh() ) );

  new FolderViewToolTip( this );
}

//-----------------------------------------------------------------------------
// connects all needed signals to their slots
void KMFolderTree::connectSignals()
{
  connect( mUpdateCountTimer, SIGNAL(timeout()),
          this, SLOT(slotUpdateCountTimeout()) );

  connect(&mUpdateTimer, SIGNAL(timeout()),
          this, SLOT(delayedUpdate()));

  connect(kmkernel->folderMgr(), SIGNAL(changed()),
          this, SLOT(doFolderListChanged()));

  connect(kmkernel->folderMgr(), SIGNAL(folderRemoved(KMFolder*)),
          this, SLOT(slotFolderRemoved(KMFolder*)));

  connect(kmkernel->folderMgr(), SIGNAL(folderMoveOrCopyOperationFinished()),
	  this, SLOT(slotFolderMoveOrCopyOperationFinished()));

  connect(kmkernel->imapFolderMgr(), SIGNAL(changed()),
          this, SLOT(doFolderListChanged()));

  connect(kmkernel->imapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
          this, SLOT(slotFolderRemoved(KMFolder*)));

  connect(kmkernel->dimapFolderMgr(), SIGNAL(changed()),
          this, SLOT(doFolderListChanged()));

  connect(kmkernel->dimapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
          this, SLOT(slotFolderRemoved(KMFolder*)));

  connect(kmkernel->searchFolderMgr(), SIGNAL(changed()),
          this, SLOT(doFolderListChanged()));

  connect(kmkernel->acctMgr(), SIGNAL(accountRemoved(KMAccount*)),
          this, SLOT(slotAccountRemoved(KMAccount*)));

  connect(kmkernel->acctMgr(), SIGNAL(accountAdded(KMAccount*)),
          this, SLOT(slotUnhideLocalInbox()));

  connect(kmkernel->searchFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
          this, SLOT(slotFolderRemoved(KMFolder*)));

  connect( &autoopen_timer, SIGNAL( timeout() ),
           this, SLOT( openFolder() ) );

  connect( this, SIGNAL( contextMenuRequested( QListViewItem*, const QPoint &, int ) ),
           this, SLOT( slotContextMenuRequested( QListViewItem*, const QPoint & ) ) );

  connect( this, SIGNAL( expanded( QListViewItem* ) ),
           this, SLOT( slotFolderExpanded( QListViewItem* ) ) );

  connect( this, SIGNAL( collapsed( QListViewItem* ) ),
           this, SLOT( slotFolderCollapsed( QListViewItem* ) ) );

  connect( this, SIGNAL( itemRenamed( QListViewItem*, int, const QString &)),
           this, SLOT( slotRenameFolder( QListViewItem*, int, const QString &)));

  connect( this, SIGNAL(folderSelected(KMFolder*)), SLOT(updateCopyActions()) );
}

//-----------------------------------------------------------------------------
void KMFolderTree::readConfig (void)
{
  KConfig* conf = KMKernel::config();

  readColorConfig();

  // Custom/Ssystem font support
  {
    KConfigGroupSaver saver(conf, "Fonts");
    if (!conf->readBoolEntry("defaultFonts",true)) {
      QFont folderFont( KGlobalSettings::generalFont() );
      setFont(conf->readFontEntry("folder-font", &folderFont));
    }
    else
      setFont(KGlobalSettings::generalFont());
  }

  // restore the layout
  restoreLayout(conf, "Geometry");
}

//-----------------------------------------------------------------------------
// Save the configuration file
void KMFolderTree::writeConfig()
{
  // save the current state of the folders
  for ( QListViewItemIterator it( this ) ; it.current() ; ++it ) {
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
    if (fti)
      writeIsListViewItemOpen(fti);
  }

  // save the current layout
  saveLayout(KMKernel::config(), "Geometry");
}

//-----------------------------------------------------------------------------
// Updates the count of unread messages (count of unread messages
// is now cached in KMails config file)
void KMFolderTree::updateUnreadAll()
{
  bool upd = isUpdatesEnabled();
  setUpdatesEnabled(false);

  KMFolderDir* fdir;
  KMFolderNode* folderNode;
  KMFolder* folder;

  fdir = &kmkernel->folderMgr()->dir();
  for (folderNode = fdir->first();
    folderNode != 0;
    folderNode =fdir->next())
  {
    if (!folderNode->isDir()) {
      folder = static_cast<KMFolder*>(folderNode);

      folder->open("updateunread");
      folder->countUnread();
      folder->close("updateunread");
    }
  }

  setUpdatesEnabled(upd);
}

//-----------------------------------------------------------------------------
// Reload the tree of items in the list view
void KMFolderTree::reload(bool openFolders)
{
  if ( mReloading ) {
    // no parallel reloads are allowed
    kdDebug(5006) << "KMFolderTree::reload - already reloading" << endl;
    return;
  }
  mReloading = true;

  int top = contentsY();
  mLastItem = 0;
  // invalidate selected drop item
  oldSelected = 0;
  // remember last
  KMFolder* last = currentFolder();
  KMFolder* selected = 0;
  KMFolder* oldCurrentFolder =
    ( oldCurrent ? static_cast<KMFolderTreeItem*>(oldCurrent)->folder(): 0 );
  for ( QListViewItemIterator it( this ) ; it.current() ; ++it ) {
    KMFolderTreeItem * fti = static_cast<KMFolderTreeItem*>(it.current());
    writeIsListViewItemOpen( fti );
    if ( fti->isSelected() )
      selected = fti->folder();
  }
  mFolderToItem.clear();
  clear();

  // construct the root of the local folders
  KMFolderTreeItem * root = new KMFolderTreeItem( this, i18n("Local Folders") );
  root->setOpen( readIsListViewItemOpen(root) );

  KMFolderDir * fdir = &kmkernel->folderMgr()->dir();
  addDirectory(fdir, root);

  fdir = &kmkernel->imapFolderMgr()->dir();
  // each imap-account creates it's own root
  addDirectory(fdir, 0);

  fdir = &kmkernel->dimapFolderMgr()->dir();
  // each dimap-account creates it's own root
  addDirectory(fdir, 0);

  // construct the root of the search folder hierarchy:
  root = new KMFolderTreeItem( this, i18n("Searches"), KFolderTreeItem::Search );
  root->setOpen( readIsListViewItemOpen( root ) );

  fdir = &kmkernel->searchFolderMgr()->dir();
  addDirectory(fdir, root);

  if (openFolders)
  {
    // we open all folders to update the count
    mUpdateIterator = QListViewItemIterator (this);
    QTimer::singleShot( 0, this, SLOT(slotUpdateOneCount()) );
  }

  for ( QListViewItemIterator it( this ) ; it.current() ; ++it ) {
    KMFolderTreeItem * fti = static_cast<KMFolderTreeItem*>(it.current());
    if ( !fti || !fti->folder() )
      continue;

    disconnect(fti->folder(),SIGNAL(iconsChanged()),
               fti,SLOT(slotIconsChanged()));
    connect(fti->folder(),SIGNAL(iconsChanged()),
            fti,SLOT(slotIconsChanged()));

    disconnect(fti->folder(),SIGNAL(nameChanged()),
               fti,SLOT(slotNameChanged()));
    connect(fti->folder(),SIGNAL(nameChanged()),
            fti,SLOT(slotNameChanged()));

    disconnect( fti->folder(), SIGNAL(noContentChanged()),
                fti, SLOT(slotNoContentChanged()) );
    connect( fti->folder(), SIGNAL(noContentChanged()),
             fti, SLOT(slotNoContentChanged()) );

    // we want to be noticed of changes to update the unread/total columns
    disconnect(fti->folder(), SIGNAL(msgAdded(KMFolder*,Q_UINT32)),
        this,SLOT(slotUpdateCountsDelayed(KMFolder*)));
    connect(fti->folder(), SIGNAL(msgAdded(KMFolder*,Q_UINT32)),
        this,SLOT(slotUpdateCountsDelayed(KMFolder*)));
    //}

    disconnect(fti->folder(), SIGNAL(numUnreadMsgsChanged(KMFolder*)),
               this,SLOT(slotUpdateCountsDelayed(KMFolder*)));
    connect(fti->folder(), SIGNAL(numUnreadMsgsChanged(KMFolder*)),
            this,SLOT(slotUpdateCountsDelayed(KMFolder*)));
    disconnect(fti->folder(), SIGNAL(msgRemoved(KMFolder*)),
               this,SLOT(slotUpdateCountsDelayed(KMFolder*)));
    connect(fti->folder(), SIGNAL(msgRemoved(KMFolder*)),
            this,SLOT(slotUpdateCountsDelayed(KMFolder*)));

  disconnect(fti->folder(), SIGNAL(folderSizeChanged( KMFolder* )),
               this,SLOT(slotUpdateCountsDelayed(KMFolder*)));
  connect(fti->folder(), SIGNAL(folderSizeChanged( KMFolder* )),
               this,SLOT(slotUpdateCountsDelayed(KMFolder*)));



    disconnect(fti->folder(), SIGNAL(shortcutChanged(KMFolder*)),
               mMainWidget, SLOT( slotShortcutChanged(KMFolder*)));
    connect(fti->folder(), SIGNAL(shortcutChanged(KMFolder*)),
            mMainWidget, SLOT( slotShortcutChanged(KMFolder*)));


    if (!openFolders)
      slotUpdateCounts(fti->folder());

    // populate the size column
    fti->setFolderSize( 0 );
    fti->setFolderIsCloseToQuota( fti->folder()->storage()->isCloseToQuota() );

  }
  ensureVisible(0, top + visibleHeight(), 0, 0);
  // if current and selected folder did not change set it again
  for ( QListViewItemIterator it( this ) ; it.current() ; ++it )
  {
    if ( last &&
         static_cast<KMFolderTreeItem*>( it.current() )->folder() == last )
    {
      mLastItem = static_cast<KMFolderTreeItem*>( it.current() );
      setCurrentItem( it.current() );
    }
    if ( selected &&
         static_cast<KMFolderTreeItem*>( it.current() )->folder() == selected )
    {
      setSelected( it.current(), true );
    }
    if ( oldCurrentFolder &&
         static_cast<KMFolderTreeItem*>( it.current() )->folder() == oldCurrentFolder )
    {
      oldCurrent = it.current();
    }
  }
  refresh();
  mReloading = false;
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotUpdateOneCount()
{
  if ( !mUpdateIterator.current() ) return;
  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(mUpdateIterator.current());
  ++mUpdateIterator;
  if ( !fti->folder() ) {
    // next one please
    QTimer::singleShot( 0, this, SLOT(slotUpdateOneCount()) );
    return;
  }

  // open the folder and update the count
  bool open = fti->folder()->isOpened();
  if (!open) fti->folder()->open("updatecount");
  slotUpdateCounts(fti->folder());
  // restore previous state
  if (!open) fti->folder()->close("updatecount");

  QTimer::singleShot( 0, this, SLOT(slotUpdateOneCount()) );
}

//-----------------------------------------------------------------------------
// Recursively add a directory of folders to the tree of folders
void KMFolderTree::addDirectory( KMFolderDir *fdir, KMFolderTreeItem* parent )
{
  for ( KMFolderNode * node = fdir->first() ; node ; node = fdir->next() ) {
    if ( node->isDir() )
      continue;

    KMFolder * folder = static_cast<KMFolder*>(node);
    KMFolderTreeItem * fti = 0;
    if (!parent)
    {
      // create new root-item, but only if this is not the root of a
      // "groupware folders only" account
      if ( kmkernel->iCalIface().hideResourceAccountRoot( folder ) )
        continue;
      // it needs a folder e.g. to save it's state (open/close)
      fti = new KMFolderTreeItem( this, folder->label(), folder );
      fti->setExpandable( true );

      // add child-folders
      if (folder && folder->child()) {
        addDirectory( folder->child(), fti );
      }
    } else {
      // hide local inbox if unused
      if ( kmkernel->inboxFolder() == folder && hideLocalInbox() ) {
        connect( kmkernel->inboxFolder(), SIGNAL(msgAdded(KMFolder*,Q_UINT32)), SLOT(slotUnhideLocalInbox()) );
        continue;
      }

      // create new child
      fti = new KMFolderTreeItem( parent, folder->label(), folder );
      // set folders explicitely to exandable when they have children
      // this way we can do a listing for IMAP folders when the user expands them
      // even when the child folders are not created yet
      if ( folder->storage()->hasChildren() == FolderStorage::HasChildren ) {
        fti->setExpandable( true );
      } else {
        fti->setExpandable( false );
      }

      // add child-folders
      if (folder && folder->child()) {
        addDirectory( folder->child(), fti );
      }

      // Check if this is an IMAP resource folder or a no-content parent only
      // containing groupware folders
      if ( (kmkernel->iCalIface().hideResourceFolder( folder ) || folder->noContent())
            && fti->childCount() == 0 ) {
        // It is
        removeFromFolderToItemMap( folder );
        delete fti;
        // still, it might change in the future, so we better check the change signals
        connect ( folder, SIGNAL(noContentChanged()), SLOT(delayedReload()) );
        continue;
      }

      connect (fti, SIGNAL(iconChanged(KMFolderTreeItem*)),
          this, SIGNAL(iconChanged(KMFolderTreeItem*)));
      connect (fti, SIGNAL(nameChanged(KMFolderTreeItem*)),
          this, SIGNAL(nameChanged(KMFolderTreeItem*)));
    }
    // restore last open-state
    fti->setOpen( readIsListViewItemOpen(fti) );
  } // for-end
}

//-----------------------------------------------------------------------------
// Initiate a delayed refresh of the tree
void KMFolderTree::refresh()
{
  mUpdateTimer.changeInterval(200);
}

//-----------------------------------------------------------------------------
// Updates the pixmap and extendedLabel information for items
void KMFolderTree::delayedUpdate()
{
  bool upd = isUpdatesEnabled();
  if ( upd ) {
    setUpdatesEnabled(false);

    for ( QListViewItemIterator it( this ) ; it.current() ; ++it ) {
      KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
      if (!fti || !fti->folder())
        continue;

      if ( fti->needsRepaint() ) {
        fti->repaint();
        fti->setNeedsRepaint( false );
      }
    }
    setUpdatesEnabled(upd);
  }
  mUpdateTimer.stop();
}

//-----------------------------------------------------------------------------
// Folders have been added/deleted update the tree of folders
void KMFolderTree::doFolderListChanged()
{
  reload();
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotAccountRemoved(KMAccount *)
{
  doFolderSelected( firstChild() );
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotFolderMoveOrCopyOperationFinished()
{
  setDragEnabled( true );
}
//-----------------------------------------------------------------------------
void KMFolderTree::slotFolderRemoved(KMFolder *aFolder)
{
  QListViewItem *item = indexOfFolder(aFolder);
  if (!item) return;
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*> ( item );
  if ( oldCurrent == fti )
    oldCurrent = 0;
  if ( oldSelected == fti )
    oldSelected = 0;
  if (!fti || !fti->folder()) return;
  if (fti == currentItem())
  {
    QListViewItem *qlvi = fti->itemAbove();
    if (!qlvi) qlvi = fti->itemBelow();
    doFolderSelected( qlvi );
  }
  removeFromFolderToItemMap( aFolder );

  if ( dropItem == fti ) { // The removed item is the dropItem
    dropItem = 0; // it becomes invalid
  }

  delete fti;
  updateCopyActions();
}

//-----------------------------------------------------------------------------
// Methods for navigating folders with the keyboard
void KMFolderTree::prepareItem( KMFolderTreeItem* fti )
{
  for ( QListViewItem * parent = fti->parent() ; parent ; parent = parent->parent() )
    parent->setOpen( true );
  ensureItemVisible( fti );
}

//-----------------------------------------------------------------------------
void KMFolderTree::nextUnreadFolder()
{
    nextUnreadFolder( false );
}

//-----------------------------------------------------------------------------
void KMFolderTree::nextUnreadFolder(bool confirm)
{
  QListViewItemIterator it( currentItem() ? currentItem() : firstChild() );
  if ( currentItem() )
    ++it; // don't find current item
  for ( ; it.current() ; ++it ) {
    //check if folder is one to stop on
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
    if (checkUnreadFolder(fti,confirm)) return;
  }
  //Now if confirm is true we are doing "ReadOn"
  //we have got to the bottom of the folder list
  //so we have to start at the top
  if (confirm) {
    for ( it = firstChild() ; it.current() ; ++it ) {
      //check if folder is one to stop on
      KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
      if (checkUnreadFolder(fti,confirm)) return;
    }
  }
}

//-----------------------------------------------------------------------------
bool KMFolderTree::checkUnreadFolder (KMFolderTreeItem* fti, bool confirm)
{
  if ( fti && fti->folder() && !fti->folder()->ignoreNewMail() &&
       ( fti->folder()->countUnread() > 0 ) ) {

    // Don't change into the trash or outbox folders.
    if (fti->type() == KFolderTreeItem::Trash ||
        fti->type() == KFolderTreeItem::Outbox )
      return false;

    if (confirm) {
      // Skip drafts, sent mail and templates as well, when reading mail with
      // the space bar but not when changing into the next folder with unread
      // mail via ctrl+ or ctrl- so we do this only if (confirm == true),
      // which means we are doing readOn.
      if ( fti->type() == KFolderTreeItem::Drafts ||
           fti->type() == KFolderTreeItem::Templates ||
           fti->type() == KFolderTreeItem::SentMail )
        return false;

      //  warn user that going to next folder - but keep track of
      //  whether he wishes to be notified again in "AskNextFolder"
      //  parameter (kept in the config file for kmail)
      if ( KMessageBox::questionYesNo( this,
            i18n( "<qt>Go to the next unread message in folder <b>%1</b>?</qt>" )
            .arg( fti->folder()->label() ),
            i18n( "Go to Next Unread Message" ),
            i18n("Go To"), i18n("Do Not Go To"), // defaults
            "AskNextFolder",
            false)
          == KMessageBox::No ) return true;
    }
    prepareItem( fti );
    blockSignals( true );
    doFolderSelected( fti );
    blockSignals( false );
    emit folderSelectedUnread( fti->folder() );
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void KMFolderTree::prevUnreadFolder()
{
  QListViewItemIterator it( currentItem() ? currentItem() : lastItem() );
  if ( currentItem() )
    --it; // don't find current item
  for ( ; it.current() ; --it ) {
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
    if (checkUnreadFolder(fti,false)) return;
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::incCurrentFolder()
{
  QListViewItemIterator it( currentItem() );
  ++it;
  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
  if (fti) {
      prepareItem( fti );
      setFocus();
      setCurrentItem( fti );
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::decCurrentFolder()
{
  QListViewItemIterator it( currentItem() );
  --it;
  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
  if (fti) {
      prepareItem( fti );
      setFocus();
      setCurrentItem( fti );
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::selectCurrentFolder()
{
  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>( currentItem() );
  if (fti) {
      prepareItem( fti );
      doFolderSelected( fti );
  }
}

//-----------------------------------------------------------------------------
KMFolder *KMFolderTree::currentFolder() const
{
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>( currentItem() );
    if (fti )
        return fti->folder();
    else
        return 0;
}

QValueList<QGuardedPtr<KMFolder> > KMFolderTree::selectedFolders()
{
  QValueList<QGuardedPtr<KMFolder> > rv;
  for ( QListViewItemIterator it( this ); it.current(); ++it ) {
    if ( it.current()->isSelected() ) {
      KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
      rv.append( fti->folder() );
    }
  }
  return rv;
}

//-----------------------------------------------------------------------------
// When not dragging and dropping a change in the selected item
// indicates the user has changed the active folder emit a signal
// so that the header list and reader window can be udpated.
void KMFolderTree::doFolderSelected( QListViewItem* qlvi, bool keepSelection )
{
  if (!qlvi) return;
  if ( mLastItem && mLastItem == qlvi && (keepSelection || selectedFolders().count() == 1) )
    return;

  KMFolderTreeItem* fti = static_cast< KMFolderTreeItem* >(qlvi);
  KMFolder* folder = 0;
  if (fti) folder = fti->folder();


  if (mLastItem && mLastItem != fti && mLastItem->folder()
     && (mLastItem->folder()->folderType() == KMFolderTypeImap))
  {
    KMFolderImap *imapFolder = static_cast<KMFolderImap*>(mLastItem->folder()->storage());
    imapFolder->setSelected(false);
  }
  mLastItem = fti;

  if ( !keepSelection )
    clearSelection();
  setCurrentItem( qlvi );
  if ( !keepSelection )
    setSelected( qlvi, true );
  ensureItemVisible( qlvi );
  if (!folder) {
    emit folderSelected(0); // Root has been selected
  }
  else {
    emit folderSelected(folder);
    slotUpdateCounts(folder);
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::resizeEvent(QResizeEvent* e)
{
  KConfig* conf = KMKernel::config();

  KConfigGroupSaver saver(conf, "Geometry");
  conf->writeEntry(name(), size().width());

  KListView::resizeEvent(e);
}

//-----------------------------------------------------------------------------
// show context menu
void KMFolderTree::slotContextMenuRequested( QListViewItem *lvi,
                                             const QPoint &p )
{
  if (!lvi)
    return;
  setCurrentItem( lvi );

  if (!mMainWidget) return; // safe bet

  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(lvi);
  if ( !isSelected( fti ) )
    doFolderSelected( fti );
  else if ( fti != mLastItem )
    doFolderSelected( fti, true );

  if (!fti )
    return;

  KPopupMenu *folderMenu = new KPopupMenu;
  bool multiFolder = selectedFolders().count() > 1;
  if (fti->folder()) folderMenu->insertTitle(fti->folder()->label());

  // outbox specific, but there it's the most used action
  if ( (fti->folder() == kmkernel->outboxFolder()) && fti->folder()->count() )
        mMainWidget->action("send_queued")->plug( folderMenu );
  // Mark all as read is supposedly used often, therefor it is first
  if ( fti->folder() && !fti->folder()->noContent() )
      mMainWidget->action("mark_all_as_read")->plug( folderMenu );

  /* Treat the special case of the root and account folders */
  if ((!fti->folder() || (fti->folder()->noContent()
    && !fti->parent())))
  {
    QString createChild = i18n("&New Subfolder...");
    if (!fti->folder()) createChild = i18n("&New Folder...");

    if (fti->folder() || (fti->text(0) != i18n("Searches")) && !multiFolder)
        folderMenu->insertItem(SmallIconSet("folder_new"),
                               createChild, this,
                               SLOT(addChildFolder()));

    if (!fti->folder()) {
      mMainWidget->action("compact_all_folders")->plug(folderMenu);
      mMainWidget->action("expire_all_folders")->plug(folderMenu);
    } else if (fti->folder()->folderType() == KMFolderTypeImap) {
      folderMenu->insertItem(SmallIconSet("mail_get"), i18n("Check &Mail"),
        this,
        SLOT(slotCheckMail()));
    }
  } else { // regular folders

    folderMenu->insertSeparator();
    if ( !fti->folder()->noChildren() && !multiFolder ) {
      folderMenu->insertItem(SmallIconSet("folder_new"),
                             i18n("&New Subfolder..."), this,
                             SLOT(addChildFolder()));
    }

    // copy folder
    QPopupMenu *copyMenu = new QPopupMenu( folderMenu );
    folderToPopupMenu( CopyFolder, this, &mMenuToFolder, copyMenu );
    folderMenu->insertItem( i18n("&Copy Folder To"), copyMenu );

    if ( fti->folder()->isMoveable() )
    {
      QPopupMenu *moveMenu = new QPopupMenu( folderMenu );
      folderToPopupMenu( MoveFolder, this, &mMenuToFolder, moveMenu );
      folderMenu->insertItem( i18n("&Move Folder To"), moveMenu );
    }

    // Want to be able to display properties for ALL folders,
    // so we can edit expiry properties.
    // -- smp.
    if (!fti->folder()->noContent())
    {
      if ( !multiFolder )
        mMainWidget->action("search_messages")->plug(folderMenu);

      mMainWidget->action("compact")->plug(folderMenu);

      if ( GlobalSettings::self()->enableFavoriteFolderView() ) {
        folderMenu->insertItem( SmallIconSet("bookmark_add"), i18n("Add to Favorite Folders"),
                                this, SLOT(slotAddToFavorites()) );
      }

      folderMenu->insertSeparator();
      mMainWidget->action("empty")->plug(folderMenu);
      if ( !fti->folder()->isSystemFolder() ) {
        mMainWidget->action("delete_folder")->plug(folderMenu);
      }
      folderMenu->insertSeparator();
    }
  }

  /* plug in IMAP and DIMAP specific things */
  if (fti->folder() &&
      (fti->folder()->folderType() == KMFolderTypeImap ||
       fti->folder()->folderType() == KMFolderTypeCachedImap ))
  {
    folderMenu->insertItem(SmallIconSet("bookmark_folder"),
        i18n("Serverside Subscription..."), mMainWidget,
        SLOT(slotSubscriptionDialog()));
    folderMenu->insertItem(SmallIcon("bookmark_folder"),
        i18n("Local Subscription..."), mMainWidget,
        SLOT(slotLocalSubscriptionDialog()));

    if (!fti->folder()->noContent())
    {
      mMainWidget->action("refresh_folder")->plug(folderMenu);
      if ( fti->folder()->folderType() == KMFolderTypeImap && !multiFolder ) {
        folderMenu->insertItem(SmallIconSet("reload"), i18n("Refresh Folder List"), this,
            SLOT(slotResetFolderList()));
      }
    }
    if ( fti->folder()->folderType() == KMFolderTypeCachedImap && !multiFolder ) {
      KMFolderCachedImap * folder = static_cast<KMFolderCachedImap*>( fti->folder()->storage() );
      folderMenu->insertItem( SmallIconSet("wizard"),
                              i18n("&Troubleshoot IMAP Cache..."),
                              folder, SLOT(slotTroubleshoot()) );
    }
    folderMenu->insertSeparator();
  }

  if ( fti->folder() && fti->folder()->isMailingListEnabled() && !multiFolder ) {
    mMainWidget->action("post_message")->plug(folderMenu);
  }

  if (fti->folder() && fti->parent() && !multiFolder)
  {
    folderMenu->insertItem(SmallIconSet("configure_shortcuts"),
        i18n("&Assign Shortcut..."),
        fti,
        SLOT(assignShortcut()));

    if ( !fti->folder()->noContent() ) {
      folderMenu->insertItem( i18n("Expire..."), fti,
                              SLOT( slotShowExpiryProperties() ) );
    }
    mMainWidget->action("modify")->plug(folderMenu);
  }


  kmkernel->setContextMenuShown( true );
  folderMenu->exec (p, 0);
  kmkernel->setContextMenuShown( false );
  triggerUpdate();
  delete folderMenu;
  folderMenu = 0;
}

//-----------------------------------------------------------------------------
void KMFolderTree::contentsMousePressEvent(QMouseEvent * e)
{
  // KFolderTree messes around with the selection mode
  KListView::contentsMousePressEvent( e );
}

// If middle button and folder holds mailing-list, create a message to that list
void KMFolderTree::contentsMouseReleaseEvent(QMouseEvent* me)
{
  QListViewItem *lvi = currentItem(); // Needed for when branches are clicked on
  ButtonState btn = me->button();
  doFolderSelected(lvi, true);

  // get underlying folder
  KMFolderTreeItem* fti = dynamic_cast<KMFolderTreeItem*>(lvi);

  if (!fti || !fti->folder()) {
    KFolderTree::contentsMouseReleaseEvent(me);
    return;
  }

  // react on middle-button only
  if (btn != Qt::MidButton) {
    KFolderTree::contentsMouseReleaseEvent(me);
    return;
  }

  if ( fti->folder()->isMailingListEnabled() ) {
    KMCommand *command = new KMMailingListPostCommand( this, fti->folder() );
    command->start();
  }

  KFolderTree::contentsMouseReleaseEvent(me);
}

// little static helper
static bool folderHasCreateRights( const KMFolder *folder )
{
  bool createRights = true; // we don't have acls for local folders yet
  if ( folder && folder->folderType() == KMFolderTypeImap ) {
    const KMFolderImap *imapFolder = static_cast<const KMFolderImap*>( folder->storage() );
    createRights = imapFolder->userRights() == 0 || // hack, we should get the acls
      ( imapFolder->userRights() > 0 && ( imapFolder->userRights() & KMail::ACLJobs::Create ) );
  } else if ( folder && folder->folderType() == KMFolderTypeCachedImap ) {
    const KMFolderCachedImap *dimapFolder = static_cast<const KMFolderCachedImap*>( folder->storage() );
    createRights = dimapFolder->userRights() == 0 ||
      ( dimapFolder->userRights() > 0 && ( dimapFolder->userRights() & KMail::ACLJobs::Create ) );
  }
  return createRights;
}

//-----------------------------------------------------------------------------
// Create a subfolder.
// Requires creating the appropriate subdirectory and show a dialog
void KMFolderTree::addChildFolder( KMFolder *folder, QWidget * parent )
{
  KMFolder *aFolder = folder;
  if ( !aFolder ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(currentItem());
    if (!fti)
      return;
    aFolder = fti->folder();
  }
  if (aFolder) {
    if (!aFolder->createChildFolder())
      return;
    if ( !folderHasCreateRights( aFolder ) ) {
      // FIXME: change this message to "Cannot create folder under ..." or similar
      const QString message = i18n( "<qt>Cannot create folder <b>%1</b> because of insufficient "
                                    "permissions on the server. If you think you should be able to create "
                                    "subfolders here, ask your administrator to grant you rights to do so."
                                    "</qt> " ).arg(aFolder->label());
      KMessageBox::error( this, message );
      return;
    }
  }

  if ( parent )
    ( new KMail::NewFolderDialog( parent, aFolder ) )->exec();
  else
    ( new KMail::NewFolderDialog( this, aFolder ) )->show();
  return;
/*
  KMFolderDir *dir = &(kmkernel->folderMgr()->dir());
  if (aFolder)
    dir = aFolder->child();

  KMFolderDialog *d =
    new KMFolderDialog(0, dir, this, i18n("Create Subfolder") );

  if (d->exec()) { // fti may be deleted here
    QListViewItem *qlvi = indexOfFolder( aFolder );
    if (qlvi) {
      qlvi->setOpen(true);
      blockSignals( true );
      setCurrentItem( qlvi );
      blockSignals( false );
    }
  }
  delete d;
  // update if added to root Folder
  if (!aFolder || aFolder->noContent()) {
     doFolderListChanged();
  }
  */
}

//-----------------------------------------------------------------------------
// Returns whether a folder directory should be open as specified in the
// config file.
bool KMFolderTree::readIsListViewItemOpen(KMFolderTreeItem *fti)
{
  KConfig* config = KMKernel::config();
  KMFolder *folder = fti->folder();
  QString name;
  if (folder)
  {
    name = "Folder-" + folder->idString();
  } else if (fti->type() == KFolderTreeItem::Root)
  {
    if (fti->protocol() == KFolderTreeItem::NONE) // local root
      name = "Folder_local_root";
    else if (fti->protocol() == KFolderTreeItem::Search)
      name = "Folder_search";
    else
      return false;
  } else {
    return false;
  }
  KConfigGroupSaver saver(config, name);

  return config->readBoolEntry("isOpen", false);
}

//-----------------------------------------------------------------------------
// Saves open/closed state of a folder directory into the config file
void KMFolderTree::writeIsListViewItemOpen(KMFolderTreeItem *fti)
{
  KConfig* config = KMKernel::config();
  KMFolder *folder = fti->folder();
  QString name;
  if (folder && !folder->idString().isEmpty())
  {
    name = "Folder-" + folder->idString();
  } else if (fti->type() == KFolderTreeItem::Root)
  {
    if (fti->protocol() == KFolderTreeItem::NONE) // local root
      name = "Folder_local_root";
    else if (fti->protocol() == KFolderTreeItem::Search)
      name = "Folder_search";
    else
      return;
  } else {
    return;
  }
  KConfigGroupSaver saver(config, name);
  config->writeEntry("isOpen", fti->isOpen() );
}


//-----------------------------------------------------------------------------
void KMFolderTree::cleanupConfigFile()
{
  if ( childCount() == 0 )
    return; // just in case reload wasn't called before
  KConfig* config = KMKernel::config();
  QStringList existingFolders;
  QListViewItemIterator fldIt(this);
  QMap<QString,bool> folderMap;
  KMFolderTreeItem *fti;
  for (QListViewItemIterator fldIt(this); fldIt.current(); fldIt++)
  {
    fti = static_cast<KMFolderTreeItem*>(fldIt.current());
    if (fti && fti->folder())
      folderMap.insert(fti->folder()->idString(), true);
  }
  QStringList groupList = config->groupList();
  QString name;
  for (QStringList::Iterator grpIt = groupList.begin();
    grpIt != groupList.end(); grpIt++)
  {
    if ((*grpIt).left(7) != "Folder-") continue;
    name = (*grpIt).mid(7);
    if (folderMap.find(name) == folderMap.end())
    {
      KMFolder* folder = kmkernel->findFolderById( name );
      if ( folder ) {
        if ( kmkernel->iCalIface().hideResourceFolder( folder )
           ||  kmkernel->iCalIface().hideResourceAccountRoot( folder ) )
          continue; // hidden IMAP resource folder, don't delete info
        if ( folder->noContent() )
          continue; // we hide nocontent folders if they have no child folders
        if ( folder == kmkernel->inboxFolder() )
          continue; // local inbox can be hidden as well
      }

      //KMessageBox::error( 0, "cleanupConfigFile: Deleting group " + *grpIt );
      config->deleteGroup(*grpIt, true);
      kdDebug(5006) << "Deleting information about folder " << name << endl;
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolderTree::openFolder()
{
    autoopen_timer.stop();
    if ( dropItem && !dropItem->isOpen() ) {
        dropItem->setOpen( true );
        dropItem->repaint();
    }
}

static const int autoopenTime = 750;

//-----------------------------------------------------------------------------
void KMFolderTree::contentsDragEnterEvent( QDragEnterEvent *e )
{
  oldCurrent = 0;
  oldSelected = 0;

  oldCurrent = currentItem();
  for ( QListViewItemIterator it( this ) ; it.current() ; ++it )
    if ( it.current()->isSelected() )
      oldSelected = it.current();

  setFocus();

  QListViewItem *i = itemAt( contentsToViewport(e->pos()) );
  if ( i ) {
    dropItem = i;
    autoopen_timer.start( autoopenTime );
  }
  else
    dropItem = 0;

  e->accept( acceptDrag(e) );
}

//-----------------------------------------------------------------------------
void KMFolderTree::contentsDragMoveEvent( QDragMoveEvent *e )
{
    QPoint vp = contentsToViewport(e->pos());
    QListViewItem *i = itemAt( vp );
    if ( i ) {
        bool dragAccepted = acceptDrag( e );
        if ( dragAccepted ) {
            setCurrentItem( i );
        }

        if ( i != dropItem ) {
            autoopen_timer.stop();
            dropItem = i;
            autoopen_timer.start( autoopenTime );
        }

        if ( dragAccepted ) {
            e->accept( itemRect(i) );

            switch ( e->action() ) {
                case QDropEvent::Copy:
                break;
                case QDropEvent::Move:
                e->acceptAction();
                break;
                case QDropEvent::Link:
                e->acceptAction();
                break;
                default:
                ;
            }
        } else {
            e->accept( false );
        }
    } else {
        e->accept( false );
        autoopen_timer.stop();
        dropItem = 0;
    }
}

//-----------------------------------------------------------------------------
void KMFolderTree::contentsDragLeaveEvent( QDragLeaveEvent * )
{
    if (!oldCurrent) return;

    autoopen_timer.stop();
    dropItem = 0;

    setCurrentItem( oldCurrent );
    if ( oldSelected )
      setSelected( oldSelected, true );
}

//-----------------------------------------------------------------------------
void KMFolderTree::contentsDropEvent( QDropEvent *e )
{
    autoopen_timer.stop();

    QListViewItem *item = itemAt( contentsToViewport(e->pos()) );
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);
    // Check that each pointer is not null
    for ( QValueList<QGuardedPtr<KMFolder> >::ConstIterator it = mCopySourceFolders.constBegin();
	  it != mCopySourceFolders.constEnd(); ++it ) {
      if ( ! (*it) ) {
	fti = 0;
	break;
      }
    }
    if (fti && mCopySourceFolders.count() == 1)
    {
      KMFolder *source = mCopySourceFolders.first();
      // if we are dragging to ourselves or to our parent, set fti to 0 so nothing is done
      if (source == fti->folder() || source->parent()->owner() == fti->folder()) fti = 0;
    }
    if (fti && acceptDrag(e) && ( fti != oldSelected || e->source() != mMainWidget->headers()->viewport() ) )
    {
      if ( e->provides("application/x-qlistviewitem") ) {
        int action = dndMode( true /* always ask */ );
        if ( (action == DRAG_COPY || action == DRAG_MOVE) && !mCopySourceFolders.isEmpty() ) {
          for ( QValueList<QGuardedPtr<KMFolder> >::ConstIterator it = mCopySourceFolders.constBegin();
                it != mCopySourceFolders.constEnd(); ++it ) {
            if ( ! (*it)->isMoveable() )
              action = DRAG_COPY;
          }
          moveOrCopyFolder( mCopySourceFolders, fti->folder(), (action == DRAG_MOVE) );
        }
      } else {
        if ( e->source() == mMainWidget->headers()->viewport() ) {
          int action;
          if ( mMainWidget->headers()->folder() && mMainWidget->headers()->folder()->isReadOnly() )
            action = DRAG_COPY;
          else
            action = dndMode();
          // KMHeaders does copy/move itself
          if ( action == DRAG_MOVE && fti->folder() )
            emit folderDrop( fti->folder() );
          else if ( action == DRAG_COPY && fti->folder() )
            emit folderDropCopy( fti->folder() );
        } else {
          handleMailListDrop( e, fti->folder() );
        }
      }
      e->accept( true );
    } else
      e->accept( false );

    dropItem = 0;

    setCurrentItem( oldCurrent );
    if ( oldCurrent) mLastItem = static_cast<KMFolderTreeItem*>(oldCurrent);
    if ( oldSelected )
    {
      clearSelection();
      setSelected( oldSelected, true );
    }

    mCopySourceFolders.clear();
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotFolderExpanded( QListViewItem * item )
{
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);
  if ( !fti || !fti->folder() || !fti->folder()->storage() ) return;

  fti->setFolderSize( fti->folder()->storage()->folderSize() );

  if( fti->folder()->folderType() == KMFolderTypeImap )
  {
    KMFolderImap *folder = static_cast<KMFolderImap*>( fti->folder()->storage() );
    // if we should list all folders we limit this to the root folder
    if ( !folder->account() || ( !folder->account()->listOnlyOpenFolders() &&
         fti->parent() ) )
      return;
    if ( folder->getSubfolderState() == KMFolderImap::imapNoInformation )
    {
      // check if all parents are expanded
      QListViewItem *parent = item->parent();
      while ( parent )
      {
        if ( !parent->isOpen() )
          return;
        parent = parent->parent();
      }
      // the tree will be reloaded after that
      bool success = folder->listDirectory();
      if (!success) fti->setOpen( false );
      if ( fti->childCount() == 0 && fti->parent() )
        fti->setExpandable( false );
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolderTree::slotFolderCollapsed( QListViewItem * item )
{
  slotResetFolderList( item, false );
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);
  if ( !fti || !fti->folder() || !fti->folder()->storage() ) return;

  fti->setFolderSize( fti->folder()->storage()->folderSize() );
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotRenameFolder(QListViewItem *item, int col,
                const QString &text)
{

  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);

  if ((!fti) || (fti && fti->folder() && col != 0 && !currentFolder()->child()))
          return;

  QString fldName, oldFldName;

  oldFldName = fti->name(0);

  if (!text.isEmpty())
          fldName = text;
  else
          fldName = oldFldName;

  fldName.replace("/", "");
  fldName.replace(QRegExp("^\\."), "");

  if (fldName.isEmpty())
          fldName = i18n("unnamed");

  fti->setText(0, fldName);
  fti->folder()->rename(fldName, &(kmkernel->folderMgr()->dir()));
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotUpdateCountsDelayed(KMFolder * folder)
{
//  kdDebug(5006) << "KMFolderTree::slotUpdateCountsDelayed()" << endl;
  if ( !mFolderToUpdateCount.contains( folder->idString() ) )
  {
//    kdDebug( 5006 )<< "adding " << folder->idString() << " to updateCountList " << endl;
    mFolderToUpdateCount.insert( folder->idString(),folder );
  }
  if ( !mUpdateCountTimer->isActive() )
    mUpdateCountTimer->start( 500 );
}


void KMFolderTree::slotUpdateCountTimeout()
{
//  kdDebug(5006) << "KMFolderTree::slotUpdateCountTimeout()" << endl;

  QMap<QString,KMFolder*>::iterator it;
  for ( it= mFolderToUpdateCount.begin();
      it!=mFolderToUpdateCount.end();
      ++it )
  {
    slotUpdateCounts( it.data() );
  }
  mFolderToUpdateCount.clear();
  mUpdateCountTimer->stop();

}

void KMFolderTree::updatePopup() const
{
   mPopup->setItemChecked( mUnreadPop, isUnreadActive() );
   mPopup->setItemChecked( mTotalPop, isTotalActive() );
   mPopup->setItemChecked( mSizePop, isSizeActive() );
}

//-----------------------------------------------------------------------------
void KMFolderTree::toggleColumn(int column, bool openFolders)
{
  if (column == unread)
  {
    // switch unread
    if ( isUnreadActive() )
    {
      removeUnreadColumn();
      reload();
    } else {
      addUnreadColumn( i18n("Unread"), 70 );
      reload();
    }
    // toggle KPopupMenu
    mPopup->setItemChecked( mUnreadPop, isUnreadActive() );

  } else if (column == total) {
    // switch total
    if ( isTotalActive() )
    {
      removeTotalColumn();
      reload();
    } else {
      addTotalColumn( i18n("Total"), 70 );
      reload(openFolders);
    }
    mPopup->setItemChecked( mTotalPop, isTotalActive() );
  } else if (column == foldersize) {
    // switch total
    if ( isSizeActive() )
    {
      removeSizeColumn();
      reload();
    } else {
      addSizeColumn( i18n("Size"), 70 );
      reload( openFolders );
    }
    // toggle KPopupMenu
    mPopup->setItemChecked( mSizePop, isSizeActive() );

  } else kdDebug(5006) << "unknown column:" << column << endl;

  // toggles the switches of the mainwin
  emit columnsChanged();
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotToggleUnreadColumn()
{
  toggleColumn(unread);
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotToggleTotalColumn()
{
  // activate the total-column and force the folders to be opened
  toggleColumn(total, true);
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotToggleSizeColumn()
{
  // activate the size-column and force the folders to be opened
  toggleColumn(foldersize, true);
}


//-----------------------------------------------------------------------------
bool KMFolderTree::eventFilter( QObject *o, QEvent *e )
{
  if ( e->type() == QEvent::MouseButtonPress &&
      static_cast<QMouseEvent*>(e)->button() == RightButton &&
      o->isA("QHeader") )
  {
    mPopup->popup( static_cast<QMouseEvent*>(e)->globalPos() );
    return true;
  }
  return KFolderTree::eventFilter(o, e);
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotCheckMail()
{
  if (!currentItem())
    return;
  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(currentItem());
  KMFolder* folder = fti->folder();
  if (folder && folder->storage() ) {
      if ( KMAccount* acct = folder->storage()->account() ) {
         kmkernel->acctMgr()->singleCheckMail(acct, true);
      }
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotNewMessageToMailingList()
{
  KMFolderTreeItem* fti = dynamic_cast<KMFolderTreeItem*>( currentItem() );
  if ( !fti || !fti->folder() )
    return;
  KMCommand *command = new KMMailingListPostCommand( this, fti->folder() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMFolderTree::createFolderList( QStringList *str,
                                     QValueList<QGuardedPtr<KMFolder> > *folders,
                                     bool localFolders,
                                     bool imapFolders,
                                     bool dimapFolders,
                                     bool searchFolders,
                                     bool includeNoContent,
                                     bool includeNoChildren )
{
  for ( QListViewItemIterator it( this ) ; it.current() ; ++it )
  {
    KMFolderTreeItem * fti = static_cast<KMFolderTreeItem*>(it.current());
    if (!fti || !fti->folder()) continue;
    // type checks
    KMFolder* folder = fti->folder();
    if (!imapFolders && folder->folderType() == KMFolderTypeImap) continue;
    if (!dimapFolders && folder->folderType() == KMFolderTypeCachedImap) continue;
    if (!localFolders && (folder->folderType() == KMFolderTypeMbox ||
                          folder->folderType() == KMFolderTypeMaildir)) continue;
    if (!searchFolders && folder->folderType() == KMFolderTypeSearch) continue;
    if (!includeNoContent && folder->noContent()) continue;
    if (!includeNoChildren && folder->noChildren()) continue;
    QString prefix;
    prefix.fill( ' ', 2 * fti->depth() );
    str->append(prefix + fti->text(0));
    folders->append(fti->folder());
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotResetFolderList( QListViewItem* item, bool startList )
{
  if ( !item )
    item = currentItem();

  KMFolderTreeItem* fti = dynamic_cast<KMFolderTreeItem*>( item );
  if ( fti && fti->folder() &&
       fti->folder()->folderType() == KMFolderTypeImap )
  {
    KMFolderImap *folder = static_cast<KMFolderImap*>( fti->folder()->storage() );
    folder->setSubfolderState( KMFolderImap::imapNoInformation );
    if ( startList )
      folder->listDirectory();
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::showFolder( KMFolder* folder )
{
  if ( !folder ) return;
  QListViewItem* item = indexOfFolder( folder );
  if ( item )
  {
    doFolderSelected( item );
    ensureItemVisible( item );
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::folderToPopupMenu( MenuAction action, QObject *receiver,
    KMMenuToFolder *aMenuToFolder, QPopupMenu *menu, QListViewItem *item )
{
  while ( menu->count() )
  {
    QPopupMenu *popup = menu->findItem( menu->idAt( 0 ) )->popup();
    if ( popup )
      delete popup;
    else
      menu->removeItemAt( 0 );
  }
  // connect the signals
  if ( action == MoveMessage || action == MoveFolder )
  {
    disconnect( menu, SIGNAL(activated(int)), receiver,
        SLOT(moveSelectedToFolder(int)) );
    connect( menu, SIGNAL(activated(int)), receiver,
        SLOT(moveSelectedToFolder(int)) );
  } else {
    disconnect( menu, SIGNAL(activated(int)), receiver,
        SLOT(copySelectedToFolder(int)) );
    connect( menu, SIGNAL(activated(int)), receiver,
        SLOT(copySelectedToFolder(int)) );
  }
  if ( !item ) {
    item = firstChild();

    // avoid a popup menu with the single entry 'Local Folders' if there
    // are no IMAP accounts
    if ( childCount() == 2 && action != MoveFolder ) { // only 'Local Folders' and 'Searches'
      KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( item );
      if ( fti->protocol() == KFolderTreeItem::Search ) {
        // skip 'Searches'
        item = item->nextSibling();
        fti = static_cast<KMFolderTreeItem*>( item );
      }
      folderToPopupMenu( action, receiver, aMenuToFolder, menu, fti->firstChild() );
      return;
    }
  }

  while ( item )
  {
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>( item );
    if ( fti->protocol() == KFolderTreeItem::Search )
    {
      // skip search folders
      item = item->nextSibling();
      continue;
    }
    QString label = fti->text( 0 );
    label.replace( "&","&&" );
    if ( fti->firstChild() )
    {
      // new level
      QPopupMenu* popup = new QPopupMenu( menu, "subMenu" );
      folderToPopupMenu( action, receiver, aMenuToFolder, popup, fti->firstChild() );
      bool subMenu = false;
      if ( ( action == MoveMessage || action == CopyMessage ) &&
           fti->folder() && !fti->folder()->noContent() )
        subMenu = true;
      if ( ( action == MoveFolder || action == CopyFolder )
          && ( !fti->folder() || ( fti->folder() && !fti->folder()->noChildren() ) ) )
        subMenu = true;

      QString sourceFolderName;
      KMFolderTreeItem* srcItem = dynamic_cast<KMFolderTreeItem*>( currentItem() );
      if ( srcItem )
        sourceFolderName = srcItem->text( 0 );

      if ( (action == MoveFolder || action == CopyFolder)
              && fti->folder() && fti->folder()->child()
              && fti->folder()->child()->hasNamedFolder( sourceFolderName ) ) {
        subMenu = false;
      }

      if ( subMenu )
      {
        int menuId;
        if ( action == MoveMessage || action == MoveFolder )
          menuId = popup->insertItem( i18n("Move to This Folder"), -1, 0 );
        else
          menuId = popup->insertItem( i18n("Copy to This Folder"), -1, 0 );
        popup->insertSeparator( 1 );
        aMenuToFolder->insert( menuId, fti->folder() );
      }
      menu->insertItem( label, popup );
    } else
    {
      // insert an item
      int menuId = menu->insertItem( label );
      if ( fti->folder() )
        aMenuToFolder->insert( menuId, fti->folder() );
      bool enabled = (fti->folder() ? true : false);
      if ( fti->folder() &&
           ( fti->folder()->isReadOnly() || fti->folder()->noContent() ) )
        enabled = false;
      menu->setItemEnabled( menuId, enabled );
    }

    item = item->nextSibling();
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::moveSelectedToFolder( int menuId )
{
  moveOrCopyFolder( selectedFolders(), mMenuToFolder[ menuId ], true /*move*/ );
}

//-----------------------------------------------------------------------------
void KMFolderTree::copySelectedToFolder( int menuId )
{
  moveOrCopyFolder( selectedFolders(), mMenuToFolder[ menuId ], false /*copy, don't move*/ );
}

//-----------------------------------------------------------------------------
void KMFolderTree::moveOrCopyFolder( QValueList<QGuardedPtr<KMFolder> > sources, KMFolder* destination, bool move )
{
  kdDebug(5006) << k_funcinfo << "source: " << sources << " destination: " << destination << " move: " << move << endl;

  // Disable drag during copy operation since it prevents from many crashes
  setDragEnabled( false );

  KMFolderDir* parent = &(kmkernel->folderMgr()->dir());
  if ( destination )
    parent = destination->createChildFolder();

  QStringList sourceFolderNames;

  // check if move/copy is possible at all
  for ( QValueList<QGuardedPtr<KMFolder> >::ConstIterator it = sources.constBegin(); it != sources.constEnd(); ++it ) {
    KMFolder* source = *it;

    // check if folder with same name already exits
    QString sourceFolderName;
    if ( source )
      sourceFolderName = source->label();

    if ( parent->hasNamedFolder( sourceFolderName ) || sourceFolderNames.contains( sourceFolderName ) ) {
      KMessageBox::error( this, i18n("<qt>Cannot move or copy folder <b>%1</b> here because a folder with the same name already exists.</qt>")
          .arg( sourceFolderName ) );
      return;
    }
    sourceFolderNames.append( sourceFolderName );

    // don't move/copy a folder that's still not completely moved/copied
    KMFolder *f = source;
    while ( f ) {
      if ( f->moveInProgress() ) {
        KMessageBox::error( this, i18n("<qt>Cannot move or copy folder <b>%1</b> because it is not completely copied itself.</qt>")
            .arg( sourceFolderName ) );
        return;
      }
      if ( f->parent() )
        f = f->parent()->owner();
    }

    QString message =
      i18n( "<qt>Cannot move or copy folder <b>%1</b> into a subfolder below itself.</qt>" ).
          arg( sourceFolderName );
    KMFolderDir* folderDir = parent;
    // check that the folder can be moved
    if ( source && source->child() )
    {
      while ( folderDir && ( folderDir != &kmkernel->folderMgr()->dir() ) &&
          ( folderDir != source->parent() ) )
      {
        if ( folderDir->findRef( source ) != -1 )
        {
          KMessageBox::error( this, message );
          return;
        }
        folderDir = folderDir->parent();
      }
    }

    if( source && source->child() && parent &&
        ( parent->path().find( source->child()->path() + "/" ) == 0 ) ) {
      KMessageBox::error( this, message );
      return;
    }

    if( source && source->child()
        && ( parent == source->child() ) ) {
      KMessageBox::error( this, message );
      return;
    }
  }

  // check if the source folders are independent of each other
  for ( QValueList<QGuardedPtr<KMFolder> >::ConstIterator it = sources.constBegin(); move && it != sources.constEnd(); ++it ) {
    KMFolderDir *parentDir = (*it)->child();
    if ( !parentDir )
      continue;
    for ( QValueList<QGuardedPtr<KMFolder> >::ConstIterator it2 = sources.constBegin(); it2 != sources.constEnd(); ++it2 ) {
      if ( *it == *it2 )
        continue;
      KMFolderDir *childDir = (*it2)->parent();
      do {
        if ( parentDir == childDir || parentDir->findRef( childDir->owner() ) != -1 ) {
          KMessageBox::error( this, i18n("Moving the selected folders is not possible") );
          return;
        }
        childDir = childDir->parent();
      }
      while ( childDir && childDir != &kmkernel->folderMgr()->dir() );
    }
  }

  // de-select moved source folders (can cause crash due to unGetMsg() in KMHeaders)
  if ( move ) {
    doFolderSelected( indexOfFolder( destination ), false );
    oldCurrent = currentItem();
  }

  // do the actual move/copy
  for ( QValueList<QGuardedPtr<KMFolder> >::ConstIterator it = sources.constBegin(); it != sources.constEnd(); ++it ) {
    KMFolder* source = *it;
    if ( move ) {
      kdDebug(5006) << "move folder " << (source ? source->label(): "Unknown") << " to "
        << ( destination ? destination->label() : "Local Folders" ) << endl;
      kmkernel->folderMgr()->moveFolder( source, parent );
    } else {
      kmkernel->folderMgr()->copyFolder( source, parent );
    }
  }
}

QDragObject * KMFolderTree::dragObject()
{
  KMFolderTreeItem *item = static_cast<KMFolderTreeItem*>
      (itemAt(viewport()->mapFromGlobal(QCursor::pos())));
  if ( !item || !item->parent() || !item->folder() ) // top-level items or something invalid
    return 0;
  mCopySourceFolders = selectedFolders();

  QDragObject *drag = KFolderTree::dragObject();
  if ( drag )
    drag->setPixmap( SmallIcon("folder") );
  return drag;
}

void KMFolderTree::copyFolder()
{
  KMFolderTreeItem *item = static_cast<KMFolderTreeItem*>( currentItem() );
  if ( item ) {
    mCopySourceFolders = selectedFolders();
    mCutFolder = false;
  }
  updateCopyActions();
}

void KMFolderTree::cutFolder()
{
  KMFolderTreeItem *item = static_cast<KMFolderTreeItem*>( currentItem() );
  if ( item ) {
    mCopySourceFolders = selectedFolders();
    mCutFolder = true;
  }
  updateCopyActions();
}

void KMFolderTree::pasteFolder()
{
  KMFolderTreeItem *item = static_cast<KMFolderTreeItem*>( currentItem() );
  if ( !mCopySourceFolders.isEmpty() && item && !mCopySourceFolders.contains( item->folder() ) ) {
    moveOrCopyFolder( mCopySourceFolders, item->folder(), mCutFolder );
    if ( mCutFolder )
      mCopySourceFolders.clear();
  }
  updateCopyActions();
}

void KMFolderTree::updateCopyActions()
{
  KAction *copy = mMainWidget->action("copy_folder");
  KAction *cut = mMainWidget->action("cut_folder");
  KAction *paste = mMainWidget->action("paste_folder");
  KMFolderTreeItem *item = static_cast<KMFolderTreeItem*>( currentItem() );

  if ( !item ||  !item->folder() ) {
    copy->setEnabled( false );
    cut->setEnabled( false );
  } else {
    copy->setEnabled( true );
    cut->setEnabled( item->folder()->isMoveable() );
  }

  if ( mCopySourceFolders.isEmpty() )
    paste->setEnabled( false );
  else
    paste->setEnabled( true );
}

void KMFolderTree::slotAddToFavorites()
{
  KMail::FavoriteFolderView *favView = mMainWidget->favoriteFolderView();
  assert( favView );
  for ( QListViewItemIterator it( this ); it.current(); ++it ) {
    if ( it.current()->isSelected() )
      favView->addFolder( static_cast<KMFolderTreeItem*>( it.current() ) );
  }
}

void KMFolderTree::slotUnhideLocalInbox()
{
  disconnect( kmkernel->inboxFolder(), SIGNAL(msgAdded(KMFolder*,Q_UINT32)),
              this, SLOT(slotUnhideLocalInbox()) );
  reload();
}

void KMFolderTree::delayedReload()
{
  QTimer::singleShot( 0, this, SLOT(reload()) );
}

#include "kmfoldertree.moc"
