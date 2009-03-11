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

#include "messagelistview/core/themedelegate.h"
#include "messagelistview/core/messageitem.h"
#include "messagelistview/core/groupheaderitem.h"
#include "messagelistview/core/manager.h"

#include <QStyle>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QAbstractItemView>
#include <QPixmap>
#include <QLinearGradient>
#include <KColorScheme>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

static const int gGroupHeaderOuterVerticalMargin = 1;
static const int gGroupHeaderOuterHorizontalMargin = 1;
static const int gGroupHeaderInnerVerticalMargin = 1;
static const int gGroupHeaderInnerHorizontalMargin = 1;
static const int gMessageVerticalMargin = 2;
static const int gMessageHorizontalMargin = 2;
static const int gHorizontalItemSpacing = 2;


ThemeDelegate::ThemeDelegate( QAbstractItemView * parent, QPaintDevice * paintDevice )
  : QStyledItemDelegate( parent )
{
  mItemView = parent;
  mPaintDevice = paintDevice;
  mTheme = 0;
}

ThemeDelegate::~ThemeDelegate()
{
}

void ThemeDelegate::setTheme( const Theme * theme )
{
  mTheme = theme;

  if ( !mTheme )
    return; // hum

  // yep..we're violating const here
  // But most of the QStyledItemDelegate virtual methods are const and expect
  // const behaviour. So we need the const pointer to avoid compiler complains
  // but on the other side we're NOT really const... well...
  const_cast< Theme * >( mTheme )->resetCache();

  // Rebuild the group header background color cache
  switch( mTheme->groupHeaderBackgroundMode() )
  {
    case Theme::Transparent:
      mGroupHeaderBackgroundColor = QColor(); // invalid
    break;
    case Theme::CustomColor:
      mGroupHeaderBackgroundColor = mTheme->groupHeaderBackgroundColor();
    break;
    case Theme::AutoColor:
    {
      QPalette pal = mItemView->palette();
      QColor txt = pal.color( QPalette::Normal, QPalette::Text );
      QColor bck = pal.color( QPalette::Normal, QPalette::Base );
      mGroupHeaderBackgroundColor = QColor(
          ( txt.red() + ( bck.red() * 3 ) ) / 4,
          ( txt.green() + ( bck.green() * 3 ) ) / 4,
          ( txt.blue() + ( bck.blue() * 3 ) ) / 4
        );
    }
    break;
  }
  mItemView->reset();
}

// FIXME: gcc will refuse to inline these functions loudly complaining
//        about function growth limit reached. Consider using macros
//        or just convert to member functions.

static inline void paint_right_aligned_elided_text( const QString &text, QPaintDevice * pd, Theme::ContentItem * ci, QPainter * painter, int &left, int top, int &right, Qt::LayoutDirection layoutDir )
{
  if ( ci->lastPaintDevice() != pd )
    ci->updateFontMetrics( pd );
  painter->setFont( ci->font() );
  int w = right - left;
  QString elidedText = ci->fontMetrics().elidedText( text, layoutDir == Qt::LeftToRight ? Qt::ElideLeft : Qt::ElideRight, w );
  QRect rct( left, top, w, ci->lineSpacing() );
  QRect outRct;

  if ( ci->softenByBlending() )
  {
    qreal oldOpacity = painter->opacity();
    painter->setOpacity( 0.6 );
    painter->drawText( rct, Qt::AlignTop | Qt::AlignRight | Qt::TextSingleLine, elidedText, &outRct );
    painter->setOpacity( oldOpacity );
  } else {
    painter->drawText( rct, Qt::AlignTop | Qt::AlignRight | Qt::TextSingleLine, elidedText, &outRct );
  }
  if ( layoutDir == Qt::LeftToRight )
    right -= outRct.width() + gHorizontalItemSpacing;
  else
    left += outRct.width() + gHorizontalItemSpacing;
}

static inline void compute_bounding_rect_for_right_aligned_elided_text( const QString &text, QPaintDevice * pd, Theme::ContentItem * ci, int &left, int top, int &right, QRect &outRect, Qt::LayoutDirection layoutDir )
{
  if ( ci->lastPaintDevice() != pd )
    ci->updateFontMetrics( pd );
  int w = right - left;
  QString elidedText = ci->fontMetrics().elidedText( text, layoutDir == Qt::LeftToRight ? Qt::ElideLeft : Qt::ElideRight, w );
  QRect rct( left, top, w, ci->lineSpacing() );
  Qt::AlignmentFlag af = layoutDir == Qt::LeftToRight ? Qt::AlignRight : Qt::AlignLeft;
  outRect = ci->fontMetrics().boundingRect( rct, Qt::AlignTop | af | Qt::TextSingleLine, elidedText );
  if ( layoutDir == Qt::LeftToRight )
    right -= outRect.width() + gHorizontalItemSpacing;
  else
    left += outRect.width() + gHorizontalItemSpacing;
}


static inline void paint_left_aligned_elided_text( const QString &text, QPaintDevice * pd, Theme::ContentItem * ci, QPainter * painter, int &left, int top, int &right, Qt::LayoutDirection layoutDir )
{
  if ( ci->lastPaintDevice() != pd )
    ci->updateFontMetrics( pd );
  painter->setFont( ci->font() );
  int w = right - left;
  QString elidedText = ci->fontMetrics().elidedText( text, layoutDir == Qt::LeftToRight ? Qt::ElideRight : Qt::ElideLeft, w );
  QRect rct( left, top, w, ci->lineSpacing() );
  QRect outRct;
  if ( ci->softenByBlending() )
  {
    qreal oldOpacity = painter->opacity();
    painter->setOpacity( 0.6 );
    painter->drawText( rct, Qt::AlignTop | Qt::AlignLeft | Qt::TextSingleLine, elidedText, &outRct );
    painter->setOpacity( oldOpacity );
  } else {
    painter->drawText( rct, Qt::AlignTop | Qt::AlignLeft | Qt::TextSingleLine, elidedText, &outRct );
  }
  if ( layoutDir == Qt::LeftToRight )
    left += outRct.width() + gHorizontalItemSpacing;
  else
    right -= outRct.width() + gHorizontalItemSpacing;
}

static inline void compute_bounding_rect_for_left_aligned_elided_text( const QString &text, QPaintDevice * pd, Theme::ContentItem * ci, int &left, int top, int &right, QRect &outRect, Qt::LayoutDirection layoutDir )
{
  if ( ci->lastPaintDevice() != pd )
    ci->updateFontMetrics( pd );
  int w = right - left;
  QString elidedText = ci->fontMetrics().elidedText( text, layoutDir == Qt::LeftToRight ? Qt::ElideRight : Qt::ElideLeft, w );
  QRect rct( left, top, w, ci->lineSpacing() );
  Qt::AlignmentFlag af = layoutDir == Qt::LeftToRight ? Qt::AlignLeft : Qt::AlignRight;
  outRect = ci->fontMetrics().boundingRect( rct, Qt::AlignTop | af | Qt::TextSingleLine, elidedText );
  if ( layoutDir == Qt::LeftToRight )
    left += outRect.width() + gHorizontalItemSpacing;
  else
    right -= outRect.width() + gHorizontalItemSpacing;
}

