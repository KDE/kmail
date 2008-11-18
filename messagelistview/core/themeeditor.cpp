/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free softhisare; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Softhisare Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Softhisare
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "messagelistview/core/themeeditor.h"
#include "messagelistview/core/theme.h"
#include "messagelistview/core/groupheaderitem.h"
#include "messagelistview/core/messageitem.h"
#include "messagelistview/core/modelinvariantrowmapper.h"
#include "messagelistview/core/manager.h"
#include "messagelistview/core/comboboxutils.h"

#include <libkdepim/messagestatus.h>

#include <QActionGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QCursor>
#include <QDrag>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTextEdit>
#include <QTreeWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPushButton>
#include <QStringList>

#include <KLocale>
#include <KFontDialog>
#include <KMenu>
#include <KIconLoader>

#include <time.h> // for time_t

namespace KMail
{

namespace MessageListView
{

namespace Core
{

static const char * gThemeContentItemTypeDndMimeDataFormat = "application/x-kmail-messagelistview-theme-contentitem-type";





ThemeColumnPropertiesDialog::ThemeColumnPropertiesDialog( QWidget * parent, Theme::Column * column, const QString &title )
  : KDialog( parent ), mColumn( column )
{
  //setAttribute( Qt::WA_DeleteOnClose );
  setWindowModality( Qt::ApplicationModal ); // FIXME: Sure ?
  setButtons( Ok | Cancel );
  setWindowTitle( title );

  QWidget * base = new QWidget( this );
  setMainWidget( base );

  QGridLayout * g = new QGridLayout( base );

  QLabel * l;

  l = new QLabel( i18nc( "@label:textbox Property name", "Name:" ), base );
  g->addWidget( l, 0, 0 );

  mNameEdit = new QLineEdit( base );
  mNameEdit->setToolTip( i18n( "The label that will be displayed in the column header." ) );
  g->addWidget( mNameEdit, 0, 1 );

  l = new QLabel( i18n( "Header Click Sorts Messages" ), base );
  g->addWidget( l, 1, 0 );

  mMessageSortingCombo = new QComboBox( base );
  mMessageSortingCombo->setToolTip( i18n( "The sorting order that cliking on this column header will switch to." ) );
  g->addWidget( mMessageSortingCombo, 1, 1 );

  mVisibleByDefaultCheck = new QCheckBox( i18n( "Visible by Default" ), base );
  mVisibleByDefaultCheck->setToolTip( i18n( "Check this if this column should be visible when the theme is selected." ) );
  g->addWidget( mVisibleByDefaultCheck, 2, 1 );

  mIsSenderOrReceiverCheck = new QCheckBox( i18n( "Contains \"Sender or Receiver\" Field" ), base );
  mIsSenderOrReceiverCheck->setToolTip( i18n( "Check this if this column label should be updated depending on the folder \"inbound\"/\"outbound\" type." ) );
  g->addWidget( mIsSenderOrReceiverCheck, 3, 1 );

  g->setColumnStretch( 1, 1 );
  g->setRowStretch( 10, 1 );

  connect( this, SIGNAL( okClicked() ),
           SLOT( slotOkButtonClicked() ) );

  // Display the current settings
  mNameEdit->setText( mColumn->label() );
  mVisibleByDefaultCheck->setChecked( mColumn->visibleByDefault() );
  mIsSenderOrReceiverCheck->setChecked( mColumn->isSenderOrReceiver() );
  ComboBoxUtils::fillIntegerOptionCombo( mMessageSortingCombo, Aggregation::enumerateMessageSortingOptions( Aggregation::PerfectReferencesAndSubject ) );
  ComboBoxUtils::setIntegerOptionComboValue( mMessageSortingCombo, mColumn->messageSorting() );

}

void ThemeColumnPropertiesDialog::slotOkButtonClicked()
{
  QString text = mNameEdit->text();
  if ( text.isEmpty() )
    text = i18n( "Unnamed Column" );
  mColumn->setLabel( text );
  mColumn->setVisibleByDefault( mVisibleByDefaultCheck->isChecked() );
  mColumn->setIsSenderOrReceiver( mIsSenderOrReceiverCheck->isChecked() );
  mColumn->setMessageSorting(
      static_cast< Aggregation::MessageSorting >(
          ComboBoxUtils::getIntegerOptionComboValue( mMessageSortingCombo, Aggregation::NoMessageSorting )
        )
    );

  accept();
}






ThemeContentItemSourceLabel::ThemeContentItemSourceLabel( QWidget * parent, Theme::ContentItem::Type type )
  : QLabel( parent ), mType( type )
{
  setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
}

ThemeContentItemSourceLabel::~ThemeContentItemSourceLabel()
{
}

void ThemeContentItemSourceLabel::mousePressEvent( QMouseEvent * e )
{
  if ( e->button() == Qt::LeftButton )
    mMousePressPoint = e->pos();
}

void ThemeContentItemSourceLabel::mouseMoveEvent( QMouseEvent * e )
{
  if ( e->buttons() & Qt::LeftButton )
  {
    QPoint diff = mMousePressPoint - e->pos();
    if ( diff.manhattanLength() > 4 )
      startDrag();
  }
}

void ThemeContentItemSourceLabel::startDrag()
{
  //QPixmap pix = QPixmap::grabWidget( this );
  //QPixmap alpha( pix.width(), pix.height() );
  //alpha.fill(0x0f0f0f0f);
  //pix.setAlphaChannel( alpha ); // <-- this crashes... no alpha for dragged pixmap :(
  QMimeData * data = new QMimeData();
  QByteArray arry;
  arry.resize( sizeof( Theme::ContentItem::Type ) );
  *( ( Theme::ContentItem::Type * ) arry.data() ) = mType;
  data->setData( gThemeContentItemTypeDndMimeDataFormat, arry );
  QDrag * drag = new QDrag( this );
  drag->setMimeData( data );
  //drag->setPixmap( pix );
  //drag->setHotSpot( mapFromGlobal( QCursor::pos() ) );
  drag->exec( Qt::CopyAction, Qt::CopyAction );
}

ThemePreviewDelegate::ThemePreviewDelegate( QAbstractItemView * parent, QPaintDevice * paintDevice )
  : ThemeDelegate( parent, paintDevice )
{
  mRowMapper = new ModelInvariantRowMapper();

  mSampleGroupHeaderItem = new GroupHeaderItem( i18n( "Message Group" ) );

  mSampleGroupHeaderItem->setDate( time( 0 ) );
  mSampleGroupHeaderItem->setMaxDate( time( 0 ) + 31337 );
  mSampleGroupHeaderItem->setSenderOrReceiver( i18n( "Sender/Receiver" ) );
  mSampleGroupHeaderItem->setSubject( i18n( "Very long subject very long subject very long subject very long subject very long subject very long" ) );

  mSampleMessageItem = new MessageItem();

  mSampleMessageItem->setDate( time( 0 ) );
  mSampleMessageItem->setSize( 0x31337 );
  mSampleMessageItem->setMaxDate( time( 0 ) + 31337 );
  mSampleMessageItem->setSenderOrReceiver( i18n( "Sender/Receiver" ) );
  mSampleMessageItem->setSender( i18n( "Sender" ) );
  mSampleMessageItem->setReceiver( i18n( "Receiver" ) );
  mSampleMessageItem->setSubject( i18n( "Very long subject very long subject very long subject very long subject very long subject very long" ) );
  mSampleMessageItem->setSignatureState( MessageItem::FullySigned );
  mSampleMessageItem->setEncryptionState( MessageItem::FullyEncrypted );

  QList< MessageItem::Tag * > * list = new QList< MessageItem::Tag * >();
  list->append( new MessageItem::Tag( SmallIcon( "feed-subscribe" ), i18n( "Sample Tag 1" ), QString() ) );
  list->append( new MessageItem::Tag( SmallIcon( "feed-subscribe" ), i18n( "Sample Tag 2" ), QString() ) );
  list->append( new MessageItem::Tag( SmallIcon( "feed-subscribe" ), i18n( "Sample Tag 3" ), QString() ) );
  mSampleMessageItem->setTagList( list );

  mRowMapper->createModelInvariantIndex( 0, mSampleMessageItem );

  mSampleGroupHeaderItem->rawAppendChildItem( mSampleMessageItem );
  mSampleMessageItem->setParent( mSampleGroupHeaderItem );

  KPIM::MessageStatus stat;

  stat.fromQInt32( 0x7fffffff );
  stat.setQueued( false );
  stat.setSent( false );
  stat.setNew();
  stat.setSpam( true );
  stat.setWatched( true );
  //stat.setHasAttachment( false );

  mSampleMessageItem->setStatus( stat );
}

ThemePreviewDelegate::~ThemePreviewDelegate()
{
  delete mSampleGroupHeaderItem;
  //delete mSampleMessageItem; (deleted by the parent)
  delete mRowMapper;
}

Item * ThemePreviewDelegate::itemFromIndex( const QModelIndex &index ) const
{
  if ( index.parent().isValid() )
    return mSampleMessageItem;

  return mSampleGroupHeaderItem;
}





ThemePreviewWidget::ThemePreviewWidget( QWidget * parent )
  : QTreeWidget( parent )
{
  mSelectedThemeContentItem = 0;
  mSelectedThemeColumn = 0;
  mFirstShow = true;

  mDelegate = new ThemePreviewDelegate( this, viewport() );
  setItemDelegate( mDelegate );
  setRootIsDecorated( false );
  viewport()->setAcceptDrops( true );

  header()->setContextMenuPolicy( Qt::CustomContextMenu ); // make sure it's true
  connect( header(), SIGNAL( customContextMenuRequested( const QPoint& ) ),
           SLOT( slotHeaderContextMenuRequested( const QPoint& ) ) );

  mGroupHeaderSampleItem = new QTreeWidgetItem( this );
  mGroupHeaderSampleItem->setText( 0, QString() );
  mGroupHeaderSampleItem->setFlags( Qt::ItemIsEnabled );

  QTreeWidgetItem * m = new QTreeWidgetItem( mGroupHeaderSampleItem );
  m->setText( 0, QString() );

  mGroupHeaderSampleItem->setExpanded( true );
}

ThemePreviewWidget::~ThemePreviewWidget()
{
}

QSize ThemePreviewWidget::sizeHint() const
{
  return QSize( 350, 180 );
}

void ThemePreviewWidget::applyThemeColumnWidths()
{
  if ( !mTheme )
    return;

  const QList< Theme::Column * > & columns = mTheme->columns();

  if ( columns.count() < 1 )
  {
    viewport()->update(); // trigger a repaint
    return;
  }

  // Now we want to distribute the available width on all the columns.
  // The algorithm used here is very similar to the one used in View::applyThemeColumns().
  // It just takes care of ALL the columns instead of the visible ones.

  QList< Theme::Column * >::ConstIterator it;

  // Gather size hints for all sections.
  int idx = 0;
  int totalVisibleWidthHint = 0;

  for ( it = columns.begin(); it != columns.end(); ++it )
  {
    totalVisibleWidthHint += mDelegate->sizeHintForItemTypeAndColumn( Item::Message, idx ).width();
    idx++;
  }

  if ( totalVisibleWidthHint < 16 )
    totalVisibleWidthHint = 16; // be reasonable

  // Now we can compute proportional widths.

  idx = 0;

  QList< int > realWidths;
  int totalVisibleWidth = 0;

  for ( it = columns.begin(); it != columns.end(); ++it )
  {
    int hintWidth = mDelegate->sizeHintForItemTypeAndColumn( Item::Message, idx ).width();
    int realWidth;
    if ( ( *it )->containsTextItems() )
    {
       // the column contains text items, it should get more space
       realWidth = ( ( hintWidth * viewport()->width() ) / totalVisibleWidthHint ) - 2; // -2 is heuristic
       if ( realWidth < ( hintWidth + 2 ) )
         realWidth = hintWidth + 2; // can't be less
    } else {
       // the column contains no text items, it should get just a little bit more than its sizeHint().
       realWidth = hintWidth + 2;
    }

    realWidths.append( realWidth );
    totalVisibleWidth += realWidth;

    idx++;
  }

  idx = 0;

  totalVisibleWidth += 4; // account for some view's border

  if ( totalVisibleWidth < viewport()->width() )
  {
    // give the additional space to the text columns
    // also give more space to the first ones and less space to the last ones
    int available = viewport()->width() - totalVisibleWidth;

    for ( it = columns.begin(); it != columns.end(); ++it )
    {
      if ( ( ( *it )->visibleByDefault() || ( idx == 0 ) ) && ( *it )->containsTextItems() )
      {
        // give more space to this column
        available >>= 1; // eat half of the available space
        realWidths[ idx ] += available; // and give it to this column
      }

      idx++;
    }

    // if any space is still available, give it to the first column
    if ( available )
      realWidths[ 0 ] += available;
  }


  idx = 0;

  // We're ready.
  // Assign widths. Hide the sections that are not visible by default, show the other ones.
  for ( it = columns.begin(); it != columns.end(); ++it )
  {
    header()->resizeSection( idx, realWidths[ idx ] );
    idx++;
  }

#if 0
  if( mTheme->viewHeaderPolicy() == Theme::NeverShowHeader )
    header()->hide();
  else
    header()->show();
#endif
}


void ThemePreviewWidget::setTheme( Theme * theme )
{
  bool themeChanged = theme != mTheme;

  mSelectedThemeContentItem = 0;
  mTheme = theme;
  mDelegate->setTheme( theme );
  mGroupHeaderSampleItem->setExpanded( true );

  const QList< Theme::Column * > & columns = mTheme->columns();

  setColumnCount( columns.count() );

  QStringList headerLabels;

  for( QList< Theme::Column * >::ConstIterator it = columns.begin(); it != columns.end(); ++it )
  {
    QString label = ( *it )->label();
    if ( ( *it )->visibleByDefault() )
      label += QString( " (%1)" ).arg( i18nc( "Indicates whether or not a header label is visible", "Visible") );

    headerLabels.append( label );
  }

  setHeaderLabels( headerLabels );

  if ( themeChanged )
    applyThemeColumnWidths();

  viewport()->update(); // trigger a repaint
}

void ThemePreviewWidget::internalHandleDragEnterEvent( QDragEnterEvent * e )
{
  e->ignore();

  if ( !e->mimeData() )
    return;
  if ( !e->mimeData()->hasFormat( gThemeContentItemTypeDndMimeDataFormat ) )
    return;

  e->accept();
}

void ThemePreviewWidget::showEvent( QShowEvent * e )
{
  QTreeWidget::showEvent( e );

  if ( mFirstShow )
  {
    // Make sure we re-apply column widths the first time we're shown.
    // The first "apply" call was made while the widget was still hidden and
    // almost surely had wrong sizes.
    applyThemeColumnWidths();
    mFirstShow = false;
  }
}

void ThemePreviewWidget::dragEnterEvent( QDragEnterEvent * e )
{
  internalHandleDragEnterEvent( e );

  mThemeSelectedContentItemRect = QRect();

  viewport()->update(); // trigger a repaint
}

void ThemePreviewWidget::internalHandleDragMoveEvent( QDragMoveEvent * e )
{
  e->ignore();

  if ( !e->mimeData() )
    return;
  if ( !e->mimeData()->hasFormat( gThemeContentItemTypeDndMimeDataFormat ) )
    return;

  QByteArray arry = e->mimeData()->data( gThemeContentItemTypeDndMimeDataFormat );

  if ( arry.size() != sizeof( Theme::ContentItem::Type ) )
    return; // ugh

  Theme::ContentItem::Type type = *( ( Theme::ContentItem::Type * ) arry.data() );

  if ( !computeContentItemInsertPosition( e->pos(), type ) )
    return;

  e->accept();
}

void ThemePreviewWidget::dragMoveEvent( QDragMoveEvent * e )
{
  internalHandleDragMoveEvent( e );

  mThemeSelectedContentItemRect = QRect();

  viewport()->update(); // trigger a repaint  
}


void ThemePreviewWidget::dropEvent( QDropEvent * e )
{
  mDropIndicatorPoint1 = mDropIndicatorPoint2;

  e->ignore();

  if ( !e->mimeData() )
    return;
  
  if ( !e->mimeData()->hasFormat( gThemeContentItemTypeDndMimeDataFormat ) )
    return;

  QByteArray arry = e->mimeData()->data( gThemeContentItemTypeDndMimeDataFormat );

  if ( arry.size() != sizeof( Theme::ContentItem::Type ) )
    return; // ugh

  Theme::ContentItem::Type type = *( ( Theme::ContentItem::Type * ) arry.data() );

  if ( !computeContentItemInsertPosition( e->pos(), type ) )
  {
    viewport()->update();
    return;
  }

  Theme::Row * row = 0;

  switch ( mRowInsertPosition )
  {
    case AboveRow:
      row = new Theme::Row();
      if ( mDelegate->hitItem()->type() == Item::Message )
        const_cast< Theme::Column * >( mDelegate->hitColumn() )->insertMessageRow( mDelegate->hitRowIndex(), row );
      else
        const_cast< Theme::Column * >( mDelegate->hitColumn() )->insertGroupHeaderRow( mDelegate->hitRowIndex(), row );
    break;
    case InsideRow:
      row = const_cast< Theme::Row * >( mDelegate->hitRow() );
    break;
    case BelowRow:
      row = new Theme::Row();
      if ( mDelegate->hitItem()->type() == Item::Message )
        const_cast< Theme::Column * >( mDelegate->hitColumn() )->insertMessageRow( mDelegate->hitRowIndex()+1, row );
      else
        const_cast< Theme::Column * >( mDelegate->hitColumn() )->insertGroupHeaderRow( mDelegate->hitRowIndex()+1, row );
    break;
  }

  if ( !row )
    return;

  Theme::ContentItem * ci = new Theme::ContentItem( type );
  if ( ci->canBeDisabled() )
  {
    if ( ci->isClickable() )
      ci->setSoftenByBlendingWhenDisabled( true ); // default to softened
    else
      ci->setHideWhenDisabled( true ); // default to hidden
  }

  int idx;

  switch( mItemInsertPosition )
  {
    case OnLeftOfItem:
      if ( !mDelegate->hitContentItem() )
      {
        // bleah
        delete ci;
        return;
      }
      idx = mDelegate->hitContentItemRight() ? \
              row->rightItems().indexOf( const_cast< Theme::ContentItem * >( mDelegate->hitContentItem() ) ) : \
              row->leftItems().indexOf( const_cast< Theme::ContentItem * >( mDelegate->hitContentItem() ) );
      if ( idx < 0 )
      {
        // bleah
        delete ci;
        return;
      }
      if ( mDelegate->hitContentItemRight() )
        row->insertRightItem( idx+1, ci );
      else
        row->insertLeftItem( idx, ci );
    break;
    case OnRightOfItem:
      if ( !mDelegate->hitContentItem() )
      {
        // bleah
        delete ci;
        return;
      }
      idx = mDelegate->hitContentItemRight() ? \
              row->rightItems().indexOf( const_cast< Theme::ContentItem * >( mDelegate->hitContentItem() ) ) : \
              row->leftItems().indexOf( const_cast< Theme::ContentItem * >( mDelegate->hitContentItem() ) );
      if ( idx < 0 )
      {
        // bleah
        delete ci;
        return;
      }
      if ( mDelegate->hitContentItemRight() )
        row->insertRightItem( idx, ci );
      else
        row->insertLeftItem( idx+1, ci );
    break;
    case AsLastLeftItem:
      row->addLeftItem( ci );
    break;
    case AsLastRightItem:
      row->addRightItem( ci );
    break;
    case AsFirstLeftItem:
      row->insertLeftItem( 0, ci );
    break;
    case AsFirstRightItem:
      row->insertRightItem( 0, ci );
    break;
    default: // should never happen
      row->addRightItem( ci );
    break;
  }

  e->acceptProposedAction();

  mThemeSelectedContentItemRect = QRect();
  mDropIndicatorPoint1 = mDropIndicatorPoint2;
  mSelectedThemeContentItem = 0;

  setTheme( mTheme ); // this will reset theme cache and trigger a global update
}

bool ThemePreviewWidget::computeContentItemInsertPosition( const QPoint &pos, Theme::ContentItem::Type type )
{
  mDropIndicatorPoint1 = mDropIndicatorPoint2; // this marks the position as invalid

  if ( !mDelegate->hitTest( pos, false ) )
    return false;

  if ( !mDelegate->hitRow() )
    return false;

  if ( mDelegate->hitRowIsMessageRow() )
  {
    if ( !Theme::ContentItem::applicableToMessageItems( type ) )
      return false;
  } else {
    if ( !Theme::ContentItem::applicableToGroupHeaderItems( type ) )
      return false;
  }

  QRect rowRect = mDelegate->hitRowRect();

  if ( pos.y() < rowRect.top() + 3 )
  {
    // above a row
    mRowInsertPosition = AboveRow;
    if ( pos.x() < ( rowRect.left() + ( rowRect.width() / 2 ) ) )
    {
      mDropIndicatorPoint1 = rowRect.topLeft();
      mItemInsertPosition = AsLastLeftItem;
    } else {
      mDropIndicatorPoint1 = rowRect.topRight();
      mItemInsertPosition = AsLastRightItem;
    }
    mDropIndicatorPoint2 = QPoint( rowRect.left() + ( rowRect.width() / 2 ), rowRect.top() );
    return true;
  }

  if ( pos.y() > rowRect.bottom() - 3 )
  {
    // below a row
    mRowInsertPosition = BelowRow;
    if ( pos.x() < ( rowRect.left() + ( rowRect.width() / 2 ) ) )
    {
      mDropIndicatorPoint1 = rowRect.bottomLeft();
      mItemInsertPosition = AsLastLeftItem;
    } else {
      mDropIndicatorPoint1 = rowRect.bottomRight();
      mItemInsertPosition = AsLastRightItem;
    }
    mDropIndicatorPoint2 = QPoint( rowRect.left() + ( rowRect.width() / 2 ), rowRect.bottom() );
    return true;
  }

  mRowInsertPosition = InsideRow;

  if ( !mDelegate->hitContentItem() )
  {
    // didn't hit anything... probably no items in the row
    if ( pos.x() < ( rowRect.left() + ( rowRect.width() / 2 ) ) )
    {
      mItemInsertPosition =  AsLastLeftItem;
      mDropIndicatorPoint1 = QPoint( rowRect.left(), rowRect.top() );
      mDropIndicatorPoint2 = QPoint( rowRect.left(), rowRect.bottom() );
    } else {
      mItemInsertPosition =  AsLastRightItem;
      mDropIndicatorPoint1 = QPoint( rowRect.right(), rowRect.top() );
      mDropIndicatorPoint2 = QPoint( rowRect.right(), rowRect.bottom() );
    }
    return true;
  }

  // hit something, maybe inexactly
  QRect itemRect = mDelegate->hitContentItemRect();

  if ( !itemRect.contains( pos ) )
  {
    // inexact hit: outside an item
    if ( pos.x() > itemRect.right() )
    {
      // right side of an item
      if ( mDelegate->hitRow()->rightItems().count() < 1 )
      {
        // between the last left item and the right side
        if ( pos.x() > ( itemRect.right() + ( ( rowRect.right() - itemRect.right() ) / 2 ) ) )
        {
          // first/last right item
          mItemInsertPosition = AsFirstRightItem;
          mDropIndicatorPoint1 = rowRect.topRight();
          mDropIndicatorPoint2 = rowRect.bottomRight();
        }
        return true;
      }
      // either there were some right items (so the theme delegate knows that the reported item is the closest)
      // or there were no right items but the position is closest to the left item than the right row end
      mItemInsertPosition = OnRightOfItem;
      mDropIndicatorPoint1 = itemRect.topRight();
      mDropIndicatorPoint2 = itemRect.bottomRight();
      return true;     
    }

    // left side of an item
    if ( mDelegate->hitRow()->leftItems().count() < 1 )
    {
      // between the left side and the leftmost right item
      if ( pos.x() < ( itemRect.left() - ( ( itemRect.left() - rowRect.left() ) / 2 ) ) )
      {
        mItemInsertPosition = AsFirstLeftItem;
        mDropIndicatorPoint1 = rowRect.topLeft();
        mDropIndicatorPoint2 = rowRect.bottomLeft();
        return true;
      }
    }
    mItemInsertPosition = OnLeftOfItem;
    mDropIndicatorPoint1 = itemRect.topLeft();
    mDropIndicatorPoint2 = itemRect.bottomLeft();
    return true;
  }
  
  // exact hit
  if ( pos.x() < ( itemRect.left() + ( itemRect.width() / 2 ) ) )
  {
    // left side
    mItemInsertPosition = OnLeftOfItem;
    mDropIndicatorPoint1 = itemRect.topLeft();
    mDropIndicatorPoint2 = itemRect.bottomLeft();
    return true;   
  }

  // right side
  mItemInsertPosition = OnRightOfItem;
  mDropIndicatorPoint1 = itemRect.topRight();
  mDropIndicatorPoint2 = itemRect.bottomRight();
  return true;
}

void ThemePreviewWidget::mouseMoveEvent( QMouseEvent * e )
{
  if ( ! ( mSelectedThemeContentItem && ( e->buttons() & Qt::LeftButton ) ) )
  {
    QTreeWidget::mouseMoveEvent( e );
    return;
  }

  if ( mSelectedThemeContentItem != mDelegate->hitContentItem() )
  {
    QTreeWidget::mouseMoveEvent( e );
    return; // ugh.. something weird happened
  }

  // starting a drag ?
  QPoint diff = e->pos() - mMouseDownPoint;
  if ( diff.manhattanLength() <= 4 )
  {
    QTreeWidget::mouseMoveEvent( e );
    return; // ugh.. something weird happened
  }

  // startin a drag
  QMimeData * data = new QMimeData();
  QByteArray arry;
  arry.resize( sizeof( Theme::ContentItem::Type ) );
  *( ( Theme::ContentItem::Type * ) arry.data() ) = mSelectedThemeContentItem->type();
  data->setData( gThemeContentItemTypeDndMimeDataFormat, arry );
  QDrag * drag = new QDrag( this );
  drag->setMimeData( data );

  // remove the Theme::ContentItem from the Theme
  if ( mDelegate->hitContentItemRight() )
    const_cast< Theme::Row * >( mDelegate->hitRow() )->removeRightItem( mSelectedThemeContentItem );
  else
    const_cast< Theme::Row * >( mDelegate->hitRow() )->removeLeftItem( mSelectedThemeContentItem );

  delete mSelectedThemeContentItem;

  if ( mDelegate->hitRow()->rightItems().isEmpty() && mDelegate->hitRow()->leftItems().isEmpty() )
  {
    if ( mDelegate->hitItem()->type() == Item::Message )
    {
      if ( mDelegate->hitColumn()->messageRows().count() > 1 )
      {
        const_cast< Theme::Column * >( mDelegate->hitColumn() )->removeMessageRow( const_cast< Theme::Row * >( mDelegate->hitRow() ) );
        delete mDelegate->hitRow();
      }
    } else {
      if ( mDelegate->hitColumn()->groupHeaderRows().count() > 1 )
      {
        const_cast< Theme::Column * >( mDelegate->hitColumn() )->removeGroupHeaderRow( const_cast< Theme::Row * >( mDelegate->hitRow() ) );
        delete mDelegate->hitRow();
      }
    }
  }

  mSelectedThemeContentItem = 0;
  mThemeSelectedContentItemRect = QRect();
  mDropIndicatorPoint1 = mDropIndicatorPoint2;

  setTheme( mTheme ); // this will reset theme cache and trigger a global update

  // and do drag
  drag->exec( Qt::CopyAction, Qt::CopyAction );
}

void ThemePreviewWidget::mousePressEvent( QMouseEvent * e )
{
  mMouseDownPoint = e->pos();

  if ( mDelegate->hitTest( mMouseDownPoint ) )
  {
    mSelectedThemeContentItem = const_cast< Theme::ContentItem * >( mDelegate->hitContentItem() );
    mThemeSelectedContentItemRect = mSelectedThemeContentItem ? mDelegate->hitContentItemRect() : QRect();
  } else {
    mSelectedThemeContentItem = 0;
    mThemeSelectedContentItemRect = QRect();
  }

  QTreeWidget::mousePressEvent( e );
  viewport()->update();

  if ( e->button() == Qt::RightButton )
  {
    KMenu menu;  

    if ( mSelectedThemeContentItem )
    {

      menu.addTitle( Theme::ContentItem::description( mSelectedThemeContentItem->type() ) );

      if ( mSelectedThemeContentItem->displaysText() )
      {
        QAction * act;
        act = menu.addAction( i18nc( "@action:inmenu soften the text color", "Soften" ) );
        act->setCheckable( true );
        act->setChecked( mSelectedThemeContentItem->softenByBlending() );
        connect( act, SIGNAL( triggered( bool ) ),
                 SLOT( slotSoftenActionTriggered( bool ) ) );

        KMenu * childmenu = new KMenu( &menu );

        QActionGroup * grp = new QActionGroup( childmenu );

        act = childmenu->addAction( i18nc("@action:inmenu Font setting", "Default") );
        act->setData( QVariant( static_cast< int >( 0 ) ) );
        act->setCheckable( true );
        act->setChecked( !mSelectedThemeContentItem->useCustomFont() );
        grp->addAction( act );
        act = childmenu->addAction( i18nc("@action:inmenu Font setting", "Custom...") );
        act->setData( QVariant( static_cast< int >( Theme::ContentItem::UseCustomFont ) ) );
        act->setCheckable( true );
        act->setChecked( mSelectedThemeContentItem->useCustomFont() );
        grp->addAction( act );

        // We would like the group to be exclusive, but then the "Custom..." action
        // will not be triggered if activated multiple times in a row... well, we'll have to live with checkboxes instead of radios.
        grp->setExclusive( false );

        connect( childmenu, SIGNAL( triggered( QAction * ) ),
                 SLOT( slotFontMenuTriggered( QAction * ) ) );

        menu.addMenu( childmenu )->setText( i18n( "Font" ) );
      }

      if ( mSelectedThemeContentItem->canUseCustomColor() )
      {
        KMenu * childmenu = new KMenu( &menu );

        QActionGroup * grp = new QActionGroup( childmenu );

        QAction * act;
        act = childmenu->addAction( i18nc("@action:inmenu Foreground color setting", "Default") );
        act->setData( QVariant( static_cast< int >( 0 ) ) );
        act->setCheckable( true );
        act->setChecked( !mSelectedThemeContentItem->useCustomColor() );
        grp->addAction( act );
        act = childmenu->addAction( i18nc("@action:inmenu Foreground color setting", "Custom...") );
        act->setData( QVariant( static_cast< int >( Theme::ContentItem::UseCustomColor ) ) );
        act->setCheckable( true );
        act->setChecked( mSelectedThemeContentItem->useCustomColor() );
        grp->addAction( act );

        // We would like the group to be exclusive, but then the "Custom..." action
        // will not be triggered if activated multiple times in a row... well, we'll have to live with checkboxes instead of radios.
        grp->setExclusive( false );

        connect( childmenu, SIGNAL( triggered( QAction * ) ),
                 SLOT( slotForegroundColorMenuTriggered( QAction * ) ) );

        menu.addMenu( childmenu )->setText( i18n( "Foreground Color" ) );
      }


      if ( mSelectedThemeContentItem->canBeDisabled() )
      {
        KMenu * childmenu = new KMenu( &menu );

        QActionGroup * grp = new QActionGroup( childmenu );

        QAction * act;
        act = childmenu->addAction( i18nc("Hide a mark if the mail does not have the attribute, e.g. Important mark on a non important mail", "Hide") );
        act->setData( QVariant( static_cast< int >( Theme::ContentItem::HideWhenDisabled ) ) );
        act->setCheckable( true );
        act->setChecked( mSelectedThemeContentItem->hideWhenDisabled() );
        grp->addAction( act );
        act = childmenu->addAction( i18nc("Keep a empty space in the list if the mail does not have the attribute, e.g. Important mark on a non important mail", "Keep Empty Space") );
        act->setData( QVariant( static_cast< int >( 0 ) ) );
        act->setCheckable( true );
        act->setChecked( ! ( mSelectedThemeContentItem->softenByBlendingWhenDisabled() || mSelectedThemeContentItem->hideWhenDisabled() ) );
        grp->addAction( act );
        act = childmenu->addAction( i18nc("Show the icon softened in the list if the mail does not have the attribute, e.g. Important mark on a non important mail", "Keep Softened Icon") );
        act->setData( QVariant( static_cast< int >( Theme::ContentItem::SoftenByBlendingWhenDisabled ) ) );
        act->setCheckable( true );
        act->setChecked( mSelectedThemeContentItem->softenByBlendingWhenDisabled() );
        grp->addAction( act );

        connect( childmenu, SIGNAL( triggered( QAction * ) ),
                 SLOT( slotDisabledFlagsMenuTriggered( QAction * ) ) );

        menu.addMenu( childmenu )->setText( i18n( "When Disabled" ) );
      }

    }

    if ( mDelegate->hitItem() )
    {
      if ( mDelegate->hitItem()->type() == Item::GroupHeader )
      {
        menu.addTitle( i18n( "Group Header" ) );

        // Background color (mode) submenu
        KMenu * childmenu = new KMenu( &menu );

        QActionGroup * grp = new QActionGroup( childmenu );

        QAction * act;
        act = childmenu->addAction( i18nc("@action:inmenu Group header background color setting", "None") );
        act->setData( QVariant( static_cast< int >( Theme::Transparent ) ) );
        act->setCheckable( true );
        act->setChecked( mTheme->groupHeaderBackgroundMode() == Theme::Transparent );
        grp->addAction( act );
        act = childmenu->addAction( i18nc("@action:inmenu Group header background color setting", "Automatic") );
        act->setData( QVariant( static_cast< int >( Theme::AutoColor ) ) );
        act->setCheckable( true );
        act->setChecked( mTheme->groupHeaderBackgroundMode() == Theme::AutoColor );
        grp->addAction( act );
        act = childmenu->addAction( i18nc("@action:inmenu Group header background color setting", "Custom...") );
        act->setData( QVariant( static_cast< int >( Theme::CustomColor ) ) );
        act->setCheckable( true );
        act->setChecked( mTheme->groupHeaderBackgroundMode() == Theme::CustomColor );
        grp->addAction( act );

        // We would like the group to be exclusive, but then the "Custom..." action
        // will not be triggered if activated multiple times in a row... well, we'll have to live with checkboxes instead of radios.
        grp->setExclusive( false );

        connect( childmenu, SIGNAL( triggered( QAction * ) ),
                 SLOT( slotGroupHeaderBackgroundModeMenuTriggered( QAction * ) ) );

        menu.addMenu( childmenu )->setText( i18n( "Background Color" ) );

        // Background style submenu
        childmenu = new KMenu( &menu );

        grp = new QActionGroup( childmenu );

        QList< QPair< QString, int > > styles = Theme::enumerateGroupHeaderBackgroundStyles();

        for ( QList< QPair< QString, int > >::ConstIterator it = styles.begin(); it != styles.end(); ++it )
        {
          act = childmenu->addAction( ( *it ).first );
          act->setData( QVariant( ( *it ).second ) );
          act->setCheckable( true );
          act->setChecked( mTheme->groupHeaderBackgroundStyle() == static_cast< Theme::GroupHeaderBackgroundStyle >( ( *it ).second ) );
          grp->addAction( act );
        }

        connect( childmenu, SIGNAL( triggered( QAction * ) ),
                 SLOT( slotGroupHeaderBackgroundStyleMenuTriggered( QAction * ) ) );

        act = menu.addMenu( childmenu );
        act->setText( i18n( "Background Style" ) );
        if ( mTheme->groupHeaderBackgroundMode() == Theme::Transparent )
          act->setEnabled( false );

      }
    }


    if ( menu.isEmpty() )
      return;

    menu.exec( viewport()->mapToGlobal( e->pos() ) );

  }
}

void ThemePreviewWidget::slotDisabledFlagsMenuTriggered( QAction * act )
{
  if ( !mSelectedThemeContentItem )
    return;

  bool ok;
  int flags = act->data().toInt( &ok );
  if ( !ok )
    return;

  mSelectedThemeContentItem->setHideWhenDisabled( flags == Theme::ContentItem::HideWhenDisabled );
  mSelectedThemeContentItem->setSoftenByBlendingWhenDisabled( flags == Theme::ContentItem::SoftenByBlendingWhenDisabled );

  setTheme( mTheme ); // this will reset theme cache and trigger a global update
}

void ThemePreviewWidget::slotForegroundColorMenuTriggered( QAction * act )
{
  if ( !mSelectedThemeContentItem )
    return;

  bool ok;
  int flag = act->data().toInt( &ok );
  if ( !ok )
    return;

  if ( flag == 0 )
  {
    mSelectedThemeContentItem->setUseCustomColor( false );
    setTheme( mTheme ); // this will reset theme cache and trigger a global update
    return;
  }

  QColor clr = QColorDialog::getColor( mSelectedThemeContentItem->customColor(), this );
  if ( !clr.isValid() )
    return;

  mSelectedThemeContentItem->setCustomColor( clr );
  mSelectedThemeContentItem->setUseCustomColor( true );

  setTheme( mTheme ); // this will reset theme cache and trigger a global update
}

void ThemePreviewWidget::slotSoftenActionTriggered( bool )
{
  if ( !mSelectedThemeContentItem )
    return;

  mSelectedThemeContentItem->setSoftenByBlending( !mSelectedThemeContentItem->softenByBlending() );
  setTheme( mTheme ); // this will reset theme cache and trigger a global update  
}

void ThemePreviewWidget::slotFontMenuTriggered( QAction * act )
{
  if ( !mSelectedThemeContentItem )
    return;

  bool ok;
  int flag = act->data().toInt( &ok );
  if ( !ok )
    return;

  if ( flag == 0 )
  {
    mSelectedThemeContentItem->setUseCustomFont( false );
    setTheme( mTheme ); // this will reset theme cache and trigger a global update
    return;
  }

  QFont f = mSelectedThemeContentItem->font();
  if ( KFontDialog::getFont( f ) != KFontDialog::Accepted )
    return;

  mSelectedThemeContentItem->setFont( f );
  mSelectedThemeContentItem->setUseCustomFont( true );

  setTheme( mTheme ); // this will reset theme cache and trigger a global update
}

void ThemePreviewWidget::slotGroupHeaderBackgroundModeMenuTriggered( QAction * act )
{
  bool ok;
  Theme::GroupHeaderBackgroundMode mode = static_cast< Theme::GroupHeaderBackgroundMode >( act->data().toInt( &ok ) );
  if ( !ok )
    return;

  switch( mode )
  {
    case Theme::Transparent:
      mTheme->setGroupHeaderBackgroundMode( Theme::Transparent );
    break;
    case Theme::AutoColor:
      mTheme->setGroupHeaderBackgroundMode( Theme::AutoColor );
    break;
    case Theme::CustomColor:
    {
      QColor clr = QColorDialog::getColor( mTheme->groupHeaderBackgroundColor(), this );
      if ( !clr.isValid() )
        return;
      mTheme->setGroupHeaderBackgroundMode( Theme::CustomColor );
      mTheme->setGroupHeaderBackgroundColor( clr );
    } 
    break;
  }
  
  setTheme( mTheme ); // this will reset theme cache and trigger a global update
}

void ThemePreviewWidget::slotGroupHeaderBackgroundStyleMenuTriggered( QAction * act )
{
  bool ok;
  Theme::GroupHeaderBackgroundStyle mode = static_cast< Theme::GroupHeaderBackgroundStyle >( act->data().toInt( &ok ) );
  if ( !ok )
    return;

  mTheme->setGroupHeaderBackgroundStyle( mode );
  
  setTheme( mTheme ); // this will reset theme cache and trigger a global update
}


void ThemePreviewWidget::paintEvent( QPaintEvent * e )
{
  QTreeWidget::paintEvent( e );

  if ( 
       mThemeSelectedContentItemRect.isValid() ||
       ( mDropIndicatorPoint1 != mDropIndicatorPoint2 ) 
    )
  {
    QPainter painter( viewport() );

    if ( mThemeSelectedContentItemRect.isValid() )
    {
      painter.setPen( QPen( Qt::black ) );
      painter.drawRect( mThemeSelectedContentItemRect );
    }
    if ( mDropIndicatorPoint1 != mDropIndicatorPoint2 ) 
    {
      painter.setPen( QPen( Qt::black, 3 ) );
      painter.drawLine( mDropIndicatorPoint1, mDropIndicatorPoint2 );
    }
  }
}



void ThemePreviewWidget::slotHeaderContextMenuRequested( const QPoint &pos )
{
  QTreeWidgetItem * hitem = headerItem();
  if ( !hitem )
    return; // ooops

  int col = header()->logicalIndexAt( pos );

  if ( col < 0 )
    return;

  if ( col >= mTheme->columns().count() )
    return;

  mSelectedThemeColumn = mTheme->column( col );
  if ( !mSelectedThemeColumn )
    return;

  KMenu menu;

  menu.addTitle( mSelectedThemeColumn->label() );

  QAction * act;

  act = menu.addAction( i18n( "Column Properties..." ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotColumnProperties() ) );

  act = menu.addAction( i18n( "Add Column..." ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotAddColumn() ) );

  act = menu.addAction( i18n( "Delete Column" ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotDeleteColumn() ) );
  act->setEnabled( col > 0 );   

  menu.exec( header()->mapToGlobal( pos ) );
}

void ThemePreviewWidget::slotAddColumn()
{
  int newColumnIndex = mTheme->columns().count();

  if ( mSelectedThemeColumn )
  {
    newColumnIndex = mTheme->columns().indexOf( mSelectedThemeColumn );
    if ( newColumnIndex < 0 )
      newColumnIndex = mTheme->columns().count();
    else
      newColumnIndex++;
  }
  
  mSelectedThemeColumn = new Theme::Column();
  mSelectedThemeColumn->setLabel( i18n( "New Column" ) );
  mSelectedThemeColumn->setVisibleByDefault( true );

  mSelectedThemeColumn->addMessageRow( new Theme::Row() );
  mSelectedThemeColumn->addGroupHeaderRow( new Theme::Row() );

  ThemeColumnPropertiesDialog * dlg = new ThemeColumnPropertiesDialog( this, mSelectedThemeColumn, i18n( "Add New Column" ) );

  if ( dlg->exec() == QDialog::Accepted )
  {
    mTheme->insertColumn( newColumnIndex, mSelectedThemeColumn );

    mSelectedThemeContentItem = 0;
    mThemeSelectedContentItemRect = QRect();
    mDropIndicatorPoint1 = mDropIndicatorPoint2;

    setTheme( mTheme ); // this will reset theme cache and trigger a global update

  } else {

    delete mSelectedThemeColumn;
    mSelectedThemeColumn = 0;
  }

  delete dlg;
}

void ThemePreviewWidget::slotColumnProperties()
{
  if ( !mSelectedThemeColumn )
    return;

  ThemeColumnPropertiesDialog * dlg = new ThemeColumnPropertiesDialog( this, mSelectedThemeColumn, i18n( "Column Properties" ) );

  if ( dlg->exec() == QDialog::Accepted )
  {
    mSelectedThemeContentItem = 0;
    mThemeSelectedContentItemRect = QRect();
    mDropIndicatorPoint1 = mDropIndicatorPoint2;

    setTheme( mTheme ); // this will reset theme cache and trigger a global update
  }

  delete dlg;
}

void ThemePreviewWidget::slotDeleteColumn()
{
  if ( !mSelectedThemeColumn )
    return;

  int idx = mTheme->columns().indexOf( mSelectedThemeColumn );
  if ( idx < 1 ) // first column can't be deleted
    return;

  mTheme->removeColumn( mSelectedThemeColumn );
  delete mSelectedThemeColumn;
  mSelectedThemeColumn = 0;

  mSelectedThemeContentItem = 0;
  mThemeSelectedContentItemRect = QRect();
  mDropIndicatorPoint1 = mDropIndicatorPoint2;

  setTheme( mTheme ); // this will reset theme cache and trigger a global update  
}





ThemeEditor::ThemeEditor( QWidget *parent )
  : OptionSetEditor( parent )
{
  mCurrentTheme = 0;

  // Appearance tab
  QWidget * tab = new QWidget( this );
  addTab( tab, i18n( "Appearance" ) );

  QGridLayout * tabg = new QGridLayout( tab );

  QGroupBox * gb = new QGroupBox( i18n( "Content Items" ), tab );
  tabg->addWidget( gb, 0, 0 );

  QGridLayout * gblayout = new QGridLayout( gb );

  ThemeContentItemSourceLabel * cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::Subject );
  cil->setText( Theme::ContentItem::description( cil->type() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 0, 0 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::Date );
  cil->setText( Theme::ContentItem::description( cil->type() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 1, 0 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::Size );
  cil->setText( Theme::ContentItem::description( cil->type() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 2, 0 );


  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::Sender );
  cil->setText( Theme::ContentItem::description( cil->type() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 0, 1 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::Receiver );
  cil->setText( Theme::ContentItem::description( cil->type() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 1, 1 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::SenderOrReceiver );
  cil->setText( Theme::ContentItem::description( cil->type() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 2, 1 );



  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::MostRecentDate );
  cil->setText( Theme::ContentItem::description( cil->type() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 0, 2 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::TagList );
  cil->setText( Theme::ContentItem::description( cil->type() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 1, 2 );



  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::CombinedReadRepliedStateIcon );
  cil->setPixmap( *( Manager::instance()->pixmapMessageRepliedAndForwarded() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 0, 3 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::ReadStateIcon );
  cil->setPixmap( *( Manager::instance()->pixmapMessageNew() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 1, 3 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::RepliedStateIcon );
  cil->setPixmap( *( Manager::instance()->pixmapMessageReplied() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 2, 3 );



  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::AttachmentStateIcon );
  cil->setPixmap( *( Manager::instance()->pixmapMessageAttachment() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 0, 4 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::EncryptionStateIcon );
  cil->setPixmap( *( Manager::instance()->pixmapMessageFullyEncrypted() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 1, 4 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::SignatureStateIcon );
  cil->setPixmap( *( Manager::instance()->pixmapMessageFullySigned() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 2, 4 );



  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::ActionItemStateIcon );
  cil->setPixmap( *( Manager::instance()->pixmapMessageActionItem() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 0, 5 );




  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::ImportantStateIcon );
  cil->setPixmap( *( Manager::instance()->pixmapMessageImportant() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 0, 6 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::SpamHamStateIcon );
  cil->setPixmap( *( Manager::instance()->pixmapMessageSpam() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 1, 6 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::WatchedIgnoredStateIcon );
  cil->setPixmap( *( Manager::instance()->pixmapMessageWatched() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 2, 6 );





  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::ExpandedStateIcon );
  cil->setPixmap( *( Manager::instance()->pixmapShowMore() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 0, 7 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::VerticalLine );
  cil->setPixmap( *( Manager::instance()->pixmapVerticalLine() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 1, 7 );

  cil = new ThemeContentItemSourceLabel( gb, Theme::ContentItem::HorizontalSpacer );
  cil->setPixmap( *( Manager::instance()->pixmapHorizontalSpacer() ) );
  cil->setToolTip( Theme::ContentItem::description( cil->type() ) );
  gblayout->addWidget( cil, 2, 7 );



  mPreviewWidget = new ThemePreviewWidget( tab );
  tabg->addWidget( mPreviewWidget, 1, 0 );

  QLabel * l = new QLabel( tab );
  l->setText( i18n( "Right click on the header to add or modify columns. Drag the content items and drop them on the columns in order to compose your theme. Right click on the items inside the view for more options." ) );
  l->setWordWrap( true );
  l->setAlignment( Qt::AlignCenter );
  tabg->addWidget( l, 2, 0 );

  tabg->setRowStretch( 1, 1 );

  // Advanced tab
  tab = new QWidget( this );
  addTab( tab, i18nc( "@title:tab Advanced theme settings", "Advanced" ) );

  tabg = new QGridLayout( tab );

  l = new QLabel( i18n( "Header:" ), tab );
  tabg->addWidget( l, 0, 0 );

  mViewHeaderPolicyCombo = new QComboBox( tab );
  tabg->addWidget( mViewHeaderPolicyCombo, 0, 1 );

  tabg->setColumnStretch( 1, 1 );
  tabg->setRowStretch( 1, 1 );

}

ThemeEditor::~ThemeEditor()
{
}

void ThemeEditor::editTheme( Theme *set )
{
  mCurrentTheme = set;

  if ( !mCurrentTheme )
  {
    setEnabled( false );
    return;
  }

  setEnabled( true );

  nameEdit()->setText( set->name() );
  descriptionEdit()->setPlainText( set->description() );

  mPreviewWidget->setTheme( set );

  fillViewHeaderPolicyCombo();
  ComboBoxUtils::setIntegerOptionComboValue( mViewHeaderPolicyCombo, (int)mCurrentTheme->viewHeaderPolicy() );

}

void ThemeEditor::commit()
{
  if ( !mCurrentTheme )
    return;

  mCurrentTheme->setName( nameEdit()->text() );
  mCurrentTheme->setDescription( descriptionEdit()->toPlainText() );

  mCurrentTheme->setViewHeaderPolicy(
      (Theme::ViewHeaderPolicy)ComboBoxUtils::getIntegerOptionComboValue( mViewHeaderPolicyCombo, 0 )
    );
  // other settings are already committed to this theme
}

void ThemeEditor::fillViewHeaderPolicyCombo()
{
  ComboBoxUtils::fillIntegerOptionCombo(
      mViewHeaderPolicyCombo,
      Theme::enumerateViewHeaderPolicyOptions()
    );
}

void ThemeEditor::slotNameEditTextEdited( const QString &newName )
{
  if( !mCurrentTheme )
    return;
  mCurrentTheme->setName( newName );
  emit themeNameChanged();
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail

