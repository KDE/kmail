// kmfoldertree.cpp
#include <qpainter.h>
#include <qcursor.h>

#include <unistd.h>
#include <assert.h>

#include <kapplication.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kdebug.h>

#include "kmfoldermgr.h"
#include "kmfolderimap.h"
#include "kmfoldertree.h"
#include "kmfolderdia.h"
#include "kmcomposewin.h"
#include "kmmainwin.h"
#include "cryptplugwrapperlist.h"
#include <X11/Xlib.h>

QPixmap* KMFolderTree::pixDir = 0;
QPixmap* KMFolderTree::pixNode = 0;
QPixmap* KMFolderTree::pixPlain = 0;
QPixmap* KMFolderTree::pixFld = 0;
QPixmap* KMFolderTree::pixFull = 0;
QPixmap* KMFolderTree::pixIn = 0;
QPixmap* KMFolderTree::pixOut = 0;
QPixmap* KMFolderTree::pixTr = 0;
QPixmap* KMFolderTree::pixSent = 0;

//=============================================================================

KMFolderTreeItem::KMFolderTreeItem( KFolderTree *parent, QString name )
    : KFolderTreeItem( parent, name ), mFolder( 0 )
{
}

//-----------------------------------------------------------------------------
KMFolderTreeItem::KMFolderTreeItem( KFolderTree *parent, QString name,
                    KMFolder* folder )
    : KFolderTreeItem( parent, name ), mFolder( folder )
{
  init();
}

//-----------------------------------------------------------------------------
KMFolderTreeItem::KMFolderTreeItem( KFolderTreeItem *parent, QString name,
                    KMFolder* folder )
    : KFolderTreeItem( parent, name ), mFolder( folder )
{
  init();
}

//-----------------------------------------------------------------------------
void KMFolderTreeItem::init()
{
  if (!mFolder) return;

  if (mFolder->protocol() == "imap")
    setProtocol(Imap);
  else if (mFolder->protocol() == "mbox" || mFolder->protocol() == "maildir")
    setProtocol(Local);
  else
    setProtocol(NONE);
}