static inline const QPixmap * get_read_state_icon( Item * item )
{
  if ( item->status().isQueued() )
    return Manager::instance()->pixmapMessageQueued();
  if ( item->status().isSent() )
    return Manager::instance()->pixmapMessageSent();
  if ( item->status().isNew() )
    return Manager::instance()->pixmapMessageNew();
  if ( item->status().isRead() || item->status().isOld() )
    return Manager::instance()->pixmapMessageRead();
  if ( item->status().isUnread() )
    return Manager::instance()->pixmapMessageUnread();
  if ( item->status().isDeleted() )
    return Manager::instance()->pixmapMessageDeleted();

  // Uhm... should never happen.. but fallback to "read"...
  return Manager::instance()->pixmapMessageRead();
}

static inline const QPixmap * get_combined_read_replied_state_icon( MessageItem * messageItem )
{
  if ( messageItem->status().isReplied() )
  {
    if ( messageItem->status().isForwarded() )
      return Manager::instance()->pixmapMessageRepliedAndForwarded();
    return Manager::instance()->pixmapMessageReplied();
  }
  if ( messageItem->status().isForwarded() )
    return Manager::instance()->pixmapMessageForwarded();

  return get_read_state_icon( messageItem );
}

static inline const QPixmap * get_encryption_state_icon( MessageItem * messageItem, bool *treatAsEnabled )
{
  switch( messageItem->encryptionState() )
  {
    case MessageItem::FullyEncrypted:
      *treatAsEnabled = true;
      return Manager::instance()->pixmapMessageFullyEncrypted();
    break;
    case MessageItem::PartiallyEncrypted:
      *treatAsEnabled = true;
      return Manager::instance()->pixmapMessagePartiallyEncrypted();
    break;
    case MessageItem::EncryptionStateUnknown:
      *treatAsEnabled = false;
      return Manager::instance()->pixmapMessageUndefinedEncrypted();
    break;
    case MessageItem::NotEncrypted:
      *treatAsEnabled = false;
      return Manager::instance()->pixmapMessageNotEncrypted();
    break;
    default:
      // should never happen
      Q_ASSERT( false );
    break;
  }

  *treatAsEnabled = false;
  return Manager::instance()->pixmapMessageUndefinedEncrypted();
}

static inline const QPixmap * get_signature_state_icon( MessageItem * messageItem, bool *treatAsEnabled )
{
  switch( messageItem->signatureState() )
  {
    case MessageItem::FullySigned:
      *treatAsEnabled = true;
      return Manager::instance()->pixmapMessageFullySigned();
    break;
    case MessageItem::PartiallySigned:
      *treatAsEnabled = true;
      return Manager::instance()->pixmapMessagePartiallySigned();
    break;
    case MessageItem::SignatureStateUnknown:
      *treatAsEnabled = false;
      return Manager::instance()->pixmapMessageUndefinedSigned();
    break;
    case MessageItem::NotSigned:
      *treatAsEnabled = false;
      return Manager::instance()->pixmapMessageNotSigned();
    break;
    default:
      // should never happen
      Q_ASSERT( false );
    break;
  }

  *treatAsEnabled = false;
  return Manager::instance()->pixmapMessageUndefinedSigned();
}

static inline const QPixmap * get_replied_state_icon( MessageItem * messageItem )
{
  if ( messageItem->status().isReplied() )
  {
    if ( messageItem->status().isForwarded() )
      return Manager::instance()->pixmapMessageRepliedAndForwarded();
    return Manager::instance()->pixmapMessageReplied();
  }
  if ( messageItem->status().isForwarded() )
    return Manager::instance()->pixmapMessageForwarded();

  return 0;
}

static inline const QPixmap * get_spam_ham_state_icon( MessageItem * messageItem )
{
  if ( messageItem->status().isSpam() )
    return Manager::instance()->pixmapMessageSpam();
  if ( messageItem->status().isHam() )
    return Manager::instance()->pixmapMessageHam();
  return 0;
}

static inline const QPixmap * get_watched_ignored_state_icon( MessageItem * messageItem )
{
  if ( messageItem->status().isIgnored() )
    return Manager::instance()->pixmapMessageIgnored();
  if ( messageItem->status().isWatched() )
    return Manager::instance()->pixmapMessageWatched();
  return 0;
}

static inline void paint_vertical_line( QPainter * painter, int &left, int top, int &right, int bottom, bool alignOnRight )
{
  if ( alignOnRight )
  {
    right -= 1;
    if ( right < 0 )
      return;
    painter->drawLine( right, top, right, bottom );
    right -= 2;
    right -= gHorizontalItemSpacing;
  } else {
    left += 1;
    if ( left > right )
      return;
    painter->drawLine( left, top, left, bottom );
    left += 2 + gHorizontalItemSpacing;
  }
}

static inline void compute_bounding_rect_for_vertical_line( int &left, int top, int &right, int bottom, QRect &outRect, bool alignOnRight )
{
  if ( alignOnRight )
  {
    right -= 3;
    outRect = QRect( right, top, 3, bottom - top );
    right -= gHorizontalItemSpacing;
  } else {
    outRect = QRect( left, top, 3, bottom - top );
    left += 3 + gHorizontalItemSpacing;
  }
}

static inline void paint_horizontal_spacer( int &left, int, int &right, int, bool alignOnRight )
{
  if ( alignOnRight )
  {
    right -= 3 + gHorizontalItemSpacing;
  } else {
    left += 3 + gHorizontalItemSpacing;
  }
}

static inline void compute_bounding_rect_for_horizontal_spacer( int &left, int top, int &right, int bottom, QRect &outRect, bool alignOnRight )
{
  if ( alignOnRight )
  {
    right -= 3;
    outRect = QRect( right, top, 3, bottom - top );
    right -= gHorizontalItemSpacing;
  } else {
    outRect = QRect( left, top, 3, bottom - top );
    left += 3 + gHorizontalItemSpacing;
  }
}

static inline void paint_permanent_icon( const QPixmap * pix, Theme::ContentItem *,
                                         QPainter * painter, int &left, int top, int &right,
                                         bool alignOnRight, int iconSize )
{
  if ( alignOnRight )
  {
    right -= iconSize; // this icon is always present
    if ( right < 0 )
      return;
    painter->drawPixmap( right, top, iconSize, iconSize, *pix );
    right -= gHorizontalItemSpacing;
  } else {
    if ( left > right - iconSize )
      return;
    painter->drawPixmap( left, top, iconSize, iconSize, *pix );
    left += iconSize + gHorizontalItemSpacing;
  }
}

static inline void compute_bounding_rect_for_permanent_icon( Theme::ContentItem *, int &left,
                                                             int top, int &right,
                                                             QRect &outRect, bool alignOnRight,
                                                             int iconSize )
{
  if ( alignOnRight )
  {
    right -= iconSize; // this icon is always present
    outRect = QRect( right, top, iconSize, iconSize );
    right -= gHorizontalItemSpacing;
  } else {
    outRect = QRect( left, top, iconSize, iconSize );
    left += iconSize + gHorizontalItemSpacing;
  }
}

