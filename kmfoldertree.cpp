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

#include <maillistdrag.h>
using namespace KPIM;

#include <kapplication.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kmenu.h>
#include <kdebug.h>

#include <QCursor>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>
#include <QPainter>
#include <QResizeEvent>
#include <QRegExp>

#include <unistd.h>
#include <assert.h>

//=============================================================================

KMFolderTreeItem::KMFolderTreeItem( KFolderTree *parent, const QString & name,
                                    KFolderTreeItem::Protocol protocol )
  : QObject( parent ),
    KFolderTreeItem( parent, name, protocol, Root ),
    mFolder( 0 ), mNeedsRepaint( true )
{
  setObjectName( name );
  init();
  setPixmap( 0, normalIcon() );
}

//-----------------------------------------------------------------------------
KMFolderTreeItem::KMFolderTreeItem( KFolderTree *parent, const QString & name,
                    KMFolder* folder )
  : QObject( parent ),
    KFolderTreeItem( parent, name ),
    mFolder( folder ), mNeedsRepaint( true )
{
  setObjectName( name );
  init();
  setPixmap( 0, normalIcon() );
}

//-----------------------------------------------------------------------------
KMFolderTreeItem::KMFolderTreeItem( KFolderTreeItem *parent, const QString & name,
                    KMFolder* folder )
  : QObject( 0 ),
    KFolderTreeItem( parent, name ),
    mFolder( folder ), mNeedsRepaint( true )
{
  setObjectName( name );
  init();
  setPixmap( 0, normalIcon() );
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
  if ( (!mFolder && type() == Root) || depth() == 0 ) {
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
      default: icon = kmkernel->iCalIface().folderPixmap( type() ); break;
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
  QPixmap pm = il->loadIcon( icon, K3Icon::Small, size,
                             K3Icon::DefaultState, 0, true );
  if ( mFolder && pm.isNull() ) {
      pm = il->loadIcon( mFolder->normalIconPath(), K3Icon::Small, size,
                         K3Icon::DefaultState, 0, true );
  }

  return pm;
}

QPixmap KMFolderTreeItem::unreadIcon(int size) const
{
  QPixmap pm;

  if ( !mFolder || depth() == 0 || mFolder->isSystemFolder()
    || kmkernel->folderIsTrash( mFolder )
    || kmkernel->folderIsDraftOrOutbox( mFolder ) )
    pm = normalIcon( size );

  KIconLoader * il = KGlobal::instance()->iconLoader();
  if ( mFolder && mFolder->useCustomIcons() ) {
    pm = il->loadIcon( mFolder->unreadIconPath(), K3Icon::Small, size,
                       K3Icon::DefaultState, 0, true );
    if ( pm.isNull() )
      pm = il->loadIcon( mFolder->normalIconPath(), K3Icon::Small, size,
                         K3Icon::DefaultState, 0, true );
  }
  if ( pm.isNull() ) {
    if ( mFolder && mFolder->noContent() ) {
      pm = il->loadIcon( "folder_grey_open", K3Icon::Small, size,
                         K3Icon::DefaultState, 0, true );
    } else {
      pm = il->loadIcon( kmkernel->iCalIface().folderPixmap( type() ),
                         K3Icon::Small, size, K3Icon::DefaultState, 0, true );
      if ( pm.isNull() )
        pm = il->loadIcon( "folder_open", K3Icon::Small, size,
                           K3Icon::DefaultState, 0, true );
    }
  }

  return pm;
}

void KMFolderTreeItem::init()
{
  if ( !mFolder )
    return;

  setProtocol( protocolFor( mFolder->folderType() ) );

  if ( depth() == 0 )
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

  KMFolderTree* tree = static_cast<KMFolderTree*>( listView() );
  tree->insertIntoFolderToItemMap( mFolder, this );
}

void KMFolderTreeItem::adjustUnreadCount( int newUnreadCount ) {
  // adjust the icons if the folder is now newly unread or
  // now newly not-unread
  if ( newUnreadCount != 0 && unreadCount() == 0 )
    setPixmap( 0, unreadIcon() );
  if ( unreadCount() != 0 && newUnreadCount == 0 )
    setPixmap( 0, normalIcon() );

  setUnreadCount( newUnreadCount );
}

void KMFolderTreeItem::slotIconsChanged()
{
  kDebug(5006) << k_funcinfo << endl;
  // this is prone to change, so better check
  if( kmkernel->iCalIface().isResourceFolder( mFolder ) )
      setType( kmkernel->iCalIface().folderType(mFolder) );

  if ( unreadCount() > 0 )
    setPixmap( 0, unreadIcon() );
  else
    setPixmap( 0, normalIcon() );
  emit iconChanged( this );
  repaint();
}

void KMFolderTreeItem::slotNameChanged()
{
  setText( 0, mFolder->label() );
  emit nameChanged( this );
  repaint();
}


//-----------------------------------------------------------------------------
bool KMFolderTreeItem::acceptDrag(QDropEvent*) const
{
  if ( !mFolder || mFolder->isReadOnly() ||
      (mFolder->noContent() && childCount() == 0) ||
       (mFolder->noContent() && isOpen()) ) {
    return false;
  }
  else {
    return true;
  }
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

  KMFolderTree* tree = static_cast<KMFolderTree*>( listView() );
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
              static_cast<KMFolderTree *>( listView() )->mainWidget(),
              listView() );
  shorty->exec();
  return;
}


