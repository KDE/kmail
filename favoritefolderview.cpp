/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "favoritefolderview.h"

#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "folderselectiondialog.h"
#include "kmmainwidget.h"
#include "kmailicalifaceimpl.h"
#include "folderstorage.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmacctcachedimap.h"
#include "korghelper.h"

#include <libkdepim/maillistdrag.h>
#include <libkdepim/kaddrbookexternal.h>

#include <kcolorscheme.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmenu.h>
#include <kio/global.h>

//#include <QHelpEvent>
#include <QtCore/QTimer>
#include <QtGui/QToolTip>

#include <cassert>

using namespace KMail;

FavoriteFolderViewItem::FavoriteFolderViewItem(FavoriteFolderView * parent, const QString & name, KMFolder * folder)
  : KMFolderTreeItem( parent, name, folder ),
  mOldName( folder->label() )
{
  // same stuff as in KMFolderTreeItem again, this time even with virtual methods working
  init();
  connect( folder, SIGNAL(nameChanged()), SLOT(nameChanged()) );
  connect( folder, SIGNAL(iconsChanged()), SLOT(slotIconsChanged()) );

  connect( folder, SIGNAL(msgAdded(KMFolder*,quint32)), SLOT(updateCount()) );
  connect( folder, SIGNAL(numUnreadMsgsChanged(KMFolder*)), SLOT(updateCount()) );
  connect( folder, SIGNAL(msgRemoved(KMFolder*)), SLOT(updateCount()) );
  connect( folder, SIGNAL(folderSizeChanged( KMFolder* )), SLOT(updateCount()) );

  updateCount();
  if ( unreadCount() > 0 )
    setPixmap( 0, unreadIcon( iconSize() ) );
  else
    setPixmap( 0, normalIcon( iconSize() ) );
}

void FavoriteFolderViewItem::updateCount()
{
  // FIXME: share with KMFolderTree
  if ( !folder() ) {
    setTotalCount( -1 );
    return;
  }

  // get the unread count
  int count = 0;
  if (folder() && folder()->noContent()) // always empty
    count = -1;
  else {
    count = folder()->countUnread();
  }
   bool repaint = false;
  if ( unreadCount() != count ) {
     adjustUnreadCount( count );
     repaint = true;
  }

  // get the total-count
  if ( folder()->noContent() )
    count = -1;
  else {
    // get the cached count if the folder is not open
    count = folder()->count( !folder()->isOpened() );
  }
  if ( count != totalCount() ) {
    setTotalCount(count);
    repaint = true;
  }

  if ( !folder()->noContent() ) {
    qint64 size = folder()->storage()->folderSize();
    if ( size != folderSize() ) {
      setFolderSize( size );
      repaint = true;
    }
  }

  if ( folderIsCloseToQuota() != folder()->storage()->isCloseToQuota() )
    setFolderIsCloseToQuota( folder()->storage()->isCloseToQuota() );

  if (repaint) {
    this->repaint();
  }
}

void FavoriteFolderViewItem::nameChanged()
{
  QString txt = text( 0 );
  txt.replace( mOldName, folder()->label() );
  setText( 0, txt );
  mOldName = folder()->label();
}


QList<FavoriteFolderView*> FavoriteFolderView::mInstances;