//-----------------------------------------------------------------------------
int KMFolderTreeItem::countUnreadRecursive()
{
  if (mFolder) 
    return mFolder->countUnreadRecursive();
  else
    return 0;
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


//=============================================================================


KMFolderTree::KMFolderTree( CryptPlugWrapperList * cryptPlugList,
                            QWidget *parent,
                            const char *name )
  : KFolderTree( parent, name ), mCryptPlugList( cryptPlugList )
{
  static bool pixmapsLoaded = FALSE;
  oldSelected = 0;
  oldCurrent = 0;
  mLastItem = NULL;

  addAcceptableDropMimetype("x-kmail-drag/message", false);

  addColumn( i18n("Folder"), 160 );

  if (!pixmapsLoaded)
  {
    pixmapsLoaded = true;

    pixDir   = new QPixmap( UserIcon("closed"));
    pixNode  = new QPixmap( UserIcon("green-bullet"));
    pixPlain = new QPixmap( SmallIcon("folder"));
    pixFld   = new QPixmap( SmallIcon("folder"));
    pixFull  = new QPixmap( SmallIcon("folder_open"));
    pixIn    = new QPixmap( UserIcon("kmfldin"));
    pixOut   = new QPixmap( UserIcon("kmfldout"));
    pixSent  = new QPixmap( UserIcon("kmfldsent"));
    pixTr    = new QPixmap( SmallIcon("trashcan_empty"));
  }
  // connect
  connectSignals();

  // popup to switch columns 
  header()->setClickEnabled(true);
  header()->installEventFilter(this);
  mPopup = new KPopupMenu;
  mPopup->insertTitle(i18n("Select columns"));
  mPopup->setCheckable(true);
  mUnreadPop = mPopup->insertItem(i18n("Unread Column"), this, SLOT(slotToggleUnreadColumn()));
  mTotalPop = mPopup->insertItem(i18n("Total Column"), this, SLOT(slotToggleTotalColumn()));

  // add the folders
  reload();

  // read the config
  readConfig();

  // get rid of old-folders
  cleanupConfigFile();
}

//-----------------------------------------------------------------------------
// connects all needed signals to their slots
void KMFolderTree::connectSignals()
{
  connect(&mUpdateTimer, SIGNAL(timeout()),
          this, SLOT(delayedUpdate()));
  
  connect(this, SIGNAL(currentChanged(QListViewItem*)),
	  this, SLOT(doFolderSelected(QListViewItem*)));
  
  connect(kernel->folderMgr(), SIGNAL(changed()),
	  this, SLOT(doFolderListChanged()));
  
  connect(kernel->folderMgr(), SIGNAL(removed(KMFolder*)),
          this, SLOT(slotFolderRemoved(KMFolder*)));
  
  connect(kernel->imapFolderMgr(), SIGNAL(changed()),
          this, SLOT(doFolderListChanged()));
  
  connect(kernel->imapFolderMgr(), SIGNAL(removed(KMFolder*)),
          this, SLOT(slotFolderRemoved(KMFolder*)));

  connect( &autoopen_timer, SIGNAL( timeout() ),
	   this, SLOT( openFolder() ) );

  connect( &autoscroll_timer, SIGNAL( timeout() ),
	   this, SLOT( autoScroll() ) );

  connect( this, SIGNAL( rightButtonPressed( QListViewItem*, const QPoint &, int)),
	   this, SLOT( rightButtonPressed( QListViewItem*, const QPoint &, int)));

  connect( this, SIGNAL( mouseButtonPressed( int, QListViewItem*, const QPoint &, int)),
	   this, SLOT( mouseButtonPressed( int, QListViewItem*, const QPoint &, int)));

  connect( this, SIGNAL( expanded( QListViewItem* ) ),
           this, SLOT( slotFolderExpanded( QListViewItem* ) ) );

  connect( this, SIGNAL( collapsed( QListViewItem* ) ),
           this, SLOT( slotFolderCollapsed( QListViewItem* ) ) );
}

//-----------------------------------------------------------------------------
bool KMFolderTree::event(QEvent *e)
{
  if (e->type() == QEvent::ApplicationPaletteChange)
  {
     readColorConfig();
     return true;
  }
  return KListView::event(e);
}

//-----------------------------------------------------------------------------
void KMFolderTree::createFolderList(QStringList *str,
  QValueList<QGuardedPtr<KMFolder> > *folders)
{
  QListViewItemIterator it( this );
  KMFolderTreeItem *fti, *fti2;
  QString prefix;
  while (it.current())
  {
    fti = static_cast<KMFolderTreeItem*>(it.current());
    if (fti && fti->folder())
    {
      fti2 = static_cast<KMFolderTreeItem*>(fti->parent());
      prefix = "";
      while (fti2 && fti2->parent())
      {
        fti2 = static_cast<KMFolderTreeItem*>(fti2->parent());
        prefix += "  ";
      }
      str->append(prefix + fti->text(0));
      if (fti->folder()->noContent()) folders->append(NULL);
      else folders->append(fti->folder());
    }
    ++it;
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::createImapFolderList(KMFolderImap *aFolder, QStringList *names,
  QStringList *urls, QStringList *mimeTypes)
{
  QListViewItemIterator it( this );
  KMFolderTreeItem *fti;
  while (it.current())
  {
    fti = static_cast<KMFolderTreeItem*>(it.current());
    if (fti && fti->folder())
    {
      KMFolderImap *folder = static_cast<KMFolderImap*>(fti->folder());
      if (folder == aFolder)
      {
        names->append(fti->text(0));
        urls->append(folder->imapPath());
        mimeTypes->append((folder->noContent()) ? "inode/directory" :
          (fti->isExpandable()) ? "message/directory" : "message/digest");
      }
    }
    ++it;
  }
}

//-----------------------------------------------------------------------------
void KMFolderTree::readColorConfig (void)
{
  KConfig* conf = kapp->config();
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
  KConfig* conf = kapp->config();
  QString fntStr;

  // Backing pixmap support
  { //area for config group "Pixmaps"
    KConfigGroupSaver saver(conf, "Pixmaps");
    QString pixmapFile = conf->readEntry("FolderTree","");
    mPaintInfo.pixmapOn = FALSE;
    if (pixmapFile != "") {
      mPaintInfo.pixmapOn = TRUE;
      mPaintInfo.pixmap = QPixmap( pixmapFile );
    }
  }

  readColorConfig();

  // Custom/Ssystem font support
  { //area for config group "Pixmaps"
    KConfigGroupSaver saver(conf, "Fonts");
    if (!conf->readBoolEntry("defaultFonts",TRUE)) {
      QFont folderFont = QFont("helvetica");
      setFont(conf->readFontEntry("folder-font", &folderFont));
    }
    else
      setFont(KGlobalSettings::generalFont());
  }

  // read D'n'D behaviour setting
  KConfigGroup behaviour( kapp->config(), "Behaviour" );
  mShowPopupAfterDnD = behaviour.readBoolEntry( "ShowPopupAfterDnD", true );

  // restore the layout
  restoreLayout(conf, "Geometry");
}

//-----------------------------------------------------------------------------
// Save the configuration file
void KMFolderTree::writeConfig()
{
  // save the current state of the folders
  QListViewItemIterator it( this );
  while (it.current()) {
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
    if (fti)
      writeIsListViewItemOpen(fti);
    ++it;
  }

  // save the current layout
  saveLayout(kapp->config(), "Geometry");
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

  fdir = &kernel->folderMgr()->dir();
  for (folderNode = fdir->first();
    folderNode != NULL;
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
  KMFolderDir* fdir;
  QString str;
  int top = contentsY();

  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(currentItem());
  mLastItem = NULL;
  QListViewItemIterator it( this );
  while (it.current()) {
    fti = static_cast<KMFolderTreeItem*>(it.current());
    if (fti && fti->folder())
      disconnect(fti->folder(),SIGNAL(numUnreadMsgsChanged(KMFolder*)),
		 this,SLOT(refresh(KMFolder*)));
    ++it;
  }
  clear();

  // construct the root of the local folders
  root = new KMFolderTreeItem( this, i18n("Local Folders") );
  root->setOpen( true );

  fdir = &kernel->folderMgr()->dir();
  addDirectory(fdir, root);

  fdir = &kernel->imapFolderMgr()->dir();
  // each imap-account creates it's own root
  addDirectory(fdir, NULL);

  if (openFolders)
  {
    // we open all folders to update the count
    mUpdateIterator = QListViewItemIterator (this);
    QTimer::singleShot( 0, this, SLOT(slotUpdateOneCount()) );
  }
  
  QListViewItemIterator jt( this );

  while (jt.current()) 
  {
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(jt.current());
    if (fti && fti->folder())
    {
      // first disconnect before each connect 
      // to make sure we don't call it several times with each reload
      disconnect(fti->folder(),SIGNAL(numUnreadMsgsChanged(KMFolder*)),
	      this,SLOT(refresh(KMFolder*)));
      connect(fti->folder(),SIGNAL(numUnreadMsgsChanged(KMFolder*)),
	      this,SLOT(refresh(KMFolder*)));
      if (isTotalActive() || isUnreadActive())
      {
        // we want to be noticed of changes to update the unread/total columns
        disconnect(fti->folder(), SIGNAL(numUnreadMsgsChanged(KMFolder*)),
            this,SLOT(slotUpdateCounts(KMFolder*)));
        connect(fti->folder(), SIGNAL(numUnreadMsgsChanged(KMFolder*)),
            this,SLOT(slotUpdateCounts(KMFolder*)));
        if (fti->folder()->protocol() == "imap") 
        {
          // imap-only
          disconnect(fti->folder(), SIGNAL(folderComplete(KMFolderImap*, bool)),
              this,SLOT(slotUpdateCounts(KMFolderImap*, bool)));
          connect(fti->folder(), SIGNAL(folderComplete(KMFolderImap*, bool)),
              this,SLOT(slotUpdateCounts(KMFolderImap*, bool)));
        } else {
          // others-only, imap doesn't need this because of the folderComplete-signal
          disconnect(fti->folder(), SIGNAL(msgAdded(KMFolder*)),
              this,SLOT(slotUpdateCounts(KMFolder*)));
          connect(fti->folder(), SIGNAL(msgAdded(KMFolder*)),
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
    ++jt;
  }
  ensureVisible(0, top + visibleHeight(), 0, 0);
  refresh(0);
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
  KMFolderNode *folderNode;
  KMFolder* folder;
  KMFolderTreeItem* fti;

  for (folderNode = fdir->first();
       folderNode != NULL;
       folderNode = fdir->next())
  {
    if (!folderNode->isDir()) {
      folder = static_cast<KMFolder*>(folderNode);
      if (!parent)
      {
        // create new root-item
        // it needs a folder e.g. to save it's state (open/close) 
        fti = new KMFolderTreeItem( this, folder->label(), folder );
        fti->setExpandable( true );
      } else {
        // create new child
        fti = new KMFolderTreeItem( parent, folder->label(), folder );
      }
      // restore last open-state
      fti->setOpen( readIsListViewItemOpen(fti) );
      
      // assign icons
      if (folder->isSystemFolder())
      {
        if (folder->label() == i18n("inbox"))
          fti->setPixmap( 0, *pixIn );
        else if (folder->label() == i18n("outbox"))
          fti->setPixmap( 0, *pixOut );
        else if (folder->label() == i18n("sent-mail"))
          fti->setPixmap( 0, *pixSent );
        else if (folder->label() == i18n("trash"))
          fti->setPixmap( 0, *pixTr );
        else
          fti->setPixmap( 0, *pixPlain );
      } else fti->setPixmap( 0, (folder->normalIcon()) ? *(folder->normalIcon()) : (*pixPlain) );

      // add child-folders
      if (folder && folder->child())
        addDirectory( folder->child(), fti );      
      // make sure that the folder-settings are correctly read on startup by calling listDirectory
      if (readIsListViewItemOpen(fti) &&
         folder && fti->folder()->protocol() == "imap")
        slotFolderExpanded(fti);
    }
  } // for-end
}

//-----------------------------------------------------------------------------
// Initiate a delayed refresh of the count of unread messages
// Not really need anymore as count is cached in config file. But causes
// a nice blink in the foldertree, that indicates kmail did something
// when the user manually checks for mail and none was found.
void KMFolderTree::refresh(KMFolder* )
{
  mUpdateTimer.changeInterval(200);
}

//-----------------------------------------------------------------------------
// Updates the pixmap and extendedLabel information for items
void KMFolderTree::delayedUpdate()
{
  QString str;
  bool upd = isUpdatesEnabled();
  setUpdatesEnabled(FALSE);

  QListViewItemIterator it( this );
  while (it.current()) {
    bool repaintRequired = false;
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
    if (!fti || !fti->folder()) {
      ++it;
      continue;
    }

    int count = fti->folder()->countUnread();
    if (count != fti->unreadCount()) 
      repaintRequired = true;
    fti->setUnreadCount(count);
    if (count > 0) {
      if (!fti->folder()->isSystemFolder())
        fti->setPixmap( 0, ((fti->folder()->unreadIcon()) ? *(fti->folder()->unreadIcon()) : (*pixFull)) );
    }
    else {
      if (!fti->folder()->isSystemFolder())
        fti->setPixmap( 0, ((fti->folder()->normalIcon()) ? *(fti->folder()->normalIcon()) : (*pixPlain)) );
    }

    if ( fti->folder()->needsRepainting() ) {
      repaintRequired = true;
      fti->folder()->repaintScheduled();
    }

    if (upd && repaintRequired)
      for (QListViewItem *p = fti; p; p = p->parent()) p->repaint();
    ++it;
  }
  setUpdatesEnabled(upd);
  mUpdateTimer.stop();
}

//-----------------------------------------------------------------------------
// Folders have been added/deleted update the tree of folders
void KMFolderTree::doFolderListChanged()
{
  KMFolderTreeItem* fti = static_cast< KMFolderTreeItem* >(currentItem());
  KMFolder* folder = (fti) ? fti->folder() : NULL;
  reload();
  QListViewItem *qlvi = indexOfFolder(folder);
  if (qlvi) {
    setCurrentItem( qlvi );
    setSelected( qlvi, TRUE );
  }
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
    QListViewItem *parent = fti->parent();
    while (parent) {
	parent->setOpen( TRUE );
	parent = parent->parent();
    }
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
  //Set iterator to the current folder
  QListViewItemIterator it( currentItem() );

  while (it.current()) { //loop down the folder list
    ++it;
    //check if folder is one to stop on
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
    if (checkUnreadFolder(fti,confirm)) return;
  }
  //Now if confirm is true we are doing "ReadOn"
  //we have got to the bottom of the folder list
  //so we have to start at the top
  if (confirm) {
    it = firstChild();
    while (it.current()) { //loop down the folder list
      ++it;
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
    if ( confirm ) {
      // If confirm is true then we are doing "ReadOn" and we want to miss
      // Out the trash folder
      if (fti->folder()->label() == i18n("trash"))
	return false;
      else {
      //  warn user that going to next folder - but keep track of
      //  whether he wishes to be notified again in "AskNextFolder"
      //  parameter (kept in the config file for kmail)
	if ( KMessageBox::questionYesNo( this,
	   i18n( "Go to the next unread message in folder %1?" )
                                    .arg( fti->folder()->label() ),
	   i18n( "Go to the Next Unread Message" ),
	   KStdGuiItem::yes(), KStdGuiItem::no(), // defaults
           "AskNextFolder",
           false)
           == KMessageBox::No ) return true;
      }
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
  QListViewItemIterator it( currentItem() );

  while (it.current()) {
    --it;
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
    if (fti && fti->folder() && (fti->folder()->countUnread() > 0)) {
	prepareItem( fti );
	doFolderSelected( fti );
	return;
    }
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
      disconnect(this,SIGNAL(currentChanged(QListViewItem*)),
		 this,SLOT(doFolderSelected(QListViewItem*)));
      setFocus();
      setCurrentItem( fti );
      connect(this,SIGNAL(currentChanged(QListViewItem*)),
	      this,SLOT(doFolderSelected(QListViewItem*)));
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
      disconnect(this,SIGNAL(currentChanged(QListViewItem*)),
		 this,SLOT(doFolderSelected(QListViewItem*)));
      setFocus();
      setCurrentItem( fti );
      connect(this,SIGNAL(currentChanged(QListViewItem*)),
	      this,SLOT(doFolderSelected(QListViewItem*)));
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
  KMFolderTreeItem* fti = static_cast< KMFolderTreeItem* >(qlvi);
  KMFolder* folder = 0;
  if (fti) folder = fti->folder();

  if (mLastItem && mLastItem != fti && mLastItem->folder()
     && (mLastItem->folder()->protocol() == "imap"))
  {
    KMFolderImap *imapFolder = static_cast<KMFolderImap*>(mLastItem->folder());
    imapFolder->setSelected(FALSE);
    KMAcctImap *act = imapFolder->account();
    if (act)
    {
      act->killAllJobs();
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
    if (folder->protocol() == "imap")
    {
      KMFolderImap *imap_folder = static_cast<KMFolderImap*>(fti->folder());
      imap_folder->setSelected(TRUE);
      if (imap_folder->getContentState() != KMFolderImap::imapInProgress)
        imap_folder->getFolder();
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
  KConfig* conf = kapp->config();

  KConfigGroupSaver saver(conf, "Geometry");
  conf->writeEntry(name(), size().width());

  KListView::resizeEvent(e);
}

//-----------------------------------------------------------------------------
QListViewItem* KMFolderTree::indexOfFolder(const KMFolder* folder)
{
  QListViewItemIterator it( this );
  while (it.current()) {
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
    if (fti && fti->folder() == folder) {
      return it.current();
    }
    ++it;
  }
  return 0;
}

//-----------------------------------------------------------------------------
// Handle RMB press, show pop up menu
void KMFolderTree::rightButtonPressed(QListViewItem *lvi, const QPoint &p, int)
{
  if (!lvi)
    return;
  setCurrentItem( lvi );
  setSelected( lvi, TRUE );

  if (!topLevelWidget()) return; // safe bet

  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(lvi);

  if (!fti )
    return;

  KPopupMenu *folderMenu = new KPopupMenu;
  if (fti->folder()) folderMenu->insertTitle(fti->folder()->label());

  if ((!fti->folder() || (fti->folder()->noContent()
    && !fti->parent())))
  {
    QString createChild = i18n("&Create Child Folder...");
    if (!fti->folder()) createChild = i18n("&Create Folder...");

    folderMenu->insertItem(SmallIcon("folder_new"),
                           createChild, this,
                           SLOT(addChildFolder()));
    if (!fti->folder()) {
      folderMenu->insertItem(i18n("Compact All &Folders"),
                     kernel->folderMgr(), SLOT(compactAll()));
      folderMenu->insertItem(i18n("Expire All Folders"),
			     kernel->folderMgr(), SLOT(expireAll()));
    } else if (fti->folder()->protocol() == "imap") {
      folderMenu->insertItem(SmallIcon("mail_get"), i18n("Check &Mail"),
        static_cast<KMFolderImap*>(fti->folder())->account(),
        SLOT(processNewMail()));
    }
  } else {
    if ((fti->folder() == kernel->outboxFolder()) && (fti->folder()->count()) )
        folderMenu->insertItem(SmallIcon("mail_send"),
                               i18n("Send Queued"), topLevelWidget(),
                               SLOT(slotSendQueued()));
    if (!fti->folder()->isSystemFolder() || fti->folder()->protocol() == "imap")
    {
      folderMenu->insertItem(SmallIcon("folder_new"),
                             i18n("&Create Child Folder..."), this,
                             SLOT(addChildFolder()));
    }

    // Want to be able to display properties for ALL folders,
    // so we can edit expiry properties.
    // -- smp.
    if (!fti->folder()->noContent())
    {
      folderMenu->insertItem(i18n("&Properties..."), topLevelWidget(),
                         SLOT(slotModifyFolder()));

      if (fti->folder()->protocol() != "imap" && fti->folder()->isAutoExpire())
        folderMenu->insertItem(i18n("E&xpire"), topLevelWidget(),
                               SLOT(slotExpireFolder()));

      folderMenu->insertItem(i18n("C&ompact"), topLevelWidget(),
                             SLOT(slotCompactFolder()));
      folderMenu->insertSeparator();
      if (fti->folder()->countUnread() > 0)
        folderMenu->insertItem(SmallIcon("goto"),
                               i18n("&Mark All Messages as Read"), topLevelWidget(),
                               SLOT(slotMarkAllAsRead()));
      folderMenu->insertItem(i18n("&Empty"), topLevelWidget(),
                             SLOT(slotEmptyFolder()));
    }
    if ( !fti->folder()->isSystemFolder() )
        folderMenu->insertItem(i18n("&Remove"), topLevelWidget(),
                               SLOT(slotRemoveFolder()));

    if (!fti->folder()->noContent() && fti->folder()->isMailingList())
    {
      folderMenu->insertSeparator();
      folderMenu->insertItem(i18n("Post to &Mailing-List"),
                             topLevelWidget(), SLOT(slotPostToML()));
    }
  }

  folderMenu->exec (p, 0);
  triggerUpdate();
  delete folderMenu;
}

//-----------------------------------------------------------------------------
// If middle button and folder holds mailing-list, create a message to that list
void KMFolderTree::mouseButtonPressed(int btn, QListViewItem *lvi, const QPoint &, int)
{
  // react on middle-button only
  if (btn != Qt::MidButton) return;

  // get underlying folder
  KMFolderTreeItem* fti = dynamic_cast<KMFolderTreeItem*>(lvi);

  if (!fti || !fti->folder())
    return;
  if (!fti->folder()->isMailingList())
    return;

  KMMessage *msg = new KMMessage;
  msg->initHeader(fti->folder()->identity());
  msg->setTo(fti->folder()->mailingListPostAddress());
  KMComposeWin *win = new KMComposeWin(mCryptPlugList, msg, fti->folder()->identity());
  win->show();

}

//-----------------------------------------------------------------------------
// Create a child folder.
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

  KMFolderDir *dir = &(kernel->folderMgr()->dir());
  if (fti->folder())
    dir = fti->folder()->child();

  KMFolderDialog *d =
    new KMFolderDialog(0, dir, topLevelWidget(), i18n("Create Child Folder") );

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
     reload();
  }
}

//-----------------------------------------------------------------------------
// Returns whether a folder directory should be open as specified in the
// config file. The root is always open
bool KMFolderTree::readIsListViewItemOpen(KMFolderTreeItem *fti)
{
  KConfig* config = kapp->config();
  KMFolder *folder = fti->folder();
  if (!folder)
    return TRUE;
  KConfigGroupSaver saver(config, "Folder-" + folder->idString());

  return config->readBoolEntry("isOpen", false);
}

//-----------------------------------------------------------------------------
// Saves open/closed state of a folder directory into the config file
void KMFolderTree::writeIsListViewItemOpen(KMFolderTreeItem *fti)
{
  KConfig* config = kapp->config();
  KMFolder *folder = fti->folder();
  if (!folder)
    return;
  KConfigGroupSaver saver(config, "Folder-" + folder->idString());
  config->writeEntry("isOpen", fti->isOpen());
}


//-----------------------------------------------------------------------------
void KMFolderTree::cleanupConfigFile()
{
  KConfig* config = kapp->config();
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
  QListViewItemIterator it( this );
  while (it.current()) {
    if (it.current()->isSelected())
      oldSelected = it.current();
    ++it;
  }
  setFocus();

  QListViewItem *i = itemAt( contentsToViewport(e->pos()) );
  if ( i ) {
    dropItem = i;
    autoopen_timer.start( autoopenTime );
  }
  disconnect(this, SIGNAL(currentChanged(QListViewItem*)),
	     this, SLOT(doFolderSelected(QListViewItem*)));

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
        dropItem = 0L;
    }
}

//-----------------------------------------------------------------------------
void KMFolderTree::contentsDragLeaveEvent( QDragLeaveEvent * )
{
    if (!oldCurrent) return;

    autoopen_timer.stop();
    stopAutoScroll();
    dropItem = 0L;

    setCurrentItem( oldCurrent );
    if (oldSelected)
      setSelected( oldSelected, TRUE );
    connect(this, SIGNAL(currentChanged(QListViewItem*)),
	    this, SLOT(doFolderSelected(QListViewItem*)));
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
          menu->insertItem( i18n("Move"), DRAG_MOVE, 0 );
          menu->insertItem( i18n("Copy"), DRAG_COPY, 1 );
          menu->insertSeparator();
          menu->insertItem( i18n("Cancel"), DRAG_CANCEL, 3 );
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

    dropItem = 0L;

    clearSelection();
    setCurrentItem( oldCurrent );
    if ( oldSelected )
      setSelected( oldSelected, TRUE );
    connect(this, SIGNAL(currentChanged(QListViewItem*)),
	    this, SLOT(doFolderSelected(QListViewItem*)));
}

//-----------------------------------------------------------------------------
void KMFolderTree::slotFolderExpanded( QListViewItem * item )
{
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);
  if (fti && fti->folder() && fti->folder()->protocol() == "imap")
  {
    KMFolderImap *folder = static_cast<KMFolderImap*>(fti->folder());
    if (folder->getSubfolderState() == KMFolderImap::imapNoInformation)
    {
      writeIsListViewItemOpen( fti );
      // the tree will be reloaded after that
      folder->listDirectory( fti );
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolderTree::slotFolderCollapsed( QListViewItem * item )
{
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);
  if (fti && fti->parent() == firstChild() && fti->folder()
    && fti->folder()->protocol() == "imap")
  {
    KMFolderImap *folder = static_cast<KMFolderImap*>(fti->folder());
    folder->setSubfolderState(KMFolderImap::imapNoInformation);
  }
}


//-----------------------------------------------------------------------------
void KMFolderTree::slotAccountDeleted(KMFolderImap *aFolder)
{
  writeConfig();
  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(currentItem());
  if (fti && fti->folder() && fti->folder() == aFolder)
    doFolderSelected(0);
  QListViewItem *lvi = firstChild();
  if (lvi) lvi = lvi->firstChild();
  while (lvi)
  {
    fti = static_cast<KMFolderTreeItem*>(lvi);
    if (fti && fti->folder())
    {
      KMFolderImap *folder = static_cast<KMFolderImap*>(fti->folder());
      if (folder == aFolder)
      {
        delete fti;
        break;
      }
    }
    lvi = lvi->nextSibling();
  }
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
    for (QListViewItem *p = fti; p; p = p->parent()) p->repaint();
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
    // toggle KPopupMenu and KToggleAction
    mPopup->setItemChecked( mUnreadPop, isUnreadActive() );
    if ( parentWidget()->parentWidget()->isA("KMMainWin") )
      static_cast<KMMainWin*>(parentWidget()->parentWidget())
        ->unreadColumnToggle->setChecked( isUnreadActive() );

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
    // toggle KPopupMenu and KToggleAction
    mPopup->setItemChecked( mTotalPop, isTotalActive() );
    if ( parentWidget()->parentWidget()->isA("KMMainWin") )
      static_cast<KMMainWin*>(parentWidget()->parentWidget())
        ->totalColumnToggle->setChecked( isTotalActive() );

  } else kdDebug(5006) << "unknown column:" << column << endl;
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

#include "kmfoldertree.moc"