//=============================================================================


KMFolderTree::KMFolderTree( KMMainWidget *mainWidget, QWidget *parent,
                            const char *name )
  : KFolderTree( parent, name )
{
  oldSelected = 0;
  oldCurrent = 0;
  mLastItem = 0;
  mMainWidget = mainWidget;
  mReloading = false;

  mUpdateCountTimer= new QTimer( this );

  addAcceptableDropMimetype(MailListDrag::format(), false);

  int namecol = addColumn( i18n("Folder"), 250 );
  header()->setStretchEnabled( true, namecol );

  // connect
  connectSignals();

  // popup to switch columns
  header()->setClickEnabled(true);
  header()->installEventFilter(this);
  mPopup = new KMenu(this);
  mPopup->addTitle(i18n("View Columns"));
  mPopup->setCheckable(true);
  mUnreadPop = mPopup->insertItem(i18n("Unread Column"), this, SLOT(slotToggleUnreadColumn()));
  mTotalPop = mPopup->insertItem(i18n("Total Column"), this, SLOT(slotToggleTotalColumn()));
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

  connect(kmkernel->searchFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
          this, SLOT(slotFolderRemoved(KMFolder*)));

  connect( &autoopen_timer, SIGNAL( timeout() ),
           this, SLOT( openFolder() ) );

  connect( this, SIGNAL( contextMenuRequested( Q3ListViewItem*, const QPoint &, int ) ),
           this, SLOT( slotContextMenuRequested( Q3ListViewItem*, const QPoint & ) ) );

  connect( this, SIGNAL( expanded( Q3ListViewItem* ) ),
           this, SLOT( slotFolderExpanded( Q3ListViewItem* ) ) );

  connect( this, SIGNAL( collapsed( Q3ListViewItem* ) ),
           this, SLOT( slotFolderCollapsed( Q3ListViewItem* ) ) );

  connect( this, SIGNAL( itemRenamed( Q3ListViewItem*, int, const QString &)),
           this, SLOT( slotRenameFolder( Q3ListViewItem*, int, const QString &)));
}

//-----------------------------------------------------------------------------
bool KMFolderTree::event(QEvent *e)
{
  if (e->type() == QEvent::ApplicationPaletteChange)
  {
     readColorConfig();
     return true;
  }
  return K3ListView::event(e);
}

//-----------------------------------------------------------------------------
void KMFolderTree::readColorConfig (void)
{
  // Custom/System color support
  KConfigGroup conf( KMKernel::config(), "Reader" );
  QColor c1=QColor(kapp->palette().active().text());
  QColor c2=QColor("blue");
  QColor c4=QColor(kapp->palette().active().base());

  if (!conf.readEntry( "defaultColors", true ) ) {
    mPaintInfo.colFore = conf.readEntry( "ForegroundColor",
                                        QVariant( &c1 ) ).value<QColor>();
    mPaintInfo.colUnread = conf.readEntry( "UnreadMessage",
                                        QVariant( &c2 ) ).value<QColor>();
    mPaintInfo.colBack = conf.readEntry( "BackgroundColor",
                                        QVariant( &c4 ) ).value<QColor>();
  }
  else {
    mPaintInfo.colFore = c1;
    mPaintInfo.colUnread = c2;
    mPaintInfo.colBack = c4;
  }
  QPalette newPal = kapp->palette();
  newPal.setColor( QColorGroup::Base, mPaintInfo.colBack );
  newPal.setColor( QColorGroup::Text, mPaintInfo.colFore );
  setPalette( newPal );
}

//-----------------------------------------------------------------------------
void KMFolderTree::readConfig (void)
{
  readColorConfig();

  // Custom/Ssystem font support
  {
    KConfigGroup conf( KMKernel::config(), "Fonts" );
    if (!conf.readEntry( "defaultFonts", true ) ) {
      QFont folderFont( KGlobalSettings::generalFont() );
      setFont(conf.readEntry("folder-font",
          QVariant( &folderFont ) ).value<QFont>() );
    }
    else
      setFont(KGlobalSettings::generalFont());
  }

  // restore the layout
  restoreLayout( KMKernel::config(), "Geometry" );
}

