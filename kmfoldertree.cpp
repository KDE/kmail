// kmfoldertree.cpp
#include "kmfoldertree.h"

#include "kmfoldermgr.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmfolderdia.h"
#include "kmcomposewin.h"
#include "kmmainwidget.h"
#include "kmailicalifaceimpl.h"
#include "kmacctmgr.h"
#include "kmkernel.h"

#include <maillistdrag.h>
using namespace KPIM;

#include <kapplication.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kdebug.h>

#include <qpainter.h>
#include <qcursor.h>
#include <qregexp.h>

#include <unistd.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <fixx11h.h>

//=============================================================================

KMFolderTreeItem::KMFolderTreeItem( KFolderTree *parent, const QString & name,
				    KFolderTreeItem::Protocol protocol )
  : QObject( parent, name.latin1() ),
    KFolderTreeItem( parent, name, protocol, Root ),
    mFolder( 0 )
{
  init();
  setPixmap( 0, normalIcon() );
}

//-----------------------------------------------------------------------------
KMFolderTreeItem::KMFolderTreeItem( KFolderTree *parent, const QString & name,
                    KMFolder* folder )
  : QObject( parent, name.latin1() ),
    KFolderTreeItem( parent, name ),
    mFolder( folder )
{
  init();
  setPixmap( 0, normalIcon() );
}

//-----------------------------------------------------------------------------
KMFolderTreeItem::KMFolderTreeItem( KFolderTreeItem *parent, const QString & name,
                    KMFolder* folder )
  : QObject( 0, name.latin1() ),
    KFolderTreeItem( parent, name ),
    mFolder( folder )
{
  init();
  setPixmap( 0, normalIcon() );
}

KMFolderTreeItem::~KMFolderTreeItem() {
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
  } else if ( mFolder->isSystemFolder() ) {
    switch ( type() ) {
      case Inbox: icon = "folder_inbox"; break;
      case Outbox: icon = "folder_outbox"; break;
      case SentMail: icon = "folder_sent_mail"; break;
      case Trash: icon = "trashcan_empty"; break;
      default: icon = kmkernel->iCalIface().folderPixmap( type() ); break;
      case Drafts: icon = "edit";break;
    }
  } else if ( protocol() == KMFolderTreeItem::Search) {
    icon = "mail_find";
  }

  if ( icon.isEmpty() )
    icon = "folder";

  if (mFolder && mFolder->useCustomIcons() ) {
    icon = mFolder->normalIconPath();
  }
  KIconLoader * il = KGlobal::instance()->iconLoader();
  QPixmap pm = il->loadIcon( icon, KIcon::Small, size,
                             KIcon::DefaultState, 0, true );
  if ( pm.isNull() ) {
      pm = il->loadIcon( mFolder->normalIconPath(), KIcon::Small, size,
                         KIcon::DefaultState, 0, true );
  }

  return pm;
}

QPixmap KMFolderTreeItem::unreadIcon(int size) const
{
  QPixmap pm;

  if ( !mFolder || depth() == 0 || mFolder->isSystemFolder() )
    pm = normalIcon( size );

  KIconLoader * il = KGlobal::instance()->iconLoader();
  if ( mFolder->useCustomIcons() ) {
    pm = il->loadIcon( mFolder->unreadIconPath(), KIcon::Small, size,
                       KIcon::DefaultState, 0, true );
    if ( pm.isNull() )
      pm = il->loadIcon( mFolder->normalIconPath(), KIcon::Small, size,
                         KIcon::DefaultState, 0, true );
  }
  if ( pm.isNull() )
    pm = il->loadIcon( "folder_open", KIcon::Small, size,
                       KIcon::DefaultState, 0, true );

  return pm;
}

void KMFolderTreeItem::init()
{
  if ( !mFolder )
    return;

  setProtocol( protocolFor( mFolder->folderType() ) );

  if ( depth() == 0 )
    setType(Root);
  else if (mFolder->isSystemFolder()) {
    if (mFolder == kmkernel->inboxFolder()
	|| mFolder->folderType() == KMFolderTypeImap)
      setType(Inbox);
    else if (mFolder == kmkernel->outboxFolder())
      setType(Outbox);
    else if (mFolder == kmkernel->sentFolder())
      setType(SentMail);
    else if (mFolder == kmkernel->draftsFolder())
      setType(Drafts);
    else if (mFolder == kmkernel->trashFolder())
      setType(Trash);
    else if(kmkernel->iCalIface().isResourceImapFolder(mFolder))
      setType(kmkernel->iCalIface().folderType(mFolder));
  } else
    setRenameEnabled(0, false);
}

bool KMFolderTreeItem::adjustUnreadCount() {
  if ( !folder() )
    return false;
  const int count = folder()->countUnread();
  //if ( count == unreadCount() )
  //return false;
  setUnreadCount( count );
  if ( count > 0 )
    setPixmap( 0, unreadIcon() );
  else
    setPixmap( 0, normalIcon() );

  return true;
}

void KMFolderTreeItem::slotRepaint() {
  if ( unreadCount() > 0 )
    setPixmap( 0, unreadIcon() );
  else
    setPixmap( 0, normalIcon() );
  emit iconChanged( this );
  repaint();
}


//-----------------------------------------------------------------------------
bool KMFolderTreeItem::acceptDrag(QDropEvent*) const
{
  if ( !mFolder ||
      (mFolder->noContent() && childCount() == 0) ||
      (mFolder->noContent() && isOpen()) )
    return false;
  else
    return true;
}

//-----------------------------------------------------------------------------
void KMFolderTreeItem::properties()
{
  KMFolderDialog *props;

  props = new KMFolderDialog( mFolder, mFolder->parent(), 0,
                              i18n("Properties of Folder %1").arg( mFolder->label() ) );
  props->exec();
  //Nothing here the above exec() may actually delete this KMFolderTreeItem
  return;
}

//-----------------------------------------------------------------------------
//=============================================================================


KMFolderTree::KMFolderTree( KMMainWidget *mainWidget, QWidget *parent,
                            const char *name )
  : KFolderTree( parent, name )
{
  oldSelected = 0;
  oldCurrent = 0;
  mLastItem = 0;
  mMainWidget = mainWidget;

  addAcceptableDropMimetype(MailListDrag::format(), false);

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
}

//-----------------------------------------------------------------------------
// connects all needed signals to their slots
void KMFolderTree::connectSignals()
{
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

  connect( &autoscroll_timer, SIGNAL( timeout() ),
	   this, SLOT( autoScroll() ) );

  connect( this, SIGNAL( contextMenuRequested( QListViewItem*, const QPoint &, int ) ),
	   this, SLOT( slotContextMenuRequested( QListViewItem*, const QPoint & ) ) );

  connect( this, SIGNAL( expanded( QListViewItem* ) ),
           this, SLOT( slotFolderExpanded( QListViewItem* ) ) );

  connect( this, SIGNAL( collapsed( QListViewItem* ) ),
           this, SLOT( slotFolderCollapsed( QListViewItem* ) ) );

  connect( this, SIGNAL( itemRenamed( QListViewItem*, int, const QString &)),
	   this, SLOT( slotRenameFolder( QListViewItem*, int, const QString &)));
}

//-----------------------------------------------------------------------------
bool KMFolderTree::event(QEvent *e)
{
  if (e->type() == QEvent::ApplicationPaletteChange)
  {
     readColorConfig();
     return true;
  }
  bool result = KListView::event(e);

  if ( e->type() == QEvent::KeyPress && currentItem() != mLastItem )
    doFolderSelected( currentItem() );

  return result;
}