FavoriteFolderView::FavoriteFolderView( KMMainWidget *mainWidget, QWidget * parent) :
    FolderTreeBase( mainWidget, parent ),
    mReadingConfig( false )
{
  assert( mainWidget );
  addColumn( i18n("Favorite Folders") );
  setResizeMode( LastColumn );
  header()->setClickEnabled( false );
  setDragEnabled( true );
  setAcceptDrops( true );
  setRootIsDecorated( false );
  setSelectionMode( Q3ListView::Single );
  setSorting( -1 );
  setShowSortIndicator( false );

  connect( this, SIGNAL(selectionChanged()), SLOT(selectionChanged()) );
  connect( this, SIGNAL(clicked(Q3ListViewItem*)), SLOT(itemClicked(Q3ListViewItem*)) );
  connect( this, SIGNAL(dropped(QDropEvent*,Q3ListViewItem*)), SLOT(dropped(QDropEvent*,Q3ListViewItem*)) );
  connect( this, SIGNAL(contextMenuRequested(Q3ListViewItem*, const QPoint &, int)),
           SLOT(contextMenu(Q3ListViewItem*,const QPoint&)) );
  connect( this, SIGNAL(moved()), SLOT(notifyInstancesOnChange()) );

  connect( kmkernel->folderMgr(), SIGNAL(changed()), SLOT(initializeFavorites()) );
  connect( kmkernel->dimapFolderMgr(), SIGNAL(changed()), SLOT(initializeFavorites()) );
  connect( kmkernel->imapFolderMgr(), SIGNAL(changed()), SLOT(initializeFavorites()) );
  connect( kmkernel->searchFolderMgr(), SIGNAL(changed()), SLOT(initializeFavorites()) );

  connect( kmkernel->folderMgr(), SIGNAL(folderRemoved(KMFolder*)), SLOT(folderRemoved(KMFolder*)) );
  connect( kmkernel->dimapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)), SLOT(folderRemoved(KMFolder*)) );
  connect( kmkernel->imapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)), SLOT(folderRemoved(KMFolder*)) );
  connect( kmkernel->searchFolderMgr(), SIGNAL(folderRemoved(KMFolder*)), SLOT(folderRemoved(KMFolder*)) );

  QFont f = font();
  f.setItalic( true );
  setFont( f );

  mInstances.append( this );
}

FavoriteFolderView::~FavoriteFolderView()
{
  mInstances.removeAll( this );
}

void FavoriteFolderView::readConfig()
{
  mReadingConfig = true;
  clear();
  QList<int> folderIds = GlobalSettings::self()->favoriteFolderIds();
  QStringList folderNames = GlobalSettings::self()->favoriteFolderNames();
  Q3ListViewItem *afterItem = 0;
  for ( int i = 0; i < folderIds.count(); ++i ) {
    KMFolder *folder = kmkernel->folderMgr()->findById( folderIds[i] );
    if ( !folder )
      folder = kmkernel->imapFolderMgr()->findById( folderIds[i] );
    if ( !folder )
      folder = kmkernel->dimapFolderMgr()->findById( folderIds[i] );
    if ( !folder )
      folder = kmkernel->searchFolderMgr()->findById( folderIds[i] );
    QString name;
    if ( folderNames.count() > i )
      name = folderNames[i];
    afterItem = addFolder( folder, name, afterItem );
  }
  if ( firstChild() )
    ensureItemVisible( firstChild() );

  // folder tree is not yet populated at this point
  QTimer::singleShot( 0, this, SLOT(initializeFavorites()) );

  readColorConfig();
  mReadingConfig = false;
}

void FavoriteFolderView::writeConfig()
{
  QList<int> folderIds;
  QStringList folderNames;
  for ( Q3ListViewItemIterator it( this ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    folderIds << fti->folder()->id();
    folderNames << fti->text( 0 );
  }
  GlobalSettings::self()->setFavoriteFolderIds( folderIds );
  GlobalSettings::self()->setFavoriteFolderNames( folderNames );
}

bool FavoriteFolderView::acceptDrag(QDropEvent * e) const
{
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  if ( e->provides( "application/x-qlistviewitem" ) &&
       (e->source() == ft->viewport() || e->source() == viewport() ) )
    return true;
  return FolderTreeBase::acceptDrag( e );
}

KMFolderTreeItem* FavoriteFolderView::addFolder(KMFolder * folder, const QString &name, Q3ListViewItem *after)
{
  if ( !folder )
    return 0;
  KMFolderTreeItem *item = new FavoriteFolderViewItem( this, name.isEmpty() ? folder->label() : name, folder );
  if ( after )
    item->moveItem( after );
  else
    item->moveItem( lastItem() );
  ensureItemVisible( item );
  notifyInstancesOnChange();
  return item;
}

void FavoriteFolderView::selectionChanged()
{
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( selectedItem() );
  if ( !fti )
    return;
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  ft->showFolder( fti->folder() );
  handleGroupwareFolder( fti );
}

static void selectKontactPlugin( const QString &plugin )
{
  QDBusInterface *kontact = new QDBusInterface( "org.kde.kontact",
      "/KontactInterface", "org.kde.kontact.KontactInterface",
       QDBusConnection::sessionBus() );
  if ( kontact->isValid() )
    kontact->call( "selectPlugin", plugin );
  delete kontact;
}

void FavoriteFolderView::handleGroupwareFolder( KMFolderTreeItem *fti )
{
  if ( !fti || !fti->folder() || !fti->folder()->storage() )
    return;
  switch ( fti->folder()->storage()->contentsType() ) {
    case KMail::ContentsTypeContact:
      KPIM::KAddrBookExternal::openAddressBook( this );
      break;
    case KMail::ContentsTypeNote:
      selectKontactPlugin( "kontact_knotesplugin" );
      break;
    case KMail::ContentsTypeCalendar:
    case KMail::ContentsTypeTask:
    case KMail::ContentsTypeJournal:
    {
      KMail::KorgHelper::ensureRunning();
      QString plugin;
      switch ( fti->folder()->storage()->contentsType() ) {
        case KMail::ContentsTypeCalendar:
          plugin = QLatin1String( "kontact_korganizerplugin" ); break;
        case KMail::ContentsTypeTask:
          plugin = QLatin1String( "kontact_todoplugin" ); break;
        case KMail::ContentsTypeJournal:
          plugin = QLatin1String( "kontact_journalplugin" ); break;
        default: assert( false );
      }
      selectKontactPlugin( plugin );
      break;
    }
    default: break;
  }
}

void FavoriteFolderView::itemClicked(Q3ListViewItem * item)
{
  if ( !item ) return;
  if ( !item->isSelected() )
    item->setSelected( true );
  item->repaint();
  handleGroupwareFolder( static_cast<KMFolderTreeItem*>( item ) );
}

void FavoriteFolderView::folderTreeSelectionChanged(KMFolder * folder)
{
  blockSignals( true );
  bool found = false;
  for ( Q3ListViewItemIterator it( this ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    if ( fti->folder() == folder && !fti->isSelected() ) {
      fti->setSelected( true );
      setCurrentItem( fti );
      ensureItemVisible( fti );
      fti->repaint();
      found = true;
    } else if ( fti->folder() != folder && fti->isSelected() ) {
      fti->setSelected( false );
      fti->repaint();
    }
  }
  blockSignals( false );
  if ( !found ) {
    clearSelection();
    setSelectionMode( Q3ListView::NoSelection );
    setSelectionMode( Q3ListView::Single );
  }
}

void FavoriteFolderView::folderRemoved(KMFolder * folder)
{
  QList<KMFolderTreeItem *> delItems;
  for ( Q3ListViewItemIterator it( this ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    if ( fti->folder() == folder )
      delItems << fti;
    if ( fti == mContextMenuItem )
      mContextMenuItem = 0;
  }
  for ( int i = 0; i < delItems.count(); ++i )
    delete delItems[i];
}

void FavoriteFolderView::dropped(QDropEvent * e, Q3ListViewItem * after)
{
  Q3ListViewItem* afterItem = after;
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  if ( e->source() == ft->viewport() && e->provides( "application/x-qlistviewitem" ) ) {
    for ( Q3ListViewItemIterator it( ft ); it.current(); ++it ) {
      if ( !it.current()->isSelected() )
        continue;
      KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
      if ( !fti->folder() )
        continue;
      afterItem = addFolder( fti->folder(), prettyName( fti ), afterItem );
    }
    e->accept();
  }
}

void FavoriteFolderView::contextMenu(Q3ListViewItem * item, const QPoint & point)
{
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( item );
  mContextMenuItem = fti;
  KMenu contextMenu;
  if ( fti && fti->folder() ) {
    contextMenu.addAction( SmallIcon( "edit-delete" ), i18n( "Remove From Favorites" ),
                           this, SLOT( removeFolder() ) );
    contextMenu.addAction( SmallIcon( "edit-rename" ), i18n( "Rename Favorite" ),
                           this, SLOT( renameFolder() ) );
    contextMenu.addSeparator();

    contextMenu.addAction( mainWidget()->action( "mark_all_as_read" ) );
    if ( fti->folder()->folderType() == KMFolderTypeImap
         || fti->folder()->folderType() == KMFolderTypeCachedImap ) {
      contextMenu.addAction( mainWidget()->action( "refresh_folder" ) );
    }

    if ( fti->folder()->isMailingListEnabled() )
      contextMenu.addAction( mainWidget()->action( "post_message" ) );

    contextMenu.addAction( SmallIcon( "configure-shortcuts" ), i18n( "&Assign Shortcut..." ),
                           fti, SLOT( assignShortcut() ) );
    contextMenu.addAction( i18n( "Expire..." ), fti, SLOT(slotShowExpiryProperties()) );
    contextMenu.addAction( mainWidget()->action( "modify" ) );
  } else {
    contextMenu.addAction( SmallIcon( "bookmark-new" ), i18n( "Add Favorite Folder..." ),
                           this, SLOT( addFolder() ) );
  }
  contextMenu.exec( point );
}

//Port the code below to QTreeWidgetItem::setToolTip() once the folder tree stuff
//is based on QTreeWidget. --tmcguire
#ifdef __GNUC__
#warning Port me!
#endif
bool FavoriteFolderView::event( QEvent * /*e*/ )
{
  return true;/*
  if ( e->type() == QEvent::ToolTip ) {
    QHelpEvent *he = static_cast<QHelpEvent *>( e );

    FavoriteFolderViewItem *item = static_cast<FavoriteFolderViewItem*>( itemAt( he->pos() ) );
    if  ( !item )
      return FolderTreeBase::event( e );
    const QRect iRect = itemRect( item );
    if ( !iRect.isValid() )
      return FolderTreeBase::event( e );
    const QRect headerRect = header()->sectionRect( 0 );
    if ( !headerRect.isValid() )
      return FolderTreeBase::event( e );

    QString tipText = i18n("<qt><b>%1</b><br>Total: %2<br>Unread: %3<br>Size: %4",
        item->folder()->prettyUrl().replace( " ", "&nbsp;" ),
        item->totalCount() < 0 ? "-" : QString::number( item->totalCount() ),
        item->unreadCount() < 0 ? "-" : QString::number( item->unreadCount() ),
        KIO::convertSize( item->folderSize() ) );
    QRect validIn( headerRect.left(), iRect.top(), headerRect.width(), iRect.height() );
    QToolTip::showText( he->pos(), tipText, this, validIn );
    return true;
  } else {
    return FolderTreeBase::event( e );
  }*/
}

void FavoriteFolderView::removeFolder()
{
  delete mContextMenuItem;
  mContextMenuItem = 0;
  notifyInstancesOnChange();
}

void FavoriteFolderView::initializeFavorites()
{
  QList<int> seenInboxes = GlobalSettings::self()->favoriteFolderViewSeenInboxes();
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  for ( Q3ListViewItemIterator it( ft ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    if ( fti->type() == KFolderTreeItem::Inbox && fti->folder() && !seenInboxes.contains( fti->folder()->id() ) ) {
      seenInboxes.append( fti->folder()->id() );
      if ( fti->folder() == kmkernel->inboxFolder() && hideLocalInbox() )
        continue;
      if ( kmkernel->iCalIface().hideResourceFolder( fti->folder() ) )
        continue;
      addFolder( fti->folder(), prettyName( fti ) );
    }
  }
  GlobalSettings::self()->setFavoriteFolderViewSeenInboxes( seenInboxes );
}

void FavoriteFolderView::renameFolder()
{
  if ( !mContextMenuItem )
    return;
  bool ok;
  QString name = KInputDialog::getText( i18n("Rename Favorite"), i18n("Name"), mContextMenuItem->text( 0 ), &ok, this );
  if ( !ok )
    return;
  mContextMenuItem->setText( 0, name );
  notifyInstancesOnChange();
}

QString FavoriteFolderView::prettyName(KMFolderTreeItem * fti)
{
  assert( fti );
  assert( fti->folder() );
  QString name = fti->folder()->label();
  Q3ListViewItem *accountFti = fti;
  while ( accountFti->parent() )
    accountFti = accountFti->parent();
  if ( fti->type() == KFolderTreeItem::Inbox ) {
    if ( fti->protocol() == KFolderTreeItem::Local || fti->protocol() == KFolderTreeItem::NONE ) {
      name = i18n( "Local Inbox" );
    } else {
      name = i18n( "Inbox of %1", accountFti->text( 0 ) );
    }
  } else {
    if ( fti->protocol() != KFolderTreeItem::Local && fti->protocol() != KFolderTreeItem::NONE ) {
      name = i18n( "%1 on %2", fti->text( 0 ), accountFti->text( 0 ) );
    } else {
      name = i18n( "%1 (local)", fti->text( 0 ) );
    }
  }
  return name;
}

void FavoriteFolderView::contentsDragEnterEvent(QDragEnterEvent * e)
{
  if ( e->provides( "application/x-qlistviewitem" ) ) {
    setDropVisualizer( true );
    setDropHighlighter( false );
  } else if ( e->mimeData()->hasFormat( KPIM::MailList::mimeDataType() ) ) {
    setDropVisualizer( false );
    setDropHighlighter( true );
  } else {
    setDropVisualizer( false );
    setDropHighlighter( false );
  }
  e->accept();
}

void FavoriteFolderView::readColorConfig()
{
  FolderTreeBase::readColorConfig();
  // Custom/System color support
  KConfigGroup cg = KMKernel::config()->group( "Reader" );
  QColor c = KColorScheme( QPalette::Normal, KColorScheme::View ).background(
                           KColorScheme::AlternateBackground ).color();
  if ( !cg.readEntry( "defaultColors", true ) )
    mPaintInfo.colBack = cg.readEntry( "AltBackgroundColor", c );
  else
    mPaintInfo.colBack = c;

  QPalette newPal = palette();
  newPal.setColor( QColorGroup::Base, mPaintInfo.colBack );
  setPalette( newPal );
}

void FavoriteFolderView::addFolder()
{
  FolderSelectionDialog dlg( mainWidget(), i18n("Add Favorite Folder"), false );
  if ( dlg.exec() != QDialog::Accepted )
    return;
  KMFolder *folder = dlg.folder();
  if ( !folder )
    return;
  KMFolderTreeItem *fti = findFolderTreeItem( folder );
  addFolder( folder, fti ? prettyName( fti ) : folder->label() );
}

KMFolderTreeItem * FavoriteFolderView::findFolderTreeItem(KMFolder * folder) const
{
  assert( folder );
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  for ( Q3ListViewItemIterator it( ft ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    if ( fti->folder() == folder )
      return fti;
  }
  return 0;
}

void FavoriteFolderView::checkMail()
{
  bool found = false;
  for ( Q3ListViewItemIterator it( this ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    if ( fti->folder()->folderType() == KMFolderTypeImap || fti->folder()->folderType() == KMFolderTypeCachedImap ) {
      if ( !found )
        if ( !kmkernel->askToGoOnline() )
          break;
      found = true;
      if ( fti->folder()->folderType() == KMFolderTypeImap ) {
        KMFolderImap *imap = static_cast<KMFolderImap*>( fti->folder()->storage() );
        imap->getAndCheckFolder();
      } else if ( fti->folder()->folderType() == KMFolderTypeCachedImap ) {
        KMFolderCachedImap* f = static_cast<KMFolderCachedImap*>( fti->folder()->storage() );
        f->account()->processNewMailSingleFolder( fti->folder() );
      }
    }
  }
}

void FavoriteFolderView::notifyInstancesOnChange()
{
  if ( mReadingConfig )
    return;
  writeConfig();
  for ( QList<FavoriteFolderView*>::ConstIterator it = mInstances.begin(); it != mInstances.end(); ++it ) {
    if ( (*it) == this || (*it)->mReadingConfig )
      continue;
    (*it)->readConfig();
  }
}

#include "favoritefolderview.moc"