// Calculates the size for the icon, based on the size hint height.
// Icons will not be bigger than 16.
static inline int iconSizeFromSizeHint( int sizeHintHeight )
{
  int iconSize = sizeHintHeight - 2;
  if ( iconSize > 16 )
    iconSize = 16;
  return iconSize;
}

static inline void paint_boolean_state_icon( bool enabled, const QPixmap * pix,
                                             Theme::ContentItem * ci, QPainter * painter, int &left,
                                             int top, int &right, bool alignOnRight,
                                             int iconSize )
{
  if ( enabled )
  {
    paint_permanent_icon( pix, ci, painter, left, top, right, alignOnRight, iconSize );
    return;
  }

  // off -> icon disabled
  if ( ci->hideWhenDisabled() )
    return; // doesn't even take space

  if ( ci->softenByBlendingWhenDisabled() )
  {
    // still paint, but very soft
    qreal oldOpacity = painter->opacity();
    painter->setOpacity( 0.3 );
    paint_permanent_icon( pix, ci, painter, left, top, right, alignOnRight, iconSize );
    painter->setOpacity( oldOpacity );
    return;
  }

  // just takes space
  if ( alignOnRight )
    right -= iconSize + gHorizontalItemSpacing;
  else
    left += iconSize + gHorizontalItemSpacing;
}

static inline void compute_bounding_rect_for_boolean_state_icon( bool enabled, Theme::ContentItem * ci,
                                                                 int &left, int top, int &right,
                                                                 QRect &outRect, bool alignOnRight,
                                                                 int iconSize )
{
  if ( ( !enabled ) && ci->hideWhenDisabled() )
  {
    outRect = QRect();
    return; // doesn't even take space
  }

  compute_bounding_rect_for_permanent_icon( ci, left, top, right, outRect, alignOnRight, iconSize );
}

static inline void paint_tag_list( QList< MessageItem::Tag * > * tagList, QPainter * painter,
                                   int &left, int top, int &right, bool alignOnRight, int iconSize )
{
  if ( alignOnRight )
  {
    for ( QList< MessageItem::Tag * >::Iterator it = tagList->begin(); it != tagList->end(); ++it )
    {
      right -= iconSize; // this icon is always present
      if ( right < 0 )
        return;
      painter->drawPixmap( right, top, ( *it )->pixmap() );
      right -= gHorizontalItemSpacing;
    }
  } else {
    for ( QList< MessageItem::Tag * >::Iterator it = tagList->begin(); it != tagList->end(); ++it )
    {
      if ( left > right - iconSize )
        return;
      painter->drawPixmap( left, top, ( *it )->pixmap() );
      left += iconSize + gHorizontalItemSpacing;
    }
  }
}

static inline void compute_bounding_rect_for_tag_list( QList< MessageItem::Tag * > * tagList,
                                                       int &left, int top, int &right, QRect &outRect,
                                                       bool alignOnRight, int iconSize )
{
  int width = tagList->count() * ( iconSize + gHorizontalItemSpacing );
  if ( alignOnRight )
  {
    right -= width;
    outRect = QRect( right, top, width, iconSize );
  } else {
    outRect = QRect( left, top, width, iconSize );
    left += width;
  }
}

static inline void compute_size_hint_for_item( Theme::ContentItem * ci, QPaintDevice * pd,
                                               int &maxh, int &totalw, int iconSize )
{
  if ( ci->displaysText() )
  {
    if ( ci->lastPaintDevice() != pd )
      ci->updateFontMetrics( pd );
    if ( ci->lineSpacing() > maxh )
      maxh = ci->lineSpacing();
    totalw += ci->displaysLongText() ? 128 : 64;
    return;
  }

  if ( ci->isIcon() )
  {
    totalw += iconSize + gHorizontalItemSpacing;
    return;
  }

  if ( ci->isSpacer() )
  {
    if ( 18 > maxh )
      maxh = 18;
    totalw += 3 + gHorizontalItemSpacing;
    return;
  }

  // should never be reached
  if ( 18 > maxh )
    maxh = 18;
  totalw += gHorizontalItemSpacing;
}

static inline void compute_size_hint_for_row( const Theme::Row * r, QPaintDevice * pd, int iconSize )
{
  int maxh = 8; // at least 8 pixels for a pixmap
  int totalw = 0;

  // right aligned stuff first
  const QList< Theme::ContentItem * > * items = &( r->rightItems() );
  QList< Theme::ContentItem * >::ConstIterator itemit;

  for ( itemit = items->begin(); itemit != items->end() ; ++itemit )
    compute_size_hint_for_item( const_cast< Theme::ContentItem * >( *itemit ), pd, maxh, totalw, iconSize );

  // then left aligned stuff
  items = &( r->leftItems() );

  for ( itemit = items->begin(); itemit != items->end() ; ++itemit )
    compute_size_hint_for_item( const_cast< Theme::ContentItem * >( *itemit ), pd, maxh, totalw, iconSize );

  const_cast< Theme::Row * >( r )->setSizeHint( QSize( totalw, maxh ) );
}

void ThemeDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return; // bleah

  Item * item = itemFromIndex( index );
  if ( !item )
    return; // hm...

  QStyleOptionViewItemV4 opt = option;
  initStyleOption( &opt, index );

  opt.text.clear(); // draw no text for me, please.. I'll do it in a while

  QStyle * style = mItemView->style();
  style->drawControl( QStyle::CE_ItemViewItem, &opt, painter, mItemView );

  if ( !mTheme )
    return; // hm hm...

  const Theme::Column * skcolumn = mTheme->column( index.column() );
  if ( !skcolumn )
    return; // bleah
  int iconSize = iconSizeFromSizeHint( skcolumn->messageSizeHint().height() );

  const QList< Theme::Row * > * rows; // I'd like to have it as reference, but gcc complains...

  MessageItem * messageItem = 0;
  GroupHeaderItem * groupHeaderItem = 0;

  int top = opt.rect.top();
  int right = opt.rect.left() + opt.rect.width(); // don't use opt.rect.right() since it's screwed
  int left = opt.rect.left();

  // Storing the changed members one by one is faster than saving the painter state
  QFont oldFont = painter->font();
  QPen oldPen = painter->pen();
  qreal oldOpacity = painter->opacity();

  QPen defaultPen;
  bool usingNonDefaultTextColor = false;

  switch ( item->type() )
  {
    case Item::Message:
    {
      rows = &( skcolumn->messageRows() );
      messageItem = static_cast< MessageItem * >( item );


      if (
           ( ! ( opt.state & QStyle::State_Enabled ) ) ||
           messageItem->aboutToBeRemoved() ||
           ( ! messageItem->isValid() )
         )
      {
        painter->setOpacity( 0.5 );
        defaultPen = QPen( opt.palette.brush( QPalette::Disabled, QPalette::Text ), 0 );
      } else {

        QPalette::ColorGroup cg;

        if ( opt.state & QStyle::State_Active )
          cg = QPalette::Normal;
        else
          cg = QPalette::Inactive;

        if ( opt.state & QStyle::State_Selected )
        {
          defaultPen = QPen( opt.palette.brush( cg, QPalette::HighlightedText ), 0 );
        } else {
          if ( messageItem->textColor().isValid() )
          {
            usingNonDefaultTextColor = true;
            defaultPen = QPen( messageItem->textColor(), 0 );
          } else {
            defaultPen = QPen( opt.palette.brush( cg, QPalette::Text ), 0 );
          }
        }
      }

      top += gMessageVerticalMargin;
      right -= gMessageHorizontalMargin;
      left += gMessageHorizontalMargin;
    }
    break;
    case Item::GroupHeader:
    {
      rows = &( skcolumn->groupHeaderRows() );
      groupHeaderItem = static_cast< GroupHeaderItem * >( item );

      QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;

      if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
        cg = QPalette::Inactive;

      QPalette::ColorRole cr;

      top += gGroupHeaderOuterVerticalMargin;
      right -= gGroupHeaderOuterHorizontalMargin;
      left += gGroupHeaderOuterHorizontalMargin;

      switch ( mTheme->groupHeaderBackgroundMode() )
      {
        case Theme::Transparent:
          cr = ( opt.state & QStyle::State_Selected ) ? QPalette::HighlightedText : QPalette::Text;
          defaultPen = QPen( opt.palette.brush( cg, cr ), 0 );
        break;
        case Theme::AutoColor:
        case Theme::CustomColor:
          switch ( mTheme->groupHeaderBackgroundStyle() )
          {
            case Theme::PlainRect:
            {
              painter->fillRect(
                  QRect( left, top, right - left, opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 )  ),
                  QBrush( mGroupHeaderBackgroundColor )
                );
            }
            break;
            case Theme::PlainJoinedRect:
            {
              int rleft = ( opt.viewItemPosition == QStyleOptionViewItemV4::Beginning ) || ( opt.viewItemPosition == QStyleOptionViewItemV4::OnlyOne ) ? left : opt.rect.left();
              int rright = ( opt.viewItemPosition == QStyleOptionViewItemV4::End ) || ( opt.viewItemPosition == QStyleOptionViewItemV4::OnlyOne ) ? right : opt.rect.left() + opt.rect.width();
              painter->fillRect(
                  QRect( rleft, top, rright - rleft, opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 ) ),
                  QBrush( mGroupHeaderBackgroundColor )
                );
            }
            break;
            case Theme::RoundedJoinedRect:
            {
              if ( opt.viewItemPosition == QStyleOptionViewItemV4::Middle )
              {
                painter->fillRect(
                    QRect( opt.rect.left(), top, opt.rect.width(), opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 ) ),
                    QBrush( mGroupHeaderBackgroundColor )
                  );
                break; // don't fall through
              }
              if ( opt.viewItemPosition == QStyleOptionViewItemV4::Beginning )
              {
                painter->fillRect(
                    QRect( opt.rect.left() + opt.rect.width() - 10, top, 10, opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 ) ),
                    QBrush( mGroupHeaderBackgroundColor )
                  );
              } else if ( opt.viewItemPosition == QStyleOptionViewItemV4::End )
              {
                painter->fillRect(
                    QRect( opt.rect.left(), top, 10 , opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 ) ),
                    QBrush( mGroupHeaderBackgroundColor )
                  );
              }
              // fall through anyway
            }
            case Theme::RoundedRect:
            {
              painter->setPen( Qt::NoPen );
              bool hadAntialiasing = painter->renderHints() & QPainter::Antialiasing;
              if ( !hadAntialiasing )
                painter->setRenderHint( QPainter::Antialiasing, true );
              painter->setBrush( QBrush( mGroupHeaderBackgroundColor ) );
              painter->setBackgroundMode( Qt::OpaqueMode );
              int w = right - left;
              if ( w > 0 )
                painter->drawRoundedRect(
                    QRect( left, top, w, opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 )  ),
                    4.0, 4.0
                  );
              if ( !hadAntialiasing )
                painter->setRenderHint( QPainter::Antialiasing, false );
              painter->setBackgroundMode( Qt::TransparentMode );
            }
            break;
            case Theme::GradientJoinedRect:
            {
              // FIXME: Could cache this brush
              QLinearGradient gradient( 0, top, 0, top + opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 ) );
              gradient.setColorAt( 0.0, KColorScheme::shade( mGroupHeaderBackgroundColor, KColorScheme::LightShade, 0.3 ) );
              gradient.setColorAt( 1.0, mGroupHeaderBackgroundColor );
              if ( opt.viewItemPosition == QStyleOptionViewItemV4::Middle )
              {
                painter->fillRect(
                    QRect( opt.rect.left(), top, opt.rect.width(), opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 ) ),
                    QBrush( gradient )
                  );
                break; // don't fall through
              }
              if ( opt.viewItemPosition == QStyleOptionViewItemV4::Beginning )
              {
                painter->fillRect(
                    QRect( opt.rect.left() + opt.rect.width() - 10, top, 10, opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 ) ),
                    QBrush( gradient )
                  );
              } else if ( opt.viewItemPosition == QStyleOptionViewItemV4::End )
              {
                painter->fillRect(
                    QRect( opt.rect.left(), top, 10 , opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 ) ),
                    QBrush( gradient )
                  );
              }
              // fall through anyway
            }
            case Theme::GradientRect:
            {
              // FIXME: Could cache this brush
              QLinearGradient gradient( 0, top, 0, top + opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 ) );
              gradient.setColorAt( 0.0, KColorScheme::shade( mGroupHeaderBackgroundColor, KColorScheme::LightShade, 0.3 ) );
              gradient.setColorAt( 1.0, mGroupHeaderBackgroundColor );
              painter->setPen( Qt::NoPen );
              bool hadAntialiasing = painter->renderHints() & QPainter::Antialiasing;
              if ( !hadAntialiasing )
                painter->setRenderHint( QPainter::Antialiasing, true );
              painter->setBrush( QBrush( gradient ) );
              painter->setBackgroundMode( Qt::OpaqueMode );
              int w = right - left;
              if ( w > 0 )
                painter->drawRoundedRect(
                    QRect( left, top, w, opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 )  ),
                    4.0, 4.0
                  );
              if ( !hadAntialiasing )
                painter->setRenderHint( QPainter::Antialiasing, false );
              painter->setBackgroundMode( Qt::TransparentMode );
            }
            break;
            case Theme::StyledRect:
            {
              // oxygen, for instance, has a nice graphics for selected items
              opt.rect = QRect( left, top, right - left, opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 ) );
              opt.state |= QStyle::State_Selected;
              opt.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;
              opt.palette.setColor( cg ,QPalette::Highlight, mGroupHeaderBackgroundColor );
              style->drawControl( QStyle::CE_ItemViewItem, &opt, painter, mItemView );
            }
            break;
            case Theme::StyledJoinedRect:
            {
              int rleft = ( opt.viewItemPosition == QStyleOptionViewItemV4::Beginning ) || ( opt.viewItemPosition == QStyleOptionViewItemV4::OnlyOne ) ? left : opt.rect.left();
              int rright = ( opt.viewItemPosition == QStyleOptionViewItemV4::End ) || ( opt.viewItemPosition == QStyleOptionViewItemV4::OnlyOne ) ? right : opt.rect.left() + opt.rect.width();
              opt.rect = QRect( rleft, top, rright - rleft, opt.rect.height() - ( gGroupHeaderInnerVerticalMargin * 2 ) );
              opt.state |= QStyle::State_Selected;
              opt.palette.setColor( cg ,QPalette::Highlight, mGroupHeaderBackgroundColor );
              style->drawControl( QStyle::CE_ItemViewItem, &opt, painter, mItemView );
            }
            break;
          }

          defaultPen = QPen( opt.palette.brush( cg, QPalette::Text ), 0 );
        break;
      }
      top += gGroupHeaderInnerVerticalMargin;
      right -= gGroupHeaderInnerHorizontalMargin;
      left += gGroupHeaderInnerHorizontalMargin;
    }
    break;
    default:
      Q_ASSERT( false );
      return; // bug
    break;
  }

  Qt::LayoutDirection layoutDir = mItemView->layoutDirection();

  for ( QList< Theme::Row * >::ConstIterator rowit = rows->begin(); rowit != rows->end(); ++rowit )
  {
    QSize rowSizeHint = ( *rowit )->sizeHint();
    if ( !rowSizeHint.isValid() )
    {
      compute_size_hint_for_row( ( *rowit ), mPaintDevice, iconSize );
      rowSizeHint = ( *rowit )->sizeHint();
    }

    int bottom = top + rowSizeHint.height();

    // paint right aligned stuff first
    const QList< Theme::ContentItem * > * items = &( ( *rowit )->rightItems() );
    QList< Theme::ContentItem * >::ConstIterator itemit;

    int r = right;
    int l = left;

    for ( itemit = items->begin(); itemit != items->end() ; ++itemit )
    {
      Theme::ContentItem * ci = const_cast< Theme::ContentItem * >( *itemit );

      if ( ci->canUseCustomColor() )
      {
        if ( ci->useCustomColor() && ( !(opt.state & QStyle::State_Selected) ) )
        {
          if ( usingNonDefaultTextColor )
          {
            // merge the colors
            QColor nonDefault = defaultPen.color();
            QColor custom = ci->customColor();
            QColor merged(
                ( nonDefault.red() + custom.red() ) >> 1,
                ( nonDefault.green() + custom.green() ) >> 1,
                ( nonDefault.blue() + custom.blue() ) >> 1
              );
            painter->setPen( QPen( merged ) );
          } else {
            painter->setPen( QPen( ci->customColor() ) );
          }
        } else
          painter->setPen( defaultPen );
      } // otherwise setting a pen is useless at this time

      switch ( ci->type() )
      {
        case Theme::ContentItem::Subject:
          paint_right_aligned_elided_text( item->subject(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::SenderOrReceiver:
          paint_right_aligned_elided_text( item->senderOrReceiver(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::Receiver:
          paint_right_aligned_elided_text( item->receiver(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::Sender:
          paint_right_aligned_elided_text( item->sender(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::Date:
          paint_right_aligned_elided_text( item->formattedDate(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::MostRecentDate:
          paint_right_aligned_elided_text( item->formattedMaxDate(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::Size:
          paint_right_aligned_elided_text( item->formattedSize(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::GroupHeaderLabel:
          if ( groupHeaderItem )
            paint_right_aligned_elided_text( groupHeaderItem->label(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::ReadStateIcon:
            paint_permanent_icon( get_read_state_icon( item ), ci, painter, l, top, r,
                                  layoutDir == Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::CombinedReadRepliedStateIcon:
          if ( messageItem )
            paint_permanent_icon( get_combined_read_replied_state_icon( messageItem ), ci, painter,
                                  l, top, r, layoutDir == Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ExpandedStateIcon:
        {
          const QPixmap * pix = item->childItemCount() > 0 ? ((option.state & QStyle::State_Open) ? Manager::instance()->pixmapShowLess() : Manager::instance()->pixmapShowMore()) : 0;
          paint_boolean_state_icon( pix != 0, pix ? pix : Manager::instance()->pixmapShowMore(),
                                    ci, painter, l, top, r, layoutDir == Qt::LeftToRight,
                                    iconSize );
        }
        break;
        case Theme::ContentItem::RepliedStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_replied_state_icon( messageItem );
            paint_boolean_state_icon( pix != 0, pix ? pix : Manager::instance()->pixmapMessageReplied(),
                                      ci, painter, l, top, r, layoutDir == Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::EncryptionStateIcon:
          if ( messageItem )
          {
            bool enabled;
            const QPixmap * pix = get_encryption_state_icon( messageItem, &enabled );
            paint_boolean_state_icon( enabled, pix, ci, painter, l, top, r,
                                      layoutDir == Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::SignatureStateIcon:
          if ( messageItem )
          {
            bool enabled;
            const QPixmap * pix = get_signature_state_icon( messageItem, &enabled );
            paint_boolean_state_icon( enabled, pix, ci, painter, l, top, r,
                                      layoutDir == Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::SpamHamStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_spam_ham_state_icon( messageItem );
            paint_boolean_state_icon( pix != 0, pix ? pix : Manager::instance()->pixmapMessageSpam(),
                                      ci, painter, l, top, r, layoutDir == Qt::LeftToRight,
                                      iconSize );
          }
        break;
        case Theme::ContentItem::WatchedIgnoredStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_watched_ignored_state_icon( messageItem );
            paint_boolean_state_icon( pix != 0, pix ? pix : Manager::instance()->pixmapMessageWatched(),
                                      ci, painter, l, top, r, layoutDir == Qt::LeftToRight,
                                      iconSize );
          }
        break;
        case Theme::ContentItem::AttachmentStateIcon:
          if ( messageItem )
            paint_boolean_state_icon( messageItem->status().hasAttachment(),
                                      Manager::instance()->pixmapMessageAttachment(), ci, painter,
                                      l, top, r, layoutDir == Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ActionItemStateIcon:
          if ( messageItem )
            paint_boolean_state_icon( messageItem->status().isToAct(),
                                      Manager::instance()->pixmapMessageActionItem(), ci, painter,
                                      l, top, r, layoutDir == Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ImportantStateIcon:
          if ( messageItem )
            paint_boolean_state_icon( messageItem->status().isImportant(),
                                      Manager::instance()->pixmapMessageImportant(), ci, painter, l,
                                      top, r, layoutDir == Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::VerticalLine:
            paint_vertical_line( painter, l, top, r, bottom, layoutDir == Qt::LeftToRight );
        break;
        case Theme::ContentItem::HorizontalSpacer:
            paint_horizontal_spacer( l, top, r, bottom, layoutDir == Qt::LeftToRight );
        break;
        case Theme::ContentItem::TagList:
          if ( messageItem )
          {
            QList< MessageItem::Tag * > * tagList = messageItem->tagList();
            if ( tagList )
              paint_tag_list( tagList, painter, l, top, r, layoutDir == Qt::LeftToRight, iconSize );
          }
        break;
      }
    }

    // then paint left aligned stuff
    items = &( ( *rowit )->leftItems() );

    for ( itemit = items->begin(); itemit != items->end() ; ++itemit )
    {
      Theme::ContentItem * ci = const_cast< Theme::ContentItem * >( *itemit );

      if ( ci->canUseCustomColor() )
      {
        if ( ci->useCustomColor() && ( !(opt.state & QStyle::State_Selected) ) )
        {
          if ( usingNonDefaultTextColor )
          {
            // merge the colors
            QColor nonDefault = defaultPen.color();
            QColor custom = ci->customColor();
            QColor merged(
                ( nonDefault.red() + custom.red() ) >> 1,
                ( nonDefault.green() + custom.green() ) >> 1,
                ( nonDefault.blue() + custom.blue() ) >> 1
              );
            painter->setPen( QPen( merged ) );
          } else {
            painter->setPen( QPen( ci->customColor() ) );
          }
        } else {
          painter->setPen( defaultPen );
        }
      } // otherwise setting a pen is useless at this time

      switch ( ci->type() )
      {
        case Theme::ContentItem::Subject:
          paint_left_aligned_elided_text( item->subject(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::SenderOrReceiver:
          paint_left_aligned_elided_text( item->senderOrReceiver(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::Receiver:
          paint_left_aligned_elided_text( item->receiver(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::Sender:
          paint_left_aligned_elided_text( item->sender(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::Date:
          paint_left_aligned_elided_text( item->formattedDate(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::MostRecentDate:
          paint_left_aligned_elided_text( item->formattedMaxDate(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::Size:
          paint_left_aligned_elided_text( item->formattedSize(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::GroupHeaderLabel:
          if ( groupHeaderItem )
            paint_left_aligned_elided_text( groupHeaderItem->label(), mPaintDevice, ci, painter, l, top, r, layoutDir );
        break;
        case Theme::ContentItem::ReadStateIcon:
            paint_permanent_icon( get_read_state_icon( item ), ci, painter, l, top, r,
                                  layoutDir != Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::CombinedReadRepliedStateIcon:
          if ( messageItem )
            paint_permanent_icon( get_combined_read_replied_state_icon( messageItem ), ci, painter,
                                  l, top, r, layoutDir != Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ExpandedStateIcon:
        {
          const QPixmap * pix = item->childItemCount() > 0 ? ((option.state & QStyle::State_Open) ? Manager::instance()->pixmapShowLess() : Manager::instance()->pixmapShowMore()) : 0;
          paint_boolean_state_icon( pix != 0, pix ? pix : Manager::instance()->pixmapShowMore(),
                                    ci, painter, l, top, r, layoutDir != Qt::LeftToRight, iconSize );
        }
        break;
        case Theme::ContentItem::RepliedStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_replied_state_icon( messageItem );
            paint_boolean_state_icon( pix != 0, pix ? pix : Manager::instance()->pixmapMessageReplied(),
                                      ci, painter, l, top, r, layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::EncryptionStateIcon:
          if ( messageItem )
          {
            bool enabled;
            const QPixmap * pix = get_encryption_state_icon( messageItem, &enabled );
            paint_boolean_state_icon( enabled, pix, ci, painter, l, top, r,
                                      layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::SignatureStateIcon:
          if ( messageItem )
          {
            bool enabled;
            const QPixmap * pix = get_signature_state_icon( messageItem, &enabled );
            paint_boolean_state_icon( enabled, pix, ci, painter, l, top, r,
                                      layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::SpamHamStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_spam_ham_state_icon( messageItem );
            paint_boolean_state_icon( pix != 0, pix ? pix : Manager::instance()->pixmapMessageSpam(),
                                      ci, painter, l, top, r, layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::WatchedIgnoredStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_watched_ignored_state_icon( messageItem );
            paint_boolean_state_icon( pix != 0, pix ? pix : Manager::instance()->pixmapMessageWatched(),
                                      ci, painter, l, top, r, layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::AttachmentStateIcon:
          if ( messageItem )
            paint_boolean_state_icon( messageItem->status().hasAttachment(),
                                      Manager::instance()->pixmapMessageAttachment(), ci, painter,
                                      l, top, r, layoutDir != Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ActionItemStateIcon:
          if ( messageItem )
            paint_boolean_state_icon( messageItem->status().isToAct(),
                                      Manager::instance()->pixmapMessageActionItem(), ci, painter,
                                      l, top, r, layoutDir != Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ImportantStateIcon:
          if ( messageItem )
            paint_boolean_state_icon( messageItem->status().isImportant(),
                                      Manager::instance()->pixmapMessageImportant(), ci, painter, l,
                                      top, r, layoutDir != Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::VerticalLine:
            paint_vertical_line( painter, l, top, r, bottom, layoutDir != Qt::LeftToRight );
        break;
        case Theme::ContentItem::HorizontalSpacer:
            paint_horizontal_spacer( l, top, r, bottom, layoutDir != Qt::LeftToRight );
        break;
        case Theme::ContentItem::TagList:
          if ( messageItem )
          {
            QList< MessageItem::Tag * > * tagList = messageItem->tagList();
            if ( tagList )
              paint_tag_list( tagList, painter, l, top, r, layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
      }
    }

    top = bottom;
  }

  painter->setFont( oldFont );
  painter->setPen( oldPen );
  painter->setOpacity( oldOpacity );
}

bool ThemeDelegate::hitTest( const QPoint &viewportPoint, bool exact )
{
  if ( !mTheme )
    return false; // hm hm...

  mHitIndex = mItemView->indexAt( viewportPoint );

  if ( !mHitIndex.isValid() )
    return false; // bleah

  mHitItem = itemFromIndex( mHitIndex );
  if ( !mHitItem )
    return false; // hm...

  mHitItemRect = mItemView->visualRect( mHitIndex );

  mHitColumn = mTheme->column( mHitIndex.column() );
  if ( !mHitColumn )
    return false; // bleah
  int iconSize = iconSizeFromSizeHint( mHitColumn->messageSizeHint().height() );
  const QList< Theme::Row * > * rows; // I'd like to have it as reference, but gcc complains...

  MessageItem * messageItem = 0;
  GroupHeaderItem * groupHeaderItem = 0;

  int top = mHitItemRect.top();
  int right = mHitItemRect.right();
  int left = mHitItemRect.left();

  mHitRow = 0;
  mHitRowIndex = -1;
  mHitContentItem = 0;

  switch ( mHitItem->type() )
  {
    case Item::Message:
      mHitRowIsMessageRow = true;
      rows = &( mHitColumn->messageRows() );
      messageItem = static_cast< MessageItem * >( mHitItem );
      // FIXME: paint eventual background here

      top += gMessageVerticalMargin;
      right -= gMessageHorizontalMargin;
      left += gMessageHorizontalMargin;
    break;
    case Item::GroupHeader:
      mHitRowIsMessageRow = false;
      rows = &( mHitColumn->groupHeaderRows() );
      groupHeaderItem = static_cast< GroupHeaderItem * >( mHitItem );

      top += gGroupHeaderOuterVerticalMargin + gGroupHeaderInnerVerticalMargin;
      right -= gGroupHeaderOuterHorizontalMargin + gGroupHeaderInnerHorizontalMargin;
      left += gGroupHeaderOuterHorizontalMargin + gGroupHeaderInnerHorizontalMargin;
    break;
    default:
      return false; // bug
    break;
  }

  int rowIdx = 0;
  int bestInexactDistance = 0xffffff;
  bool bestInexactItemRight = false;
  QRect bestInexactRect;
  const Theme::ContentItem * bestInexactContentItem = 0;

  Qt::LayoutDirection layoutDir = mItemView->layoutDirection();

  for ( QList< Theme::Row * >::ConstIterator rowit = rows->begin(); rowit != rows->end(); ++rowit )
  {
    QSize rowSizeHint = ( *rowit )->sizeHint();
    if ( !rowSizeHint.isValid() )
    {
      compute_size_hint_for_row( ( *rowit ), mPaintDevice, iconSize );
      rowSizeHint = ( *rowit )->sizeHint();
    }

    if ( ( viewportPoint.y() < top ) && ( rowIdx > 0 ) )
      break; // not this row (tough we should have already found it... probably clicked upper margin)

    int bottom = top + rowSizeHint.height();

    if ( viewportPoint.y() > bottom )
    {
      top += rowSizeHint.height();
      rowIdx++;
      continue; // not this row
    }

    bestInexactItemRight = false;
    bestInexactDistance = 0xffffff;
    bestInexactContentItem = 0;

    // this row!
    mHitRow = *rowit;
    mHitRowIndex = rowIdx;
    mHitRowRect = QRect( left, top, right - left, bottom - top );

    // check right aligned stuff first
    const QList< Theme::ContentItem * > * items = &( mHitRow->rightItems() );
    QList< Theme::ContentItem * >::ConstIterator itemit;

    mHitContentItemRight = true;

    int r = right;
    int l = left;

    for ( itemit = items->begin(); itemit != items->end() ; ++itemit )
    {
      Theme::ContentItem * ci = const_cast< Theme::ContentItem * >( *itemit );

      mHitContentItemRect = QRect();

      switch ( ci->type() )
      {
        case Theme::ContentItem::Subject:
          compute_bounding_rect_for_right_aligned_elided_text( mHitItem->subject(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::SenderOrReceiver:
          compute_bounding_rect_for_right_aligned_elided_text( mHitItem->senderOrReceiver(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::Receiver:
          compute_bounding_rect_for_right_aligned_elided_text( mHitItem->receiver(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::Sender:
          compute_bounding_rect_for_right_aligned_elided_text( mHitItem->sender(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::Date:
          compute_bounding_rect_for_right_aligned_elided_text( mHitItem->formattedDate(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::MostRecentDate:
          compute_bounding_rect_for_right_aligned_elided_text( mHitItem->formattedMaxDate(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::Size:
          compute_bounding_rect_for_right_aligned_elided_text( mHitItem->formattedSize(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::GroupHeaderLabel:
          if ( groupHeaderItem )
            compute_bounding_rect_for_right_aligned_elided_text( groupHeaderItem->label(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::ReadStateIcon:
          compute_bounding_rect_for_permanent_icon( ci, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::CombinedReadRepliedStateIcon:
          compute_bounding_rect_for_permanent_icon( ci, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ExpandedStateIcon:
          compute_bounding_rect_for_boolean_state_icon( mHitItem->childItemCount() > 0, ci, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::RepliedStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_replied_state_icon( messageItem );
            compute_bounding_rect_for_boolean_state_icon( pix != 0, ci, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::EncryptionStateIcon:
          if ( messageItem )
          {
            bool enabled;
            get_encryption_state_icon( messageItem, &enabled );
            compute_bounding_rect_for_boolean_state_icon( enabled, ci, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::SignatureStateIcon:
          if ( messageItem )
          {
            bool enabled;
            get_signature_state_icon( messageItem, &enabled );
            compute_bounding_rect_for_boolean_state_icon( enabled, ci, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::SpamHamStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_spam_ham_state_icon( messageItem );
            compute_bounding_rect_for_boolean_state_icon( pix != 0, ci, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::WatchedIgnoredStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_watched_ignored_state_icon( messageItem );
            compute_bounding_rect_for_boolean_state_icon( pix != 0, ci, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::AttachmentStateIcon:
          if ( messageItem )
            compute_bounding_rect_for_boolean_state_icon( messageItem->status().hasAttachment(), ci, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ActionItemStateIcon:
          if ( messageItem )
            compute_bounding_rect_for_boolean_state_icon( messageItem->status().isToAct(), ci, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ImportantStateIcon:
          if ( messageItem )
            compute_bounding_rect_for_boolean_state_icon( messageItem->status().isImportant(), ci, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::VerticalLine:
          compute_bounding_rect_for_vertical_line( l, top, r, bottom, mHitContentItemRect, layoutDir == Qt::LeftToRight );
        break;
        case Theme::ContentItem::HorizontalSpacer:
          compute_bounding_rect_for_horizontal_spacer( l, top, r, bottom, mHitContentItemRect, layoutDir == Qt::LeftToRight );
        break;
        case Theme::ContentItem::TagList:
          if ( messageItem )
          {
            QList< MessageItem::Tag * > * tagList = messageItem->tagList();
            if ( tagList )
              compute_bounding_rect_for_tag_list( tagList, l, top, r, mHitContentItemRect, layoutDir == Qt::LeftToRight, iconSize );
          }
        break;
      }

      if ( mHitContentItemRect.isValid() )
      {
        if ( mHitContentItemRect.contains( viewportPoint ) )
        {
          // caught!
          mHitContentItem = ci;
          return true;
        }
        if ( !exact )
        {
          QRect inexactRect( mHitContentItemRect.left(), mHitRowRect.top(), mHitContentItemRect.width(), mHitRowRect.height() );
          if ( inexactRect.contains( viewportPoint ) )
          {
            mHitContentItem = ci;
            return true;
          }

          int inexactDistance = viewportPoint.x() > inexactRect.right() ? viewportPoint.x() - inexactRect.right() : inexactRect.left() - viewportPoint.x();
          if ( inexactDistance < bestInexactDistance )
          {
            bestInexactDistance = inexactDistance;
            bestInexactRect = mHitContentItemRect;
            bestInexactItemRight = true;
            bestInexactContentItem = ci;
          }
        }
      }
    }

    // then check left aligned stuff
    items = &( mHitRow->leftItems() );

    mHitContentItemRight = false;

    for ( itemit = items->begin(); itemit != items->end() ; ++itemit )
    {
      Theme::ContentItem * ci = const_cast< Theme::ContentItem * >( *itemit );

      mHitContentItemRect = QRect();

      switch ( ci->type() )
      {
        case Theme::ContentItem::Subject:
          compute_bounding_rect_for_left_aligned_elided_text( mHitItem->subject(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::SenderOrReceiver:
          compute_bounding_rect_for_left_aligned_elided_text( mHitItem->senderOrReceiver(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::Receiver:
          compute_bounding_rect_for_left_aligned_elided_text( mHitItem->receiver(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::Sender:
          compute_bounding_rect_for_left_aligned_elided_text( mHitItem->sender(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::Date:
          compute_bounding_rect_for_left_aligned_elided_text( mHitItem->formattedDate(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::MostRecentDate:
          compute_bounding_rect_for_left_aligned_elided_text( mHitItem->formattedMaxDate(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::Size:
          compute_bounding_rect_for_left_aligned_elided_text( mHitItem->formattedSize(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::GroupHeaderLabel:
          if ( groupHeaderItem )
            compute_bounding_rect_for_left_aligned_elided_text( groupHeaderItem->label(), mPaintDevice, ci, l, top, r, mHitContentItemRect, layoutDir );
        break;
        case Theme::ContentItem::ReadStateIcon:
          compute_bounding_rect_for_permanent_icon( ci, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::CombinedReadRepliedStateIcon:
          compute_bounding_rect_for_permanent_icon( ci, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ExpandedStateIcon:
          compute_bounding_rect_for_boolean_state_icon( mHitItem->childItemCount() > 0, ci, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::RepliedStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_replied_state_icon( messageItem );
            compute_bounding_rect_for_boolean_state_icon( pix != 0, ci, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::EncryptionStateIcon:
          if ( messageItem )
          {
            bool enabled;
            get_encryption_state_icon( messageItem, &enabled );
            compute_bounding_rect_for_boolean_state_icon( enabled, ci, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::SignatureStateIcon:
          if ( messageItem )
          {
            bool enabled;
            get_signature_state_icon( messageItem, &enabled );
            compute_bounding_rect_for_boolean_state_icon( enabled, ci, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::SpamHamStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_spam_ham_state_icon( messageItem );
            compute_bounding_rect_for_boolean_state_icon( pix != 0, ci, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::WatchedIgnoredStateIcon:
          if ( messageItem )
          {
            const QPixmap * pix = get_watched_ignored_state_icon( messageItem );
            compute_bounding_rect_for_boolean_state_icon( pix != 0, ci, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
        case Theme::ContentItem::AttachmentStateIcon:
          if ( messageItem )
            compute_bounding_rect_for_boolean_state_icon( messageItem->status().hasAttachment(), ci, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ActionItemStateIcon:
          if ( messageItem )
            compute_bounding_rect_for_boolean_state_icon( messageItem->status().isToAct(), ci, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::ImportantStateIcon:
          if ( messageItem )
            compute_bounding_rect_for_boolean_state_icon( messageItem->status().isImportant(), ci, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
        break;
        case Theme::ContentItem::VerticalLine:
          compute_bounding_rect_for_vertical_line( l, top, r, bottom, mHitContentItemRect, layoutDir != Qt::LeftToRight );
        break;
        case Theme::ContentItem::HorizontalSpacer:
          compute_bounding_rect_for_horizontal_spacer( l, top, r, bottom, mHitContentItemRect, layoutDir != Qt::LeftToRight );
        break;
        case Theme::ContentItem::TagList:
          if ( messageItem )
          {
            QList< MessageItem::Tag * > * tagList = messageItem->tagList();
            if ( tagList )
              compute_bounding_rect_for_tag_list( tagList, l, top, r, mHitContentItemRect, layoutDir != Qt::LeftToRight, iconSize );
          }
        break;
      }

      if ( mHitContentItemRect.isValid() )
      {
        if ( mHitContentItemRect.contains( viewportPoint ) )
        {
          // caught!
          mHitContentItem = ci;
          return true;
        }
        if ( !exact )
        {
          QRect inexactRect( mHitContentItemRect.left(), mHitRowRect.top(), mHitContentItemRect.width(), mHitRowRect.height() );
          if ( inexactRect.contains( viewportPoint ) )
          {
            mHitContentItem = ci;
            return true;
          }

          int inexactDistance = viewportPoint.x() > inexactRect.right() ? viewportPoint.x() - inexactRect.right() : inexactRect.left() - viewportPoint.x();
          if ( inexactDistance < bestInexactDistance )
          {
            bestInexactDistance = inexactDistance;
            bestInexactRect = mHitContentItemRect;
            bestInexactItemRight = false;
            bestInexactContentItem = ci;
          }
        }
      }
    }

    top += rowSizeHint.height();
    rowIdx++;
  }

  mHitContentItem = bestInexactContentItem;
  mHitContentItemRight = bestInexactItemRight;
  mHitContentItemRect = bestInexactRect;
  return true;
}

QSize ThemeDelegate::sizeHintForItemTypeAndColumn( Item::Type type, int column ) const
{
  if ( !mTheme )
    return QSize( 16, 16 ); // bleah

  const Theme::Column * skcolumn = mTheme->column( column );
  if ( !skcolumn )
    return QSize( 16, 16 ); // bleah

  const QList< Theme::Row * > * rows; // I'd like to have it as reference, but gcc complains...

  // The sizeHint() is layout direction independent.

  int marginw;
  int marginh;

  switch ( type )
  {
    case Item::Message:
    {
      QSize cached = skcolumn->messageSizeHint();
      if ( cached.isValid() )
        return cached;
      rows = &( skcolumn->messageRows() );

      marginh = gMessageVerticalMargin << 1;
      marginw = gMessageHorizontalMargin << 1;
    }
    break;
    case Item::GroupHeader:
    {
      QSize cached = skcolumn->groupHeaderSizeHint();
      if ( cached.isValid() )
        return cached;
      rows = &( skcolumn->groupHeaderRows() );

      marginh = ( gGroupHeaderOuterVerticalMargin + gGroupHeaderInnerVerticalMargin ) << 1;
      marginw = ( gGroupHeaderOuterVerticalMargin + gGroupHeaderInnerVerticalMargin ) << 1;
    }
    break;
    default:
      return QSize( 16, 16 ); // bug
    break;
  }

  int totalh = 0;
  int maxw = 0;

  for ( QList< Theme::Row * >::ConstIterator rowit = rows->begin(); rowit != rows->end(); ++rowit )
  {
    int iconSize = iconSizeFromSizeHint( skcolumn->messageSizeHint().height() );
    compute_size_hint_for_row( *rowit, mPaintDevice, iconSize );

    QSize sh = ( *rowit )->sizeHint();
    totalh += sh.height();
    if ( sh.width() > maxw )
      maxw = sh.width();
  }

  QSize ret( maxw + marginw , totalh + marginh );

  // cache it
  switch ( type )
  {
    case Item::Message:
      const_cast< Theme::Column * >( skcolumn )->setMessageSizeHint( ret );
    break;
    case Item::GroupHeader:
      const_cast< Theme::Column * >( skcolumn )->setGroupHeaderSizeHint( ret );
    break;
    default:
      // make gcc happy
    break;
  }

  return ret;
}


QSize ThemeDelegate::sizeHint( const QStyleOptionViewItem &, const QModelIndex & index ) const
{
  if ( !mTheme )
    return QSize( 16, 16 ); // hm hm...

  if ( !index.isValid() )
    return QSize( 16, 16 ); // bleah

  Item * item = itemFromIndex( index );
  if ( !item )
    return QSize( 16, 16 ); // hm...

  //Item::Type type = item->type();

  return sizeHintForItemTypeAndColumn( item->type(), index.column() );
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail

