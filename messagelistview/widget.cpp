/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "messagelistview/widget.h"
#include "messagelistview/pane.h"
#include "messagelistview/storagemodel.h"

#include "messagelistview/core/view.h"
#include "messagelistview/core/messageitem.h"

#include "kmmessage.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "kmmessagetag.h"
#include "kmkernel.h"
#include "mainfolderview.h"
#include "messagecopyhelper.h"
#include "globalsettings.h"
#include "textsource.h"
#include "messagetree.h"

#include <QDrag>
#include <QPixmap>
#include <QTimer>
#include <QAction>
#include <QActionGroup>

#include <KAcceleratorManager>
#include <KActionMenu>
#include <KApplication>
#include <KConfigGroup>
#include <KIconEffect>
#include <KIconLoader>
#include <KMenu>

#include <libkdepim/maillistdrag.h> // KPIM::MailList

namespace KMail
{

namespace MessageListView
{

Widget::Widget( KMMainWidget *mainWidget, Pane *pPane )
  : Core::Widget( pPane ), mPane( pPane ), mMainWidget( mainWidget )
{
  mLastSelectedMessage = -1;

  // Behavior
  {
    // This is an ugly hack. I'm even not 100% sure that it's correct...
    // Let's hope for the best :D
    KConfigGroup config( KMKernel::config(), "Geometry" );
    mReleaseMessageAfterCurrentChange = config.readEntry( "readerWindowMode", "below" ) != "hide";
  }

  mIconAnimationTimer = new QTimer();

  QObject::connect(
      mIconAnimationTimer, SIGNAL( timeout() ),
      this, SLOT( animateIcon() )
    );
}

Widget::~Widget()
{
  delete mIconAnimationTimer;
}

void Widget::setFolder( KMFolder * fld, const QIcon &icon, Core::PreSelectionMode preSelectionMode )
{
  if ( ( mLastSelectedMessage >= 0 ) && mReleaseMessageAfterCurrentChange && storageModel() )
  {
    static_cast< StorageModel * >( storageModel() )->releaseMessage( mLastSelectedMessage );
    mLastSelectedMessage = -1;
  }

  mFolderIconBase = icon;

  QImage base = mFolderIconBase.pixmap( 16, 16 ).toImage();

  for ( int i = 0; i < BusyAnimationSteps; i++ )
  {
    QImage tmp = SmallIcon( QString( "overlay-busy-clock-%1.png" ).arg( i + 1 ) ).toImage();

    QImage busy = base;
    KIconEffect::overlay( busy, tmp );

    mFolderIconBusy[ i ] = QIcon( QPixmap::fromImage( busy ) );
  }

  mPane->widgetIconChangeRequest( this, mFolderIconBase );

  // Set the storage _after_ setting the icon so it can be overridden
  // from inside setStorageModel().
  setStorageModel( fld ? new StorageModel( fld ) : 0, preSelectionMode );
}

KMFolder * Widget::folder() const
{
  if ( !storageModel() )
    return 0;
  return static_cast< StorageModel * >( storageModel() )->folder();
}

void Widget::viewMessageSelected( Core::MessageItem *msg )
{
  int row = -1;
  if ( msg )
    row = msg->currentModelIndexRow();

  if ( mLastSelectedMessage >= 0 && mReleaseMessageAfterCurrentChange &&
       storageModel() ) {
    bool selectedStaysSame = row != -1 && mLastSelectedMessage == row &&
                             msg && msg->isValid();

    // No need to release the last selected message if it is the new selected
    // message is going to be the same.
    // This fixes a crash in KMMainWidget::slotMsgPopup(), which can activate
    // an already active message in certain circumstances. Releasing that message
    // would make all the message pointers invalid.
    if ( !selectedStaysSame ) {
      static_cast< StorageModel * >( storageModel() )->releaseMessage( mLastSelectedMessage );
    }
  }

  if ( !msg || !storageModel() )
  {
    mLastSelectedMessage = -1;
    emit messageSelected( 0 );
    return;
  }

  if ( !msg->isValid() )
  {
    mLastSelectedMessage = -1;
    emit messageSelected( 0 );
    return;
  }

  Q_ASSERT( row >= 0 );

  KMMessage * message = static_cast< const StorageModel * >( storageModel() )->message( row );

  mLastSelectedMessage = row;

  emit messageSelected( message ); // this MAY be null
}

void Widget::viewMessageActivated( Core::MessageItem *msg )
{
  Q_ASSERT( msg ); // must not be null
  Q_ASSERT( storageModel() );

  if ( !msg->isValid() )
    return;

  int row = msg->currentModelIndexRow();
  Q_ASSERT( row >= 0 );

  // The assert below may fail when quickly opening and closing a non-selected thread.
  // This will actually activate the item without selecting it...
  //Q_ASSERT( mLastSelectedMessage == row );

  if ( mLastSelectedMessage != row )
  {
    // Very ugly. We are activating a non selected message.
    // This is very likely a double click on the plus sign near a thread leader.
    // Dealing with mLastSelectedMessage here would be expensive: it would involve releasing the last selected,
    // emitting signals, handling recursion... ugly.
    // We choose a very simple solution: double clicking on the plus sign near a thread leader does
    // NOT activate the message (i.e open it in a toplevel window) if it isn't previously selected.
    return;
  }

  KMMessage * message = static_cast< const StorageModel * >( storageModel() )->message( row );

  emit messageActivated( message ); // this MAY be null
}

void Widget::viewMessageStatusChangeRequest( Core::MessageItem *msg, const KPIM::MessageStatus &set, const KPIM::MessageStatus &clear )
{
  Q_ASSERT( msg ); // must not be null
  Q_ASSERT( storageModel() );

  if ( !msg->isValid() )
    return;

  int row = msg->currentModelIndexRow();
  Q_ASSERT( row >= 0 );

  KMMsgBase * msgBase = static_cast< const StorageModel * >( storageModel() )->msgBase( row );

  Q_ASSERT( msgBase );

  emit messageStatusChangeRequest( msgBase, set, clear );
}


Core::MessageItem * Widget::currentMessageItem() const
{
  return view()->currentMessageItem();
}

Core::MessageItem * Widget::messageItemFromMessage( KMMessage * msg ) const
{
  return messageItemFromMsgBase( msg );
}


KMMessage * Widget::currentMessage() const
{
  if ( !storageModel() )
    return 0;
  Core::MessageItem * mi = currentMessageItem();
  if ( !mi )
    return 0;
  return static_cast< StorageModel * >( storageModel() )->message( mi );
}

KMMsgBase * Widget::currentMsgBase() const
{
  if ( !storageModel() )
    return 0;
  Core::MessageItem * mi = currentMessageItem();
  if ( !mi )
    return 0;
  return static_cast< StorageModel * >( storageModel() )->msgBase( mi );
}

Core::MessageItem * Widget::messageItemFromMsgBase( KMMsgBase * msg ) const
{
  if ( !storageModel() )
    return 0;

  int row = static_cast< StorageModel * >( storageModel() )->msgBaseRow( msg );
  if ( row < 0 )
    return 0;

  return view()->model()->messageItemByStorageRow( row );
}

void Widget::activateMessageItemByMsgBase( KMMsgBase * msg )
{
  // This function may be expensive since it needs to perform a linear search
  // in the storage. We want to avoid that so we use some tricks.

  if ( !storageModel() )
    return;


  Core::MessageItem * mi = 0;

  // take care of current first
  if ( currentMsgBase() != msg ) // this 
  {
    mi = messageItemFromMsgBase( msg );

    view()->setCurrentMessageItem( mi ); // clears the current if mi == 0

    if ( !mi )
    {
      // aargh.. not found
      view()->clearSelection();
      return;
    }
  }

  // take care of selection

  // As the current selection is smaller than the whole folder and as per
  // the locality principle it's likely that we're trying to activate
  // and already selected item then loop through the selection first.
  QList< Core::MessageItem * > selectedItems = selectionAsMessageItemList( false );
  bool foundInSelection = false;

  QItemSelection sel;

  for ( QList< Core::MessageItem * >::ConstIterator it = selectedItems.constBegin(); it != selectedItems.constEnd(); ++it )
  {
    if ( static_cast< StorageModel * >( storageModel() )->msgBase( *it ) == msg )
    {
      foundInSelection = true;
    } else {
      sel.append( QItemSelectionRange( view()->model()->index( *it, 0 ) ) );
    }
  }

  if ( foundInSelection )
  {
    if ( !sel.isEmpty() )
      view()->selectionModel()->select( sel, QItemSelectionModel::Deselect | QItemSelectionModel::Rows );
    return;
  }

  // not found in selection... select as only item
  if ( !mi )
  {
    // doh.. need a linear search :(
    mi = messageItemFromMsgBase( msg );
    if ( !mi )
    {
      // weird...
      view()->clearSelection();
      return;
    }
  }

  sel.clear();
  sel.append( QItemSelectionRange( view()->model()->index( mi, 0 ) ) );
  view()->selectionModel()->select( sel, QItemSelectionModel::Select | QItemSelectionModel::Rows | QItemSelectionModel::Clear );
}


QList< Core::MessageItem * > Widget::selectionAsMessageItemList( bool includeCollapsedChildren ) const
{
  return view()->selectionAsMessageItemList( includeCollapsedChildren );
}

QList< KMMessage * > Widget::selectionAsMessageList( bool includeCollapsedChildren ) const
{
  QList< KMMessage * > ret;
  if ( !storageModel() )
    return ret;

  QList< Core::MessageItem * > selectedItems = view()->selectionAsMessageItemList( includeCollapsedChildren );

  for ( QList< Core::MessageItem * >::ConstIterator it = selectedItems.constBegin(); it != selectedItems.constEnd(); ++it )
  {
    KMMessage * msg = static_cast< StorageModel * >( storageModel() )->message( *it );
    Q_ASSERT( msg );
    ret.append( msg );
  }

  return ret;
}

QList< KMMsgBase * > Widget::selectionAsMsgBaseList( bool includeCollapsedChildren ) const
{
  QList< KMMsgBase * > ret;
  if ( !storageModel() )
    return ret;

  QList< Core::MessageItem * > selectedItems = view()->selectionAsMessageItemList( includeCollapsedChildren );

  for ( QList< Core::MessageItem * >::ConstIterator it = selectedItems.constBegin(); it != selectedItems.constEnd(); ++it )
  {
    KMMsgBase * msg = static_cast< StorageModel * >( storageModel() )->msgBase( *it );
    Q_ASSERT( msg );
    ret.append( msg );
  }

  return ret;
}

MessageTreeCollection * Widget::itemListToMessageTreeCollection( const QList< Core::MessageItem * > &list ) const
{
  if ( list.isEmpty() )
    return 0;

  MessageTreeCollection * collection = new MessageTreeCollection();

  QHash< Core::MessageItem *, MessageTree * > messageItemsToMessageTrees;

  for( QList< Core::MessageItem * >::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it )
  {
    KMMsgBase * msg = static_cast< StorageModel * >( storageModel() )->msgBase( *it );
    Q_ASSERT( msg );

    MessageTree * tree = new MessageTree( msg );

    messageItemsToMessageTrees.insert( *it, tree );
  }

  for( QList< Core::MessageItem * >::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it )
  {
    MessageTree * tree = messageItemsToMessageTrees.value( *it, 0 );

    Q_ASSERT( tree );

    Core::Item * pParent = ( *it )->parent();

    bool attached = false;

    while ( pParent )
    {
      if ( pParent->type() != Core::Item::Message )
        break;

      MessageTree * parentTree = messageItemsToMessageTrees.value( static_cast< Core::MessageItem * >( pParent ), 0 );
      if( parentTree )
      {
        parentTree->addChild( tree );
        attached = true;
        break;
      }

      pParent = pParent->parent();
    }

    if ( !attached )
      collection->addTree( tree );
  }  

  return collection;
}

MessageTreeCollection * Widget::selectionAsMessageTreeCollection( bool includeCollapsedChildren ) const
{
  if ( !storageModel() )
    return 0;

  return itemListToMessageTreeCollection( view()->selectionAsMessageItemList( includeCollapsedChildren ) );
}


QList< Core::MessageItem * > Widget::currentThreadAsMessageItemList() const
{
  return view()->currentThreadAsMessageItemList();
}

QList< KMMsgBase * > Widget::currentThreadAsMsgBaseList() const
{
  QList< KMMsgBase * > ret;
  if ( !storageModel() )
    return ret;

  QList< Core::MessageItem * > currentThreadItems = view()->currentThreadAsMessageItemList();

  for ( QList< Core::MessageItem * >::ConstIterator it = currentThreadItems.constBegin(); it != currentThreadItems.constEnd(); ++it )
  {
    KMMsgBase * msg = static_cast< StorageModel * >( storageModel() )->msgBase( *it );
    Q_ASSERT( msg );
    ret.append( msg );
  }

  return ret;
}

QList< KMMessage * > Widget::currentThreadAsMessageList() const
{
  QList< KMMessage * > ret;
  if ( !storageModel() )
    return ret;

  QList< Core::MessageItem * > currentThreadItems = view()->currentThreadAsMessageItemList();

  for ( QList< Core::MessageItem * >::ConstIterator it = currentThreadItems.constBegin(); it != currentThreadItems.constEnd(); ++it )
  {
    KMMessage * msg = static_cast< StorageModel * >( storageModel() )->message( *it );
    Q_ASSERT( msg );
    ret.append( msg );
  }

  return ret;
}

MessageTreeCollection * Widget::currentThreadAsMessageTreeCollection() const
{
  if ( !storageModel() )
    return 0;

  return itemListToMessageTreeCollection( view()->currentThreadAsMessageItemList() );
}

Core::MessageItemSetReference Widget::createPersistentSetFromMessageItemList( const QList< Core::MessageItem * > &items )
{
  if ( !storageModel() )
    return static_cast< Core::MessageItemSetReference >( 0 ); // never exists

  if ( items.isEmpty() )
    return static_cast< Core::MessageItemSetReference >( 0 );

  return view()->createPersistentSet( items );
}

QList< KMMsgBase * > Widget::persistentSetContentsAsMsgBaseList( Core::MessageItemSetReference ref ) const
{
  QList< KMMsgBase * > ret;
  if ( !storageModel() )
    return ret;

  QList< Core::MessageItem * > setItems = view()->persistentSetCurrentMessageItemList( ref );
  if ( setItems.isEmpty() )
    return ret;

  for ( QList< Core::MessageItem * >::ConstIterator it = setItems.constBegin(); it != setItems.constEnd(); ++it )
  {
    KMMsgBase * msg = static_cast< StorageModel * >( storageModel() )->msgBase( *it );
    Q_ASSERT( msg );
    ret.append( msg );
  }

  return ret;  
}

void Widget::markPersistentSetAsAboutToBeRemoved( Core::MessageItemSetReference ref, bool bMark )
{
  QList< KMMsgBase * > ret;
  if ( !storageModel() )
    return;

  QList< Core::MessageItem * > setItems = view()->persistentSetCurrentMessageItemList( ref );
  if ( setItems.isEmpty() )
    return;

  view()->markMessageItemsAsAboutToBeRemoved( setItems, bMark );
}

void Widget::selectPersistentSet( Core::MessageItemSetReference ref, bool clearSelectionFirst ) const
{
  QList< KMMsgBase * > ret;
  if ( !storageModel() )
    return;

  QList< Core::MessageItem * > setItems = view()->persistentSetCurrentMessageItemList( ref );
  if ( setItems.isEmpty() )
    return;

  if ( clearSelectionFirst )
    view()->clearSelection();

  view()->selectMessageItems( setItems );
}

void Widget::viewSelectionChanged()
{
  emit selectionChanged();
  if ( !currentMessageItem() )
    emit messageSelected( 0 );
}

void Widget::deletePersistentSet( Core::MessageItemSetReference ref )
{
  view()->deletePersistentSet( ref );
}

bool Widget::getSelectionStats(
      QList< quint32 > &selectedSernums,
      QList< quint32 > &selectedVisibleSernums,
      bool * allSelectedBelongToSameThread,
      bool includeCollapsedChildren
    ) const
{
  if ( !storageModel() )
    return false;

  selectedSernums.clear();
  selectedVisibleSernums.clear();

  QList< Core::MessageItem * > selected = selectionAsMessageItemList( includeCollapsedChildren );

  Core::MessageItem * topmost = 0;

  *allSelectedBelongToSameThread = true;

  for ( QList< Core::MessageItem * >::ConstIterator it = selected.constBegin(); it != selected.constEnd(); ++it )
  {
    KMMsgBase * mb = static_cast< StorageModel * >( storageModel() )->msgBase( ( *it ) );
    Q_ASSERT( mb );

    selectedSernums.append( mb->getMsgSerNum() );
    if ( view()->isDisplayedWithParentsExpanded( ( *it ) ) )
      selectedVisibleSernums.append( mb->getMsgSerNum() );

    if ( topmost == 0 )
      topmost = ( *it )->topmostMessage();
    else {
      if ( topmost != ( *it )->topmostMessage() )
        *allSelectedBelongToSameThread = false;
    }
  }

  return true;
}

bool Widget::selectionEmpty() const
{
  return view()->selectionEmpty();
}


void Widget::viewMessageListContextPopupRequest( const QList< Core::MessageItem * > &selectedItems, const QPoint &globalPos )
{
  if ( !topLevelWidget() ) return; // safe bet
  if ( selectedItems.isEmpty() ) return; // we have nothing to do

  Q_ASSERT( storageModel() );

  mMainWidget->updateMessageMenu(); 
  //mMainWidget->updateMessageActions(); <-- this is called by updateMessageMenu()

  KMenu menu( this );


  bool out_folder = kmkernel->folderIsDraftOrOutbox( folder() );
  bool tem_folder = kmkernel->folderIsTemplates( folder() );

  if (tem_folder)
  {
    menu.addAction( mMainWidget->useAction() );
  } else {
    // show most used actions
    menu.addAction( mMainWidget->messageActions()->replyMenu() );
    menu.addAction( mMainWidget->messageActions()->forwardMenu() );
    if( mMainWidget->sendAgainAction()->isEnabled() )
      menu.addAction( mMainWidget->sendAgainAction() );
    else
      menu.addAction( mMainWidget->editAction() );
  }

  menu.addSeparator();

  menu.addAction( mMainWidget->action( "copy_to" ) );

  if ( folder()->isReadOnly() )
  {
    QAction* act = menu.addAction( i18n( "&Move To" ) );
    act->setEnabled( false );
  } else {
    menu.addAction( mMainWidget->action( "move_to" ) );
  }

  menu.addSeparator();
  menu.addAction( mMainWidget->messageActions()->messageStatusMenu() ); // Mark Message menu
  if ( mMainWidget->threadStatusMenu()->isEnabled() )
    menu.addAction( mMainWidget->threadStatusMenu() ); // Mark Thread menu

  if ( !out_folder )
  {
    menu.addSeparator();
    menu.addAction( mMainWidget->filterMenu() ); // Create Filter menu
    menu.addAction( mMainWidget->action( "apply_filter_actions" ) );
  }

  menu.addSeparator();

  menu.addAction( mMainWidget->printAction() );
  menu.addAction( mMainWidget->saveAsAction() );
  menu.addAction( mMainWidget->saveAttachmentsAction() );

  menu.addSeparator();

  if ( folder()->isTrash() )
  {
    menu.addAction( mMainWidget->deleteAction() );
    if ( mMainWidget->trashThreadAction()->isEnabled() )
      menu.addAction( mMainWidget->deleteThreadAction() );
  } else {
    menu.addAction( mMainWidget->trashAction() );
    if ( mMainWidget->trashThreadAction()->isEnabled() )
      menu.addAction( mMainWidget->trashThreadAction() );
  }

  menu.addSeparator();

  menu.addAction( mMainWidget->messageActions()->createTodoAction() );

  KAcceleratorManager::manage( &menu );
  kmkernel->setContextMenuShown( true );
  menu.exec( globalPos );
  kmkernel->setContextMenuShown( false );
}

void Widget::viewGroupHeaderContextPopupRequest( Core::GroupHeaderItem *ghi, const QPoint &globalPos )
{
  Q_UNUSED( ghi );

  KMenu menu( this );

  QAction *act;

  menu.addSeparator();

  act = menu.addAction( i18n( "Expand All Groups" ) );
  connect(
      act, SIGNAL( triggered( bool ) ),
      view(), SLOT( slotExpandAllGroups() )
    );

  act = menu.addAction( i18n( "Collapse All Groups" ) );
  connect(
      act, SIGNAL( triggered( bool ) ),
      view(), SLOT( slotCollapseAllGroups() )
    );

  menu.exec( globalPos );

}

bool Widget::canAcceptDrag( const QDragMoveEvent * e )
{
  KMFolder * fld = folder();

  if ( !fld )
    return false; // no folder here

  if ( fld->isReadOnly() )
    return false; // no way to drag into

  if ( !e->mimeData()->hasFormat( KPIM::MailList::mimeDataType() ) )
    return false; // no way to decode it

  KPIM::MailList list = KPIM::MailList::fromMimeData( e->mimeData() );

  if ( list.isEmpty() )
  {
    kWarning() << "Could not decode drag data!";
    return false;
  }

  QList<uint> serNums = MessageCopyHelper::serNumListFromMailList( list );

  KMFolder * sourceFolder = MessageCopyHelper::firstSourceFolder( serNums );

  return sourceFolder != fld;
}

void Widget::viewDragEnterEvent( QDragEnterEvent *e )
{
  if ( !canAcceptDrag( e ) )
  {
    e->ignore();
    return;
  }

  e->accept();
}

void Widget::viewDragMoveEvent( QDragMoveEvent *e )
{
  if ( !canAcceptDrag( e ) )
  {
    e->ignore();
    return;
  }

  e->accept();
}

void Widget::viewStartDragRequest()
{
  if ( !folder() )
    return;

  QList< KMMsgBase * > selectedList = selectionAsMsgBaseList();
  if ( selectedList.isEmpty() )
    return;

  KPIM::MailList mailList;

  for ( QList< KMMsgBase * >::Iterator it = selectedList.begin(); it != selectedList.end(); ++it )
  {
    KPIM::MailSummary mailSummary(
        ( *it )->getMsgSerNum(), ( *it )->msgIdMD5(),
        ( *it )->subject(), ( *it )->fromStrip(),
        ( *it )->toStrip(), ( *it )->date()
      );

    mailList.append( mailSummary );
  }

  QDrag *drag = new QDrag( view()->viewport() );
  drag->setMimeData( new KPIM::MailListMimeData( new KMTextSource ) );

  mailList.populateMimeData( drag->mimeData() );

  // Set pixmap
  QPixmap pixmap;
  if( selectedList.count() == 1 )
    pixmap = QPixmap( DesktopIcon("mail-message", KIconLoader::SizeSmall) );
  else
    pixmap = QPixmap( DesktopIcon("document-multiple", KIconLoader::SizeSmall) );

  // Calculate hotspot (as in Konqueror)
  if( !pixmap.isNull() )
  {
    drag->setHotSpot( QPoint( pixmap.width() / 2, pixmap.height() / 2 ) );
    drag->setPixmap( pixmap );
  }

  if ( folder()->isReadOnly() )
    drag->exec( Qt::CopyAction );
  else
    drag->exec( Qt::CopyAction | Qt::MoveAction );
}

enum DragMode
{
  DragCopy,
  DragMove,
  DragCancel
};

void Widget::viewDropEvent( QDropEvent *e )
{
  KMFolder * fld = folder();

  if ( !fld || !e->mimeData()->hasFormat( KPIM::MailList::mimeDataType() ) )
  {
    e->ignore();
    return;
  }

  KPIM::MailList list = KPIM::MailList::fromMimeData( e->mimeData() );
  if ( list.isEmpty() )
  {
    kWarning() << "Could not decode drag data!";
    e->ignore();
    return;
  }

  e->accept();

  QList<uint> serNums = MessageCopyHelper::serNumListFromMailList( list );
  int action;
  if ( MessageCopyHelper::inReadOnlyFolder( serNums ) )
    action = DragCopy;
  else {
    action = DragCancel;
    int keybstate = kapp->keyboardModifiers();
    if ( keybstate & Qt::CTRL )
    {
      action = DragCopy;
    } else if ( keybstate & Qt::SHIFT )
    {
      action = DragMove;
    } else {
      // FIXME: This code is duplicated almost exactly in FolderView... shouldn't we share ?
      // FIXME: anybody sets this option to false ?
      if ( GlobalSettings::self()->showPopupAfterDnD() )
      { 
        KMenu menu;
        QAction *moveAction = menu.addAction( KIcon( "go-jump"), i18n( "&Move Here" ) );
        QAction *copyAction = menu.addAction( KIcon( "edit-copy" ), i18n( "&Copy Here" ) );
        menu.addSeparator();
        menu.addAction( KIcon( "dialog-cancel" ), i18n( "C&ancel" ) );

        QAction *menuChoice = menu.exec( QCursor::pos() );
        if ( menuChoice == moveAction )
          action = DragMove;
        else if ( menuChoice == copyAction )
          action = DragCopy;
        else
          action = DragCancel;
      } else {
        action = DragMove;
      }
    }
  }

  if ( action == DragCopy || action == DragMove )
    new MessageCopyHelper( serNums, fld, action == DragMove, this );
}

void Widget::animateIcon()
{
  mPane->widgetIconChangeRequest( this, mFolderIconBusy[ mIconAnimationStep ] );
  mIconAnimationStep++;
  if ( mIconAnimationStep >= BusyAnimationSteps )
    mIconAnimationStep = 0;
}

void Widget::viewJobBatchStarted()
{
  mIconAnimationStep = 0;
  animateIcon();
  mIconAnimationTimer->start( 500 ); // don't waste too much time with this
}

void Widget::viewJobBatchTerminated()
{
  mIconAnimationTimer->stop();
  mPane->widgetIconChangeRequest( this, mFolderIconBase );
}

QActionGroup * Widget::fillMessageTagMenu( KMenu * menu )
{
  // Used for sorting, go through this routine so that mMsgTagList is properly ordered
  KMMessageTagList sortedTags;

  QHashIterator<QString,KMMessageTagDescription*> it( *( kmkernel->msgTagMgr()->msgTagDict() ) );

  while (it.hasNext())
  {
    it.next();
    sortedTags.append( it.value()->label() );
  }

  if ( sortedTags.isEmpty() )
    return 0;

  sortedTags.prioritySort();

  QActionGroup * grp = new QActionGroup( menu );

  QAction * act;

  QString currentTagId = currentFilterTagId();

  for ( KMMessageTagList::Iterator itl = sortedTags.begin(); itl != sortedTags.end(); ++itl )
  {
    const KMMessageTagDescription *description = kmkernel->msgTagMgr()->find( *itl );
    if ( description )
    {
      act = menu->addAction( description->name() );
      act->setIcon( SmallIcon( description->toolbarIconName() ) );
      act->setCheckable( true );
      act->setChecked( description->label() == currentTagId );
      act->setData( QVariant( description->label() ) );
      grp->addAction( act );
    }
  }

  return grp;
}


} // namespace MessageListView

} // namespace KMail