//-----------------------------------------------------------------------------
void KMFolderTree::createFolderList(QStringList *str,
  QValueList<QGuardedPtr<KMFolder> > *folders)
{
  for ( QListViewItemIterator it( this ) ; it.current() ; ++it ) {
    KMFolderTreeItem * fti = static_cast<KMFolderTreeItem*>(it.current());
    if ( !fti || !fti->folder() )
      continue;

    QString prefix;
    prefix.fill( ' ', 2 * fti->depth() );
    str->append(prefix + fti->text(0));
    if (fti->folder()->noContent()) folders->append(0);
    else folders->append(fti->folder());
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::createImapFolderList(KMFolderImap *aFolder, QStringList *names,
  QStringList *urls, QStringList *mimeTypes)
{
  for ( QListViewItemIterator it( this ) ; it.current() ; ++it ) {
    KMFolderTreeItem * fti = static_cast<KMFolderTreeItem*>(it.current());
    if ( !fti || !fti->folder() )
      continue;

    KMFolderImap *folder = static_cast<KMFolderImap*>(fti->folder());
    if ( folder != aFolder )
      continue;

    names->append(fti->text(0));
    urls->append(folder->imapPath());
    mimeTypes->append((folder->noContent()) ? "inode/directory" :
          (fti->isExpandable()) ? "message/directory" : "message/digest");
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::readColorConfig (void)
{
  KConfig* conf = KMKernel::config();
  // Custom/System color support
  KConfigGroupSaver saver(conf, "Reader");
  QColor c1=QColor(kapp->palette().active().text());
  QColor c2=QColor("blue");
  QColor c4=QColor(kapp->palette().active().base());

  if (!conf->readBoolEntry("defaultColors",TRUE)) {
    mPaintInfo.colFore = conf->readColorEntry("ForegroundColor",&c1);
    mPaintInfo.colUnread = conf->readColorEntry("UnreadMessage",&c2);
    mPaintInfo.colBack = conf->readColorEntry("BackgroundColor",&c4);
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
  KConfig* conf = KMKernel::config();
  QString fntStr;

  // Backing pixmap support
  { //area for config group "Pixmaps"
    KConfigGroupSaver saver(conf, "Pixmaps");
    QString pixmapFile = conf->readPathEntry("FolderTree");
    mPaintInfo.pixmapOn = FALSE;
    if (!pixmapFile.isEmpty()) {
      mPaintInfo.pixmapOn = TRUE;
      mPaintInfo.pixmap = QPixmap( pixmapFile );
    }
  }

  readColorConfig();

  // Custom/Ssystem font support
  { //area for config group "Pixmaps"
    KConfigGroupSaver saver(conf, "Fonts");
    if (!conf->readBoolEntry("defaultFonts",TRUE)) {
      QFont folderFont( KGlobalSettings::generalFont() );
      setFont(conf->readFontEntry("folder-font", &folderFont));
    }
    else
      setFont(KGlobalSettings::generalFont());
  }

  // read D'n'D behaviour setting
  KConfigGroup behaviour( KMKernel::config(), "Behaviour" );
  mShowPopupAfterDnD = behaviour.readBoolEntry( "ShowPopupAfterDnD", true );

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
  setUpdatesEnabled(FALSE);

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

      folder->open();
      folder->countUnread();
      folder->close();
    }
  }

  setUpdatesEnabled(upd);
}

//-----------------------------------------------------------------------------
// Draw empty area of list view with support for a backing pixmap
void KMFolderTree::paintEmptyArea( QPainter * p, const QRect & rect )
{
  if (mPaintInfo.pixmapOn)
    p->drawTiledPixmap( rect.left(), rect.top(), rect.width(), rect.height(),
			mPaintInfo.pixmap,
			rect.left() + contentsX(),
			rect.top() + contentsY() );
  else
    p->fillRect( rect, colorGroup().base() );
}

//-----------------------------------------------------------------------------
// Reload the tree of items in the list view
void KMFolderTree::reload(bool openFolders)
{
  int top = contentsY();
  mLastItem = 0;
  for ( QListViewItemIterator it( this ) ; it.current() ; ++it ) {
    KMFolderTreeItem * fti = static_cast<KMFolderTreeItem*>(it.current());
    if (fti && fti->folder())
      disconnect(fti->folder(),SIGNAL(numUnreadMsgsChanged(KMFolder*)),
		 this,SLOT(refresh()));
    writeIsListViewItemOpen( fti );
  }
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

    // first disconnect before each connect
    // to make sure we don't call it several times with each reload
    disconnect(fti->folder(),SIGNAL(numUnreadMsgsChanged(KMFolder*)),
	       this,SLOT(refresh()));
    connect(fti->folder(),SIGNAL(numUnreadMsgsChanged(KMFolder*)),
	    this,SLOT(refresh()));
    disconnect(fti->folder(),SIGNAL(iconsChanged()),
	       fti,SLOT(slotRepaint()));
    connect(fti->folder(),SIGNAL(iconsChanged()),
	    fti,SLOT(slotRepaint()));

    disconnect(fti->folder(),SIGNAL(nameChanged()),
	       fti,SLOT(slotNameChanged()));
    connect(fti->folder(),SIGNAL(nameChanged()),
	    fti,SLOT(slotNameChanged()));

    if (isTotalActive() || isUnreadActive())
    {

      if (fti->folder()->folderType() == KMFolderTypeImap)
      {
	// imap-only
	disconnect(fti->folder(), SIGNAL(folderComplete(KMFolderImap*, bool)),
		   this,SLOT(slotUpdateCounts(KMFolderImap*, bool)));
	connect(fti->folder(), SIGNAL(folderComplete(KMFolderImap*, bool)),
		this,SLOT(slotUpdateCounts(KMFolderImap*, bool)));
      } else {
	// others-only, imap doesn't need this because of the folderComplete-signal
	// we want to be noticed of changes to update the unread/total columns
        disconnect(fti->folder(), SIGNAL(numUnreadMsgsChanged(KMFolder*)),
		   this,SLOT(slotUpdateCounts(KMFolder*)));
        connect(fti->folder(), SIGNAL(numUnreadMsgsChanged(KMFolder*)),
		this,SLOT(slotUpdateCounts(KMFolder*)));
	disconnect(fti->folder(), SIGNAL(msgAdded(KMFolder*,Q_UINT32)),
		   this,SLOT(slotUpdateCounts(KMFolder*)));
	connect(fti->folder(), SIGNAL(msgAdded(KMFolder*,Q_UINT32)),
		this,SLOT(slotUpdateCounts(KMFolder*)));
      }
      disconnect(fti->folder(), SIGNAL(msgRemoved(KMFolder*)),
		 this,SLOT(slotUpdateCounts(KMFolder*)));
      connect(fti->folder(), SIGNAL(msgRemoved(KMFolder*)),
	      this,SLOT(slotUpdateCounts(KMFolder*)));
    }
    if (!openFolders)
      slotUpdateCounts(fti->folder());
  }
  ensureVisible(0, top + visibleHeight(), 0, 0);
  refresh();
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
  for ( KMFolderNode * node = fdir->first() ; node ; node = fdir->next() ) {
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
      // create new child
      fti = new KMFolderTreeItem( parent, folder->label(), folder );
      connect (fti, SIGNAL(iconChanged(KMFolderTreeItem*)),
               this, SIGNAL(iconChanged(KMFolderTreeItem*)));
      connect (fti, SIGNAL(nameChanged(KMFolderTreeItem*)),
               this, SIGNAL(nameChanged(KMFolderTreeItem*)));
    }
    // restore last open-state
    fti->setOpen( readIsListViewItemOpen(fti) );

    // add child-folders
    if (folder && folder->child())
      addDirectory( folder->child(), fti );
    // make sure that the folder-settings are correctly read on startup by calling listDirectory
   if (readIsListViewItemOpen(fti) &&
	fti->folder() && fti->folder()->folderType() == KMFolderTypeImap) {
      disconnect( this, SIGNAL( expanded( QListViewItem* ) ),
           this, SLOT( slotFolderExpanded( QListViewItem* ) ) );
      slotFolderExpanded(fti);
      connect( this, SIGNAL( expanded( QListViewItem* ) ),
           this, SLOT( slotFolderExpanded( QListViewItem* ) ) );
   }
  } // for-end
}
//-----------------------------------------------------------------------------
// The folder neds a refresh!
void KMFolderTree::refresh(KMFolder* folder, bool doUpdate)
{
  reload();
  QListViewItem *qlvi = indexOfFolder( folder );
  if (qlvi)
  {
    qlvi->setOpen(true);
    setCurrentItem( qlvi );
    mMainWidget->slotMsgChanged();

    // FIXME: fugly, fugly hack that shows a deffinite design problem
    // this came over from kmoflderdia
    static_cast<KMHeaders*>(mMainWidget->child("headers"))->setFolder(folder);
  }
  if ( doUpdate )
    delayedUpdate();
}

//-----------------------------------------------------------------------------
// Initiate a delayed refresh of the count of unread messages
// Not really need anymore as count is cached in config file. But causes
// a nice blink in the foldertree, that indicates kmail did something
// when the user manually checks for mail and none was found.
void KMFolderTree::refresh()
{
  mUpdateTimer.changeInterval(200);
}

//-----------------------------------------------------------------------------
// Updates the pixmap and extendedLabel information for items
void KMFolderTree::delayedUpdate()
{
  bool upd = isUpdatesEnabled();
  setUpdatesEnabled(FALSE);

  for ( QListViewItemIterator it( this ) ; it.current() ; ++it ) {
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
    if (!fti || !fti->folder())
      continue;

    bool repaintRequired = fti->adjustUnreadCount();

    if (upd && repaintRequired)
      for (QListViewItem *p = fti; p; p = p->parent())
        p->repaint();
  }
  setUpdatesEnabled(upd);
  mUpdateTimer.stop();
}

//-----------------------------------------------------------------------------
// Folders have been added/deleted update the tree of folders
void KMFolderTree::doFolderListChanged()
{
  KMFolderTreeItem* fti = static_cast< KMFolderTreeItem* >(currentItem());
  KMFolder* folder = (fti) ? fti->folder() : 0;
  reload();
  QListViewItem *qlvi = indexOfFolder(folder);
  if (qlvi) {
    setCurrentItem( qlvi );
    setSelected( qlvi, TRUE );
  }
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
    QListViewItem *qlvi = fti->itemAbove();
    if (!qlvi) qlvi = fti->itemBelow();
    doFolderSelected( qlvi );
  }
  delete fti;
}

//-----------------------------------------------------------------------------
// Methods for navigating folders with the keyboard
void KMFolderTree::prepareItem( KMFolderTreeItem* fti )
{
  for ( QListViewItem * parent = fti->parent() ; parent ; parent = parent->parent() )
    parent->setOpen( TRUE );
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
  if (fti && fti->folder()  &&
      (fti->folder()->countUnread() > 0)) {

    // Don't change into the trash folder.
    if (fti->type() == KFolderTreeItem::Trash)
      return false;

    if (confirm) {
      //  warn user that going to next folder - but keep track of
      //  whether he wishes to be notified again in "AskNextFolder"
      //  parameter (kept in the config file for kmail)
      if ( KMessageBox::questionYesNo( this,
            i18n( "<qt>Go to the next unread message in folder <b>%1</b>?</qt>" )
            .arg( fti->folder()->label() ),
            i18n( "Go to the Next Unread Message" ),
            KStdGuiItem::yes(), KStdGuiItem::no(), // defaults
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
  if (fti && fti->folder()) {
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
  if (fti && fti->folder()) {
      prepareItem( fti );
      setFocus();
      setCurrentItem( fti );
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::selectCurrentFolder()
{
  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>( currentItem() );
  if (fti && fti->folder()) {
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
void KMFolderTree::doFolderSelected( QListViewItem* qlvi )
{
  if (!qlvi) return;

  KMFolderTreeItem* fti = static_cast< KMFolderTreeItem* >(qlvi);
  KMFolder* folder = 0;
  if (fti) folder = fti->folder();

  if (mLastItem && mLastItem != fti && mLastItem->folder()
     && (mLastItem->folder()->folderType() == KMFolderTypeImap))
  {
    KMFolderImap *imapFolder = static_cast<KMFolderImap*>(mLastItem->folder());
    imapFolder->setSelected(FALSE);
    KMAcctImap *act = imapFolder->account();
    if (act)
    {
      // Can't we do without this? -till
      //act->killAllJobs();
      act->setIdle(TRUE);
    }
  }
  mLastItem = fti;

  clearSelection();
  setCurrentItem( qlvi );
  setSelected( qlvi, TRUE );
  if (!folder) {
    emit folderSelected(0); // Root has been selected
  }
  else {
    emit folderSelected(folder);
    if (folder->folderType() == KMFolderTypeImap)
    {
      KMFolderImap *imap_folder = static_cast<KMFolderImap*>(folder);
      imap_folder->setSelected(TRUE);
      if (imap_folder->getContentState() != KMFolderImap::imapInProgress)
        imap_folder->getAndCheckFolder();
    } else {
      // we don't need this for imap-folders because
      // they're updated with the folderComplete-signal
      slotUpdateCounts(folder);
    }
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
QListViewItem* KMFolderTree::indexOfFolder(const KMFolder* folder)
{
  for ( QListViewItemIterator it( this ) ; it.current() ; ++it )
    if ( static_cast<KMFolderTreeItem*>(it.current())->folder() == folder )
      return it.current();
  return 0;
}

//-----------------------------------------------------------------------------
// show context menu
void KMFolderTree::slotContextMenuRequested( QListViewItem *lvi,
                                             const QPoint &p )
{
  if (!lvi)
    return;
  setCurrentItem( lvi );
  setSelected( lvi, TRUE );

  if (!mMainWidget) return; // safe bet

  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(lvi);
  if ( fti != mLastItem )
    doFolderSelected( fti );

  if (!fti )
    return;

  KPopupMenu *folderMenu = new KPopupMenu;
  if (fti->folder()) folderMenu->insertTitle(fti->folder()->label());

  if ((!fti->folder() || (fti->folder()->noContent()
    && !fti->parent())))
  {
    QString createChild = i18n("&New Subfolder...");
    if (!fti->folder()) createChild = i18n("&New Folder...");

    folderMenu->insertItem(SmallIcon("folder_new"),
                           createChild, this,
                           SLOT(addChildFolder()));
    if (!fti->folder()) {
      folderMenu->insertItem(i18n("&Compact All Folders"),
                     kmkernel->folderMgr(), SLOT(compactAll()));
      folderMenu->insertItem(i18n("&Expire All Folders"),
			     kmkernel->folderMgr(), SLOT(expireAll()));
    } else if (fti->folder()->folderType() == KMFolderTypeImap) {
      folderMenu->insertItem(SmallIcon("mail_get"), i18n("Check &Mail"),
        this,
        SLOT(slotCheckMail()));
    }
  } else {
    if ((fti->folder() == kmkernel->outboxFolder()) && (fti->folder()->count()) )
        folderMenu->insertItem(SmallIcon("mail_send"),
                               i18n("&Send Queued Messages"), mMainWidget,
                               SLOT(slotSendQueued()));
    if (!fti->folder()->isSystemFolder() || fti->folder()->folderType() == KMFolderTypeImap)
    {
      folderMenu->insertItem(SmallIcon("folder_new"),
                             i18n("&New Subfolder..."), this,
                             SLOT(addChildFolder()));
    }

    // Want to be able to display properties for ALL folders,
    // so we can edit expiry properties.
    // -- smp.
    if (!fti->folder()->noContent())
    {
      int itemId = folderMenu->insertItem( SmallIcon("goto"),
                                           i18n("Mark All Messages as &Read"),
                                           mMainWidget,
                                           SLOT( slotMarkAllAsRead() ) );
      folderMenu->setItemEnabled( itemId, fti->folder()->countUnread() > 0 );

      folderMenu->insertItem(i18n("&Compact"), mMainWidget,
                             SLOT(slotCompactFolder()));

      itemId = folderMenu->insertItem(i18n("&Expire"), mMainWidget,
                                      SLOT(slotExpireFolder()));
      folderMenu->setItemEnabled( itemId, fti->folder()->isAutoExpire() );


      folderMenu->insertSeparator();

      itemId = folderMenu->insertItem(SmallIcon("edittrash"),
        (kmkernel->folderIsTrash(fti->folder())) ? i18n("&Empty") :
                             i18n("&Move All Messages to Trash"), mMainWidget,
                             SLOT(slotEmptyFolder()));
      folderMenu->setItemEnabled( itemId, fti->folder()->count() > 0 );
    }
    if ( !fti->folder()->isSystemFolder() )
      folderMenu->insertItem(SmallIcon("editdelete"),
                             i18n("&Delete Folder"), mMainWidget,
                             SLOT(slotRemoveFolder()));

  }
  if (fti->folder() &&
      (fti->folder()->folderType() == KMFolderTypeImap || fti->folder()->folderType() == KMFolderTypeCachedImap ))
  {
    folderMenu->insertSeparator();
    folderMenu->insertItem(SmallIcon("configure"),
        i18n("Subscription..."), mMainWidget,
        SLOT(slotSubscriptionDialog()));

    if (!fti->folder()->noContent())
    {
      folderMenu->insertItem(SmallIcon("reload"), i18n("Check Mail in this Folder"), mMainWidget,
          SLOT(slotRefreshFolder()));
    }
    if ( fti->folder()->folderType() == KMFolderTypeCachedImap ) {
      KMFolderCachedImap * folder = static_cast<KMFolderCachedImap*>( fti->folder() );
      folderMenu->insertItem( SmallIcon("wizard"),
			      i18n("&Troubleshoot IMAP Cache..."),
			      folder, SLOT(slotTroubleshoot()) );
    }
  }

  if (fti->folder() && !fti->folder()->noContent())
  {
    folderMenu->insertSeparator();
    folderMenu->insertItem(SmallIcon("configure"),
        i18n("&Properties"),
        fti,
        SLOT(properties()));
  }


  folderMenu->exec (p, 0);
  triggerUpdate();
  delete folderMenu;
  folderMenu = 0;
}

//-----------------------------------------------------------------------------
// If middle button and folder holds mailing-list, create a message to that list
void KMFolderTree::contentsMouseReleaseEvent(QMouseEvent* me)
{
  QListViewItem *lvi = currentItem(); // Needed for when branches are clicked on
  ButtonState btn = me->button();
  doFolderSelected(lvi);

  // get underlying folder
  KMFolderTreeItem* fti = dynamic_cast<KMFolderTreeItem*>(lvi);

  if (!fti || !fti->folder())
    return;

  // react on middle-button only
  if (btn != Qt::MidButton) return;

  if (!fti->folder()->isMailingList())
    return;

  KMMessage *msg = new KMMessage;
  msg->initHeader(fti->folder()->identity());
  msg->setTo(fti->folder()->mailingListPostAddress());
  KMComposeWin *win = new KMComposeWin(msg, fti->folder()->identity());
  win->show();

  KFolderTree::contentsMouseReleaseEvent(me);
}

//-----------------------------------------------------------------------------
// Create a subfolder.
// Requires creating the appropriate subdirectory and show a dialog
void KMFolderTree::addChildFolder()
{
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(currentItem());
  if (!fti)
    return;
  KMFolder *aFolder = fti->folder();
  if (fti->folder())
    if (!fti->folder()->createChildFolder())
      return;

  KMFolderDir *dir = &(kmkernel->folderMgr()->dir());
  if (fti->folder())
    dir = fti->folder()->child();

  KMFolderDialog *d =
    new KMFolderDialog(0, dir, mMainWidget, i18n("Create Subfolder") );

  if (d->exec()) /* fti may be deleted here */ {
    QListViewItem *qlvi = indexOfFolder( aFolder );
    if (qlvi) {
      qlvi->setOpen(TRUE);
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

  return config->readBoolEntry("isOpen", true);
}

//-----------------------------------------------------------------------------
// Saves open/closed state of a folder directory into the config file
void KMFolderTree::writeIsListViewItemOpen(KMFolderTreeItem *fti)
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
      return;
  } else {
    return;
  }
  KConfigGroupSaver saver(config, name);
  config->writeEntry("isOpen", fti->isOpen());
}


//-----------------------------------------------------------------------------
void KMFolderTree::cleanupConfigFile()
{
  KConfig* config = KMKernel::config();
  QStringList existingFolders;
  QListViewItemIterator fldIt(this);
  QMap<QString,bool> folderMap;
  KMFolderTreeItem *fti;
  for (QListViewItemIterator fldIt(this); fldIt.current(); fldIt++)
  {
    fti = static_cast<KMFolderTreeItem*>(fldIt.current());
    if (fti && fti->folder())
      folderMap.insert(fti->folder()->idString(), fti->isExpandable());
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
      config->deleteGroup(*grpIt, TRUE);
      kdDebug(5006) << "Deleting information about folder " << name << endl;
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
        dropItem->setOpen( TRUE );
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
  if ( !acceptDrag(e) ) {
    e->ignore();
  }
}

static const int autoscroll_margin = 16;
static const int initialScrollTime = 30;
static const int initialScrollAccel = 5;

//-----------------------------------------------------------------------------
void KMFolderTree::startAutoScroll()
{
    if ( !autoscroll_timer.isActive() ) {
        autoscroll_time = initialScrollTime;
        autoscroll_accel = initialScrollAccel;
        autoscroll_timer.start( autoscroll_time );
    }
}

//-----------------------------------------------------------------------------
void KMFolderTree::stopAutoScroll()
{
    autoscroll_timer.stop();
}

//-----------------------------------------------------------------------------
void KMFolderTree::autoScroll()
{
    QPoint p = viewport()->mapFromGlobal( QCursor::pos() );

    if ( autoscroll_accel-- <= 0 && autoscroll_time ) {
        autoscroll_accel = initialScrollAccel;
        autoscroll_time--;
        autoscroll_timer.start( autoscroll_time );
    }
    int l = QMAX(1,(initialScrollTime-autoscroll_time));

    int dx=0,dy=0;
    if ( p.y() < autoscroll_margin ) {
        dy = -l;
    } else if ( p.y() > visibleHeight()-autoscroll_margin ) {
        dy = +l;
    }
    if ( p.x() < autoscroll_margin ) {
        dx = -l;
    } else if ( p.x() > visibleWidth()-autoscroll_margin ) {
        dx = +l;
    }
    if ( dx || dy ) {
        scrollBy(dx,dy);
    } else {
        stopAutoScroll();
    }
}

//-----------------------------------------------------------------------------
void KMFolderTree::contentsDragMoveEvent( QDragMoveEvent *e )
{
    QPoint vp = contentsToViewport(e->pos());
    QRect inside_margin((contentsX() > 0) ? autoscroll_margin : 0,
                        (contentsY() > 0) ? autoscroll_margin : 0,
      visibleWidth() - ((contentsX() + visibleWidth() < contentsWidth())
        ? autoscroll_margin*2 : 0),
      visibleHeight() - ((contentsY() + visibleHeight() < contentsHeight())
        ? autoscroll_margin*2 : 0));
    QListViewItem *i = itemAt( vp );
    if ( i ) {
        if ( acceptDrag(e) ) {
            setCurrentItem( i );
        }
        if ( !inside_margin.contains(vp) ) {
            startAutoScroll();
            e->accept(QRect(0,0,0,0)); // Keep sending move events
            autoopen_timer.stop();
        } else {
            if ( acceptDrag(e) ) {
                e->accept();
            }
            else {
                e->ignore();
            }
            if ( i != dropItem ) {
                autoopen_timer.stop();
                dropItem = i;
                autoopen_timer.start( autoopenTime );
            }
        }
        if ( acceptDrag(e) ) {
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
        }
    } else {
        e->ignore();
        autoopen_timer.stop();
        dropItem = 0;
    }
}

//-----------------------------------------------------------------------------
void KMFolderTree::contentsDragLeaveEvent( QDragLeaveEvent * )
{
    if (!oldCurrent) return;

    autoopen_timer.stop();
    stopAutoScroll();
    dropItem = 0;

    setCurrentItem( oldCurrent );
    if (oldSelected)
      setSelected( oldSelected, TRUE );
}

//-----------------------------------------------------------------------------
void KMFolderTree::contentsDropEvent( QDropEvent *e )
{
    autoopen_timer.stop();
    stopAutoScroll();

    QListViewItem *item = itemAt( contentsToViewport(e->pos()) );
    if (! item ) e->ignore();

    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);
    if (fti && (fti != oldSelected) && (fti->folder()))
    {
      int root_x, root_y, win_x, win_y;
      uint keybstate;
      Window rootw, childw;
      XQueryPointer( qt_xdisplay(), qt_xrootwin(), &rootw, &childw,
          &root_x, &root_y, &win_x, &win_y, &keybstate );

      if ( keybstate & ControlMask ) {
        emit folderDropCopy(fti->folder());
      } else if ( keybstate & ShiftMask ) {
        emit folderDrop(fti->folder());
      } else {
        if ( mShowPopupAfterDnD ) {
          KPopupMenu *menu = new KPopupMenu( this );
          menu->insertItem( i18n("&Move Here"), DRAG_MOVE, 0 );
          menu->insertItem( SmallIcon("editcopy"), i18n("&Copy Here"), DRAG_COPY, 1 );
          menu->insertSeparator();
          menu->insertItem( SmallIcon("cancel"), i18n("C&ancel"), DRAG_CANCEL, 3 );
          int id = menu->exec( QCursor::pos(), 0 );
          switch(id) {
            case DRAG_COPY:
              emit folderDropCopy(fti->folder());
              break;
            case DRAG_MOVE:
              emit folderDrop(fti->folder());
              break;
            case DRAG_CANCEL:
              //just chill, doing nothing
              break;
            default:
              kdDebug(5006) << "Unknown dnd-type!" << endl;
          }
        }
        else
          emit folderDrop(fti->folder());
      }
      e->accept();
    } else
      e->ignore();

    dropItem = 0;

    clearSelection();
    setCurrentItem( oldCurrent );
    if ( oldSelected )
      setSelected( oldSelected, TRUE );
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotFolderExpanded( QListViewItem * item )
{
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);
  if (fti && fti->folder() && fti->folder()->folderType() == KMFolderTypeImap &&
      !fti->parent())
  {
    KMFolderImap *folder = static_cast<KMFolderImap*>(fti->folder());
    if (folder->getSubfolderState() == KMFolderImap::imapNoInformation)
    {
      // the tree will be reloaded after that
      bool success = folder->listDirectory();
      if (!success) fti->setOpen( false );
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolderTree::slotFolderCollapsed( QListViewItem * item )
{
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);
  if (fti && fti->parent() == firstChild() && fti->folder()
    && fti->folder()->folderType() == KMFolderTypeImap)
  {
    KMFolderImap *folder = static_cast<KMFolderImap*>(fti->folder());
    folder->setSubfolderState(KMFolderImap::imapNoInformation);
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotRenameFolder(QListViewItem *item, int col,
		const QString &text)
{

  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);

  if (fti && fti->folder() && col != 0 && !currentFolder()->child())
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
void KMFolderTree::slotUpdateCounts(KMFolderImap * folder, bool success)
{
  if (success) slotUpdateCounts(static_cast<KMFolder*>(folder));
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotUpdateCounts(KMFolder * folder)
{
  QListViewItem * current;

  if (folder) current = indexOfFolder(folder);
  else current = currentItem();

  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(current);
  // sanity check
  if (!fti) return;
  if (!fti->folder()) fti->setTotalCount(-1);

  // get the unread count
  int count = 0;
  if (folder->noContent()) // always empty
    count = -1;
  else if (fti->folder()->countUnread() == 0)
    count = 0;
  else
    count = fti->folder()->countUnread();

  // set it
  bool repaint = false;
  if (fti->unreadCount() != count) repaint = true;
  fti->setUnreadCount(count);

  if (isTotalActive())
  {
    count = 0;
    // get the total-count
    if (fti->folder()->isOpened())
      count = fti->folder()->count();
    else
      count = fti->folder()->count(true); // count with caching

    if (fti->folder()->noContent())
      count = -1;

    // set it
    fti->setTotalCount(count);
  }
  if (repaint) {
    // repaint the item and it's parents
    for (QListViewItem *p = fti; p; p = p->parent())
      p->repaint();
  }
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
    // toggle KPopupMenu
    mPopup->setItemChecked( mTotalPop, isTotalActive() );

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
  if (folder && folder->folderType() == KMFolderTypeImap)
  {
    KMAccount* acct = static_cast<KMFolderImap*>(folder)->account();
    kmkernel->acctMgr()->singleCheckMail(acct, true);
  }
}

#include "kmfoldertree.moc"