//-----------------------------------------------------------------------------
// Save the configuration file
void KMFolderTree::writeConfig()
{
  // save the current state of the folders
  for ( Q3ListViewItemIterator it( this ) ; it.current() ; ++it ) {
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
  QList<KMFolderNode*>::const_iterator it;
  for ( it = fdir->constBegin();
      ( folderNode = *it ) && ( it != fdir->constEnd() );
      ++it )
  {
    if (!folderNode->isDir()) {
      folder = static_cast<KMFolder*>(folderNode);

      folder->open();
      folder->countUnread();
      folder->close();
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
    kDebug(5006) << "KMFolderTree::reload - already reloading" << endl;
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
  for ( Q3ListViewItemIterator it( this ) ; it.current() ; ++it ) {
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
    mUpdateIterator = Q3ListViewItemIterator (this);
    QTimer::singleShot( 0, this, SLOT(slotUpdateOneCount()) );
  }

  for ( Q3ListViewItemIterator it( this ) ; it.current() ; ++it ) {
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

    // With the use of slotUpdateCountsDelayed is not necesary
    // a specific processing for Imap
#if 0
    if (fti->folder()->folderType() == KMFolderTypeImap) {
      // imap-only
      KMFolderImap *imapFolder =
        dynamic_cast<KMFolderImap*> ( fti->folder()->storage() );
      disconnect( imapFolder, SIGNAL(folderComplete(KMFolderImap*, bool)),
          this,SLOT(slotUpdateCounts(KMFolderImap*, bool)));
      connect( imapFolder, SIGNAL(folderComplete(KMFolderImap*, bool)),
          this,SLOT(slotUpdateCounts(KMFolderImap*, bool)));
    } else {*/
#endif

    // we want to be noticed of changes to update the unread/total columns
    disconnect(fti->folder(), SIGNAL(msgAdded(KMFolder*,quint32)),
        this,SLOT(slotUpdateCountsDelayed(KMFolder*)));
    connect(fti->folder(), SIGNAL(msgAdded(KMFolder*,quint32)),
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

    disconnect(fti->folder(), SIGNAL(shortcutChanged(KMFolder*)),
               mMainWidget, SLOT( slotShortcutChanged(KMFolder*)));
    connect(fti->folder(), SIGNAL(shortcutChanged(KMFolder*)),
            mMainWidget, SLOT( slotShortcutChanged(KMFolder*)));

    if (!openFolders)
      slotUpdateCounts(fti->folder());
  }
  ensureVisible(0, top + visibleHeight(), 0, 0);
  // if current and selected folder did not change set it again
  for ( Q3ListViewItemIterator it( this ) ; it.current() ; ++it )
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
  if (!open) fti->folder()->open();
  slotUpdateCounts(fti->folder());
  // restore previous state
  if (!open) fti->folder()->close();

  QTimer::singleShot( 0, this, SLOT(slotUpdateOneCount()) );
}

//-----------------------------------------------------------------------------
// Recursively add a directory of folders to the tree of folders
void KMFolderTree::addDirectory( KMFolderDir *fdir, KMFolderTreeItem* parent )
{
  QList<KMFolderNode*>::const_iterator it;
  KMFolderNode * node;
  for ( it = fdir->constBegin();
      ( node = *it ) && ( it != fdir->constEnd() );
      ++it ) {
    if ( node->isDir() )
      continue;

    KMFolder * folder = static_cast<KMFolder*>(node);
    KMFolderTreeItem * fti = 0;
    if (!parent)
    {
      // create new root-item
      // it needs a folder e.g. to save it's state (open/close)
      fti = new KMFolderTreeItem( this, folder->label(), folder );
      fti->setExpandable( true );
    } else {
      // Check if this is an IMAP resource folder
      if ( kmkernel->iCalIface().hideResourceFolder( folder ) )
        // It is
        continue;

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

      connect (fti, SIGNAL(iconChanged(KMFolderTreeItem*)),
          this, SIGNAL(iconChanged(KMFolderTreeItem*)));
      connect (fti, SIGNAL(nameChanged(KMFolderTreeItem*)),
          this, SIGNAL(nameChanged(KMFolderTreeItem*)));

    }
    // restore last open-state
    fti->setOpen( readIsListViewItemOpen(fti) );

    // add child-folders
    if (folder && folder->child()) {
      addDirectory( folder->child(), fti );
    }
   } // for-end
}

//-----------------------------------------------------------------------------
// Initiate a delayed refresh of the tree
void KMFolderTree::refresh()
{
  mUpdateTimer.start(200);
}

//-----------------------------------------------------------------------------
// Updates the pixmap and extendedLabel information for items
void KMFolderTree::delayedUpdate()
{
  bool upd = isUpdatesEnabled();
  if ( upd ) {
    setUpdatesEnabled(false);

    for ( Q3ListViewItemIterator it( this ) ; it.current() ; ++it ) {
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
void KMFolderTree::slotFolderRemoved(KMFolder *aFolder)
{
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>
    (indexOfFolder(aFolder));
  if (!fti || !fti->folder()) return;
  if (fti == currentItem())
  {
    Q3ListViewItem *qlvi = fti->itemAbove();
    if (!qlvi) qlvi = fti->itemBelow();
    doFolderSelected( qlvi );
  }
  removeFromFolderToItemMap( aFolder );
  delete fti;
}

//-----------------------------------------------------------------------------
// Methods for navigating folders with the keyboard
void KMFolderTree::prepareItem( KMFolderTreeItem* fti )
{
  for ( Q3ListViewItem * parent = fti->parent() ; parent ; parent = parent->parent() )
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
  Q3ListViewItemIterator it( currentItem() ? currentItem() : firstChild() );
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
      // Skip drafts and sent mail as well, when reading mail with the space bar
      // but not when changing into the next folder with unread mail via ctrl+ or
      // ctrl- so we do this only if (confirm == true), which means we are doing
      // readOn.
      if ( fti->type() == KFolderTreeItem::Drafts ||
           fti->type() == KFolderTreeItem::SentMail )
        return false;

      //  warn user that going to next folder - but keep track of
      //  whether he wishes to be notified again in "AskNextFolder"
      //  parameter (kept in the config file for kmail)
      if ( KMessageBox::questionYesNo( this,
            i18n( "<qt>Go to the next unread message in folder <b>%1</b>?</qt>" ,
              fti->folder()->label() ),
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
  Q3ListViewItemIterator it( currentItem() ? currentItem() : lastItem() );
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
  Q3ListViewItemIterator it( currentItem() );
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
  Q3ListViewItemIterator it( currentItem() );
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

//-----------------------------------------------------------------------------
// When not dragging and dropping a change in the selected item
// indicates the user has changed the active folder emit a signal
// so that the header list and reader window can be udpated.
void KMFolderTree::doFolderSelected( Q3ListViewItem* qlvi )
{
  if (!qlvi) return;
  if ( mLastItem && mLastItem == qlvi )
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

  clearSelection();
  setCurrentItem( qlvi );
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
  KConfigGroup conf( KMKernel::config(), "Geometry" );
  conf.writeEntry(objectName(), size().width());

  K3ListView::resizeEvent(e);
}

//-----------------------------------------------------------------------------
// show context menu
void KMFolderTree::slotContextMenuRequested( Q3ListViewItem *lvi,
                                             const QPoint &p )
{
  if (!lvi)
    return;
  setCurrentItem( lvi );
  setSelected( lvi, true );

  if (!mMainWidget) return; // safe bet

  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(lvi);
  if ( fti != mLastItem )
    doFolderSelected( fti );

  if (!fti )
    return;

  KMenu *folderMenu = new KMenu;
  if (fti->folder()) folderMenu->addTitle(fti->folder()->label());

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

    if (fti->folder() || (fti->text(0) != i18n("Searches")))
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

    folderMenu->addSeparator();
    if ( !fti->folder()->noChildren() ) {
      folderMenu->insertItem(SmallIconSet("folder_new"),
                             i18n("&New Subfolder..."), this,
                             SLOT(addChildFolder()));
    }

    if ( fti->folder()->isMoveable() )
    {
      QMenu *moveMenu = new QMenu( folderMenu );
      folderToPopupMenu( MoveFolder, this, &mMenuToFolder, moveMenu );
      folderMenu->insertItem( i18n("&Move Folder To"), moveMenu );
    }

    // Want to be able to display properties for ALL folders,
    // so we can edit expiry properties.
    // -- smp.
    if (!fti->folder()->noContent())
    {
      mMainWidget->action("search_messages")->plug(folderMenu);

      mMainWidget->action("compact")->plug(folderMenu);

      folderMenu->addSeparator();
      if ( !fti->folder()->isSystemFolder() ) {
        mMainWidget->action("delete_folder")->plug(folderMenu);
        folderMenu->addSeparator();
      }
      mMainWidget->action("empty")->plug(folderMenu);
      folderMenu->addSeparator();
    }
  }

  /* plug in IMAP and DIMAP specific things */
  if (fti->folder() &&
      (fti->folder()->folderType() == KMFolderTypeImap ||
       fti->folder()->folderType() == KMFolderTypeCachedImap ))
  {
    folderMenu->insertItem(SmallIconSet("bookmark_folder"),
        i18n("Subscription..."), mMainWidget,
        SLOT(slotSubscriptionDialog()));

    if (!fti->folder()->noContent())
    {
      mMainWidget->action("refresh_folder")->plug(folderMenu);
      if ( fti->folder()->folderType() == KMFolderTypeImap ) {
        folderMenu->insertItem(SmallIconSet("reload"), i18n("Refresh Folder List"), this,
            SLOT(slotResetFolderList()));
      }
    }
    if ( fti->folder()->folderType() == KMFolderTypeCachedImap ) {
      KMFolderCachedImap * folder = static_cast<KMFolderCachedImap*>( fti->folder()->storage() );
      folderMenu->insertItem( SmallIconSet("wizard"),
                              i18n("&Troubleshoot IMAP Cache..."),
                              folder, SLOT(slotTroubleshoot()) );
    }
    folderMenu->addSeparator();
  }

  if ( fti->folder() && fti->folder()->isMailingListEnabled() ) {
    mMainWidget->action("post_message")->plug(folderMenu);
  }

  if (fti->folder() && fti->parent())
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
// If middle button and folder holds mailing-list, create a message to that list
void KMFolderTree::contentsMouseReleaseEvent(QMouseEvent* me)
{
  Q3ListViewItem *lvi = currentItem(); // Needed for when branches are clicked on
  Qt::MouseButton btn = me->button();
  doFolderSelected(lvi);

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
                                    "</qt> " , aFolder->label());
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
  KConfigGroup config( KMKernel::config(), name );

  return config.readEntry( "isOpen", false );
}

//-----------------------------------------------------------------------------
// Saves open/closed state of a folder directory into the config file
void KMFolderTree::writeIsListViewItemOpen(KMFolderTreeItem *fti)
{
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
  KConfigGroup config( KMKernel::config(), name );
  config.writeEntry("isOpen", fti->isOpen() );
}


//-----------------------------------------------------------------------------
void KMFolderTree::cleanupConfigFile()
{
  if ( childCount() == 0 )
    return; // just in case reload wasn't called before
  KConfig* config = KMKernel::config();
  QStringList existingFolders;
  Q3ListViewItemIterator fldIt(this);
  QMap<QString,bool> folderMap;
  KMFolderTreeItem *fti;
  for (Q3ListViewItemIterator fldIt(this); fldIt.current(); fldIt++)
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
    if (!folderMap.contains(name) )
    {
      KMFolder* folder = kmkernel->findFolderById( name );
      if ( folder && kmkernel->iCalIface().hideResourceFolder( folder ) )
        continue; // hidden IMAP resource folder, don't delete info

      //KMessageBox::error( 0, "cleanupConfigFile: Deleting group " + *grpIt );
      config->deleteGroup(*grpIt, KConfig::NLS);
      kDebug(5006) << "Deleting information about folder " << name << endl;
    }
  }
}


//-----------------------------------------------------------------------------
// Drag and Drop handling -- based on the Troll Tech dirview example

enum {
  DRAG_COPY = 0,
  DRAG_MOVE = 1,
  DRAG_CANCEL = 2
};

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
  for ( Q3ListViewItemIterator it( this ) ; it.current() ; ++it )
    if ( it.current()->isSelected() )
      oldSelected = it.current();

  setFocus();

  Q3ListViewItem *i = itemAt( contentsToViewport(e->pos()) );
  if ( i ) {
    dropItem = i;
    autoopen_timer.start( autoopenTime );
  }
  e->accept( acceptDrag(e) );
}

//-----------------------------------------------------------------------------
void KMFolderTree::contentsDragMoveEvent( QDragMoveEvent *e )
{
    QPoint vp = contentsToViewport(e->pos());
    Q3ListViewItem *i = itemAt( vp );
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

    Q3ListViewItem *item = itemAt( contentsToViewport(e->pos()) );
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);
    if (fti && (fti != oldSelected) && (fti->folder()) && acceptDrag(e))
    {
      int keybstate = kapp->keyboardModifiers();
      if ( keybstate & Qt::ControlModifier ) {
        emit folderDropCopy(fti->folder());
      } else if ( keybstate & Qt::ShiftModifier ) {
        emit folderDrop(fti->folder());
      } else {
        if ( GlobalSettings::self()->showPopupAfterDnD() ) {
          KMenu *menu = new KMenu( this );
          const QAction *dragMove = menu->addAction( i18n("&Move Here") );
          const QAction *dragCopy = menu->addAction( SmallIcon("editcopy"), i18n("&Copy Here") );
          menu->addSeparator();
          menu->addAction( SmallIcon("cancel"), i18n("C&ancel") );
          const QAction *selectedAction = menu->exec( QCursor::pos() );
          if ( selectedAction == dragMove )
            emit folderDrop( fti->folder() );
          else if ( selectedAction == dragCopy )
            emit folderDropCopy( fti->folder() );
        }
        else
          emit folderDrop(fti->folder());
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
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotFolderExpanded( Q3ListViewItem * item )
{
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);

  if ( fti && fti->folder() &&
       fti->folder()->folderType() == KMFolderTypeImap )
  {
    KMFolderImap *folder = static_cast<KMFolderImap*>( fti->folder()->storage() );
    // if we should list all folders we limit this to the root folder
    if ( !folder->account()->listOnlyOpenFolders() &&
         fti->parent() )
      return;
    if ( folder->getSubfolderState() == KMFolderImap::imapNoInformation )
    {
      // check if all parents are expanded
      Q3ListViewItem *parent = item->parent();
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
void KMFolderTree::slotFolderCollapsed( Q3ListViewItem * item )
{
  slotResetFolderList( item, false );
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotRenameFolder(Q3ListViewItem *item, int col,
                const QString &text)
{

  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);

  if ((!fti) || (fti && fti->folder() && col != 0 && !currentFolder()->child()))
          return;

  QString fldName, oldFldName;

  oldFldName = fti->objectName();

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
void KMFolderTree::slotUpdateCounts(KMFolderImap * folder, bool success)
{
  if (success) slotUpdateCounts(folder->folder());
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotUpdateCountsDelayed(KMFolder * folder)
{
//  kDebug(5006) << "KMFolderTree::slotUpdateCountsDelayed()" << endl;
  if ( !mFolderToUpdateCount.contains( folder->idString() ) )
  {
//    kDebug( 5006 )<< "adding " << folder->idString() << " to updateCountList " << endl;
    mFolderToUpdateCount.insert( folder->idString(),folder );
  }
  if ( !mUpdateCountTimer->isActive() )
    mUpdateCountTimer->start( 500 );
}


void KMFolderTree::slotUpdateCountTimeout()
{
//  kDebug(5006) << "KMFolderTree::slotUpdateCountTimeout()" << endl;

  QMap<QString,KMFolder*>::iterator it;
  for ( it= mFolderToUpdateCount.begin();
      it!=mFolderToUpdateCount.end();
      ++it )
  {
    slotUpdateCounts( it.value() );
  }
  mFolderToUpdateCount.clear();
  mUpdateCountTimer->stop();

}

void KMFolderTree::slotUpdateCounts(KMFolder * folder)
{
 // kDebug(5006) << "KMFolderTree::slotUpdateCounts()" << endl;
  Q3ListViewItem * current;
  if (folder)
    current = indexOfFolder(folder);
  else
    current = currentItem();

  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(current);
  // sanity check
  if (!fti) return;
  if (!fti->folder()) fti->setTotalCount(-1);

  // get the unread count
  int count = 0;
  if (folder && folder->noContent()) // always empty
    count = -1;
  else {
    if ( fti->folder() )
      count = fti->folder()->countUnread();
  }

  // set it
  bool repaint = false;
  if (fti->unreadCount() != count) {
     fti->adjustUnreadCount( count );
     repaint = true;
  }
  if (isTotalActive())
  {
    // get the total-count
    if (fti->folder()->noContent())
      count = -1;
    else {
      // get the cached count if the folder is not open
      count = fti->folder()->count( !fti->folder()->isOpened() );
    }
    // set it
    if ( count != fti->totalCount() ) {
      fti->setTotalCount(count);
      repaint = true;
    }
  }
  if (fti->parent() && !fti->parent()->isOpen())
    repaint = false; // we're not visible
  if (repaint) {
    fti->setNeedsRepaint( true );
    refresh();
  }
  // tell the kernel that one of the counts has changed
  kmkernel->messageCountChanged();
}

void KMFolderTree::updatePopup() const
{
   mPopup->setItemChecked( mUnreadPop, isUnreadActive() );
   mPopup->setItemChecked( mTotalPop, isTotalActive() );
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
    // toggle KMenu
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
    // toggle KMenu
    mPopup->setItemChecked( mTotalPop, isTotalActive() );

  } else kDebug(5006) << "unknown column:" << column << endl;

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
bool KMFolderTree::eventFilter( QObject *o, QEvent *e )
{
  if ( e->type() == QEvent::MouseButtonPress &&
      static_cast<QMouseEvent*>(e)->button() == Qt::RightButton &&
      o->metaObject()->className() == "QHeader" )
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
  if (folder && folder->folderType() == KMFolderTypeImap)
  {
    KMAccount* acct = static_cast<KMFolderImap*>(folder->storage())->account();
    kmkernel->acctMgr()->singleCheckMail(acct, true);
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
                                     QList<QPointer<KMFolder> > *folders,
                                     bool localFolders,
                                     bool imapFolders,
                                     bool dimapFolders,
                                     bool searchFolders,
                                     bool includeNoContent,
                                     bool includeNoChildren )
{
  for ( Q3ListViewItemIterator it( this ) ; it.current() ; ++it )
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
void KMFolderTree::slotResetFolderList( Q3ListViewItem* item, bool startList )
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
  Q3ListViewItem* item = indexOfFolder( folder );
  if ( item )
  {
    doFolderSelected( item );
    ensureItemVisible( item );
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::folderToPopupMenu( MenuAction action, QObject *receiver,
    KMMenuToFolder *aMenuToFolder, QMenu *menu, Q3ListViewItem *item )
{
#warning Port me!
/*  while ( menu->count() )
  {
    Q3PopupMenu *popup = menu->findItem( menu->idAt( 0 ) )->popup();
    if ( popup )
      delete popup;
    else
      menu->removeItemAt( 0 );
  }*/
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
      QMenu* popup = new QMenu( menu );
      popup->setObjectName( "subMenu" );
      folderToPopupMenu( action, receiver, aMenuToFolder, popup, fti->firstChild() );
      bool subMenu = false;
      if ( ( action == MoveMessage || action == CopyMessage ) &&
           fti->folder() && !fti->folder()->noContent() )
        subMenu = true;
      if ( action == MoveFolder && ( !fti->folder() ||
            ( fti->folder() && !fti->folder()->noChildren() ) ) )
        subMenu = true;
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
  moveFolder( mMenuToFolder[menuId] );
}

//-----------------------------------------------------------------------------
void KMFolderTree::moveFolder( KMFolder* destination )
{
  KMFolder* folder = currentFolder();
  if (!folder)
	return;

  KMFolderDir* parent = &(kmkernel->folderMgr()->dir());
  if ( destination )
    parent = destination->createChildFolder();
  QString message =
    i18n( "<qt>Cannot move folder <b>%1</b> into a subfolder below itself.</qt>" , 
         folder->label() );

  KMFolderDir* folderDir = parent;
  // check that the folder can be moved
  if ( folder->child() )
  {
    while ( folderDir && ( folderDir != &kmkernel->folderMgr()->dir() ) &&
        ( folderDir != folder->parent() ) )
    {
      if ( folderDir->indexOf( folder ) != -1 )
      {
        KMessageBox::error( this, message );
        return;
      }
      folderDir = folderDir->parent();
    }
  }

  if( folder->child() && parent &&
      ( parent->path().startsWith( folder->child()->path() + "/" ) ) ) {
    KMessageBox::error( this, message );
    return;
  }

  if( folder->child()
      && ( parent == folder->child() ) ) {
    KMessageBox::error( this, message );
    return;
  }

  kDebug(5006) << "move folder " << currentFolder()->label() << " to "
    << ( destination ? destination->label() : "Local Folders" ) << endl;
  kmkernel->folderMgr()->moveFolder( folder, parent );
}

#include "kmfoldertree.moc"

