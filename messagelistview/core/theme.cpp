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

#include "messagelistview/core/theme.h"

#include <QDataStream>

#include <KLocale>
#include <KGlobalSettings>
#include <KDebug>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

static const int gThemeCurrentVersion = 0x1013; // increase if you add new fields of change the meaning of some


Theme::ContentItem::ContentItem( Type type )
  : mType( type ), mFlags( 0 ), mLastPaintDevice( 0 ), mFontMetrics( QFont() )
{
}

Theme::ContentItem::ContentItem( const ContentItem &src )
  : mType( src.mType ),
    mFlags( src.mFlags ),
    mFont( src.mFont ),
    mCustomColor( src.mCustomColor ),
    mLastPaintDevice( src.mLastPaintDevice ),
    mFontMetrics( src.mFontMetrics ),
    mLineSpacing( src.mLineSpacing )
{
}

QString Theme::ContentItem::description( Type type )
{
  switch ( type )
  {
    case Subject:
      return i18n( "Subject" );
    break;
    case Date:
      return i18n( "Date" );
    break;
    case SenderOrReceiver:
      return i18n( "Sender/Receiver" );
    break;
    case Sender:
      return i18n( "Sender" );
    break;
    case Receiver:
      return i18n( "Receiver" );
    break;
    case Size:
      return i18n( "Size" );
    break;
    case ReadStateIcon:
      return i18n( "New/Unread/Read Icon" );
    break;
    case AttachmentStateIcon:
      return i18n( "Attachment Icon" );
    break;
    case RepliedStateIcon:
      return i18n( "Replied/Forwarded Icon" );
    break;
    case CombinedReadRepliedStateIcon:
      return i18n( "Combined New/Unread/Read/Replied/Forwarded Icon" );
    break;
    case ToDoStateIcon:
      return i18n( "To Do Icon" );
    break;
    case ImportantStateIcon:
      return i18n( "Important Icon" );
    break;
    case GroupHeaderLabel:
      return i18n( "Group Header Label" );
    break;
    case SpamHamStateIcon:
      return i18n( "Spam/Ham Icon" );
    break;
    case WatchedIgnoredStateIcon:
      return i18n( "Watched/Ignored Icon" );
    break;
    case ExpandedStateIcon:
      return i18n( "Group Header Expand/Collapse Icon" );
    break;
    case EncryptionStateIcon:
      return i18n( "Encryption State Icon" );
    break;
    case SignatureStateIcon:
      return i18n( "Signature State Icon" );
    break;
    case VerticalLine:
      return i18n( "Vertical Separation Line" );
    break;
    case HorizontalSpacer:
      return i18n( "Horizontal Spacer" );
    break;
    case MostRecentDate:
      return i18n( "Max Date" );
    break;
    case TagList:
      return i18n( "Message Tags" );
    break;
    default:
      return i18n( "Unknown" );
    break;
  }
}


bool Theme::ContentItem::applicableToMessageItems( Type type )
{
  return ( static_cast< int >( type ) & ApplicableToMessageItems );
}

bool Theme::ContentItem::applicableToGroupHeaderItems( Type type )
{
  return ( static_cast< int >( type ) & ApplicableToGroupHeaderItems );
}


void Theme::ContentItem::updateFontMetrics( QPaintDevice * device )
{
  if ( !( mFlags & UseCustomFont ) )
    mFont = KGlobalSettings::generalFont();
  mLastPaintDevice = device;
  mFontMetrics = QFontMetrics( mFont, device );
  mLineSpacing = mFontMetrics.lineSpacing();
}

void Theme::ContentItem::setFont( const QFont &font )
{
  mFont = font;
  mLastPaintDevice = 0; // will force regeneration of font metrics
}

void Theme::ContentItem::resetCache()
{
  mLastPaintDevice = 0; // will force regeneration of font metrics
}

void Theme::ContentItem::save( QDataStream &stream ) const
{
  stream << (int)mType;
  stream << mFlags;
  stream << mFont;
  stream << mCustomColor;
}

bool Theme::ContentItem::load( QDataStream &stream )
{
  int val;

  stream >> val;
  mType = static_cast< Type >( val );
  switch( mType )
  {
    case Subject:
    case Date:
    case SenderOrReceiver:
    case Sender:
    case Receiver:
    case Size:
    case ReadStateIcon:
    case AttachmentStateIcon:
    case RepliedStateIcon:
    case GroupHeaderLabel:
    case ToDoStateIcon:
    case ImportantStateIcon:
    case SpamHamStateIcon:
    case WatchedIgnoredStateIcon:
    case ExpandedStateIcon:
    case EncryptionStateIcon:
    case SignatureStateIcon:
    case VerticalLine:
    case HorizontalSpacer:
    case MostRecentDate:
    case CombinedReadRepliedStateIcon:
    case TagList:
      // ok
    break;
    default:
      kDebug() << "Invalid content item type";
      return false; // b0rken
    break;
  }

  stream >> mFlags;
  stream >> mFont;
  stream >> mCustomColor;
  if ( mFlags & UseCustomColor )
  {
    if ( !mCustomColor.isValid() )
      mFlags &= ~UseCustomColor;
  }
  return true;
}




Theme::Row::Row()
{
}

Theme::Row::Row( const Row &src )
{
  for ( QList< ContentItem * >::ConstIterator it = src.mLeftItems.begin(); it != src.mLeftItems.end() ; ++it )
    addLeftItem( new ContentItem( *( *it ) ) );
  for ( QList< ContentItem * >::ConstIterator it = src.mRightItems.begin(); it != src.mRightItems.end() ; ++it )
    addRightItem( new ContentItem( *( *it ) ) );
}


Theme::Row::~Row()
{
  removeAllLeftItems();
  removeAllRightItems();
}

void Theme::Row::removeAllLeftItems()
{
  while( !mLeftItems.isEmpty() )
    delete mLeftItems.takeFirst();
}

void Theme::Row::removeAllRightItems()
{
  while( !mRightItems.isEmpty() )
    delete mRightItems.takeFirst();
}

void Theme::Row::insertLeftItem( int idx, ContentItem * item )
{
  if ( idx >= mLeftItems.count() )
  {
    mLeftItems.append( item );
    return;
  }
  mLeftItems.insert( idx, item );
}

void Theme::Row::insertRightItem( int idx, ContentItem * item )
{
  if ( idx >= mRightItems.count() )
  {
    mRightItems.append( item );
    return;
  }
  mRightItems.insert( idx, item );
}

void Theme::Row::resetCache()
{
  mSizeHint = QSize();
  for ( QList< ContentItem * >::ConstIterator it = mLeftItems.begin(); it != mLeftItems.end() ; ++it )
    ( *it )->resetCache();
  for ( QList< ContentItem * >::ConstIterator it = mRightItems.begin(); it != mRightItems.end() ; ++it )
    ( *it )->resetCache();
}

bool Theme::Row::containsTextItems() const
{
  for ( QList< ContentItem * >::ConstIterator it = mLeftItems.begin(); it != mLeftItems.end() ; ++it )
  {
    if ( ( *it )->displaysText() )
      return true;
  }
  for ( QList< ContentItem * >::ConstIterator it = mRightItems.begin(); it != mRightItems.end() ; ++it )
  {
    if ( ( *it )->displaysText() )
      return true;
  }
  return false;
}

void Theme::Row::save( QDataStream &stream ) const
{
  stream << (int)mLeftItems.count();

  int cnt = mLeftItems.count();

  for ( int i = 0; i < cnt ; ++i )
  {
    ContentItem * ci = mLeftItems.at( i );
    ci->save( stream );
  }

  stream << (int)mRightItems.count();

  cnt = mRightItems.count();

  for ( int i = 0; i < cnt ; ++i )
  {
    ContentItem * ci = mRightItems.at( i );
    ci->save( stream );
  }
}

bool Theme::Row::load( QDataStream &stream )
{
  removeAllLeftItems();
  removeAllRightItems();

  int val;

  // left item count

  stream >> val;

  if ( ( val < 0 ) || ( val > 50 ) )
    return false; // senseless

  for ( int i = 0; i < val ; ++i )
  {
    ContentItem * ci = new ContentItem( ContentItem::Subject ); // dummy type
    if ( !ci->load( stream ) )
    {
      kDebug() << "Left content item loading failed";
      delete ci;
      return false;
    }
    addLeftItem( ci );
  }

  // right item count

  stream >> val;

  if ( ( val < 0 ) || ( val > 50 ) )
    return false; // senseless

  for ( int i = 0; i < val ; ++i )
  {
    ContentItem * ci = new ContentItem( ContentItem::Subject ); // dummy type
    if ( !ci->load( stream ) )
    {
      kDebug() << "Right content item loading failed";
      delete ci;
      return false;
    }
    addRightItem( ci );
  }

  return true;
}





Theme::Column::Column()
  : mVisibleByDefault( true ),
    mIsSenderOrReceiver( false ),
    mMessageSorting( Aggregation::NoMessageSorting )
{
}

Theme::Column::Column( const Column &src )
{
  mLabel = src.mLabel;
  mVisibleByDefault = src.mVisibleByDefault;
  mIsSenderOrReceiver = src.mIsSenderOrReceiver;
  mMessageSorting = src.mMessageSorting;

  for ( QList< Row * >::ConstIterator it = src.mMessageRows.begin(); it != src.mMessageRows.end() ; ++it )
    addMessageRow( new Row( *( *it ) ) );
  for ( QList< Row * >::ConstIterator it = src.mGroupHeaderRows.begin(); it != src.mGroupHeaderRows.end() ; ++it )
    addGroupHeaderRow( new Row( *( *it ) ) );
}

Theme::Column::~Column()
{
  removeAllMessageRows();
  removeAllGroupHeaderRows();
}

void Theme::Column::removeAllMessageRows()
{
  while ( !mMessageRows.isEmpty() )
    delete mMessageRows.takeFirst();
}

void Theme::Column::removeAllGroupHeaderRows()
{
  while ( !mGroupHeaderRows.isEmpty() )
    delete mGroupHeaderRows.takeFirst();
}

void Theme::Column::insertMessageRow( int idx, Row * row )
{
  if ( idx >= mMessageRows.count() )
  {
    mMessageRows.append( row );
    return;
  }
  mMessageRows.insert( idx, row );
}

void Theme::Column::insertGroupHeaderRow( int idx, Row * row )
{
  if ( idx >= mGroupHeaderRows.count() )
  {
    mGroupHeaderRows.append( row );
    return;
  }
  mGroupHeaderRows.insert( idx, row );
}

void Theme::Column::resetCache()
{
  mGroupHeaderSizeHint = QSize();
  mMessageSizeHint = QSize();

  for ( QList< Row * >::ConstIterator it = mMessageRows.begin(); it != mMessageRows.end() ; ++it )
    ( *it )->resetCache();
  for ( QList< Row * >::ConstIterator it = mGroupHeaderRows.begin(); it != mGroupHeaderRows.end() ; ++it )
    ( *it )->resetCache();
}

bool Theme::Column::containsTextItems() const
{
  for ( QList< Row * >::ConstIterator it = mMessageRows.begin(); it != mMessageRows.end() ; ++it )
  {
    if ( ( *it )->containsTextItems() )
      return true;
  }
  for ( QList< Row * >::ConstIterator it = mGroupHeaderRows.begin(); it != mGroupHeaderRows.end() ; ++it )
  {
    if ( ( *it )->containsTextItems() )
      return true;
  }
  return false;
}

void Theme::Column::save( QDataStream &stream ) const
{
  stream << mLabel;
  stream << mVisibleByDefault;
  stream << mIsSenderOrReceiver;
  stream << (int)mMessageSorting;

  stream << (int)mGroupHeaderRows.count();

  int cnt = mGroupHeaderRows.count();

  for ( int i = 0; i < cnt ; ++i )
  {
    Row * row = mGroupHeaderRows.at( i );
    row->save( stream );
  }

  stream << (int)mMessageRows.count();

  cnt = mMessageRows.count();

  for ( int i = 0; i < cnt ; ++i )
  {
    Row * row = mMessageRows.at( i );
    row->save( stream );
  }
}

bool Theme::Column::load( QDataStream &stream )
{
  removeAllGroupHeaderRows();
  removeAllMessageRows();

  stream >> mLabel;
  stream >> mVisibleByDefault;
  stream >> mIsSenderOrReceiver;

  int val;

  stream >> val;
  mMessageSorting = static_cast< Aggregation::MessageSorting >( val );
  if ( !Aggregation::isValidMessageSorting( mMessageSorting ) )
  {
    kDebug() << "Invalid message sorting";
    return false;
  }

  // group header row count
  stream >> val;

  if ( ( val < 0 ) || ( val > 50 ) )
  {
    kDebug() << "Invalid group header row count";
    return false; // senseless
  }

  for ( int i = 0; i < val ; i++ )
  {
    Row * row = new Row();
    if ( !row->load( stream ) )
    {
      kDebug() << "Group header row loading failed";
      delete row;
      return false;
    }
    addGroupHeaderRow( row );
  }

  // message row count
  stream >> val;

  if ( ( val < 0 ) || ( val > 50 ) )
  {
    kDebug() << "Invalid message row count";
    return false; // senseless
  }

  for ( int i = 0; i < val ; i++ )
  {
    Row * row = new Row();
    if ( !row->load( stream ) )
    {
      kDebug() << "Message row loading failed";
      delete row;
      return false;
    }
    addMessageRow( row );
  }

  return true;
}




Theme::Theme()
  : OptionSet()
{
  mGroupHeaderBackgroundMode = AutoColor;
  mViewHeaderPolicy = ShowHeaderAlways;
}

Theme::Theme( const QString &name, const QString &description )
  : OptionSet( name, description )
{
  mGroupHeaderBackgroundMode = AutoColor;
  mGroupHeaderBackgroundStyle = GradientRect;
  mViewHeaderPolicy = ShowHeaderAlways;
}


Theme::Theme( const Theme &src )
  : OptionSet( src )
{
  mGroupHeaderBackgroundMode = src.mGroupHeaderBackgroundMode;
  mGroupHeaderBackgroundColor = src.mGroupHeaderBackgroundColor;
  mGroupHeaderBackgroundStyle = src.mGroupHeaderBackgroundStyle;
  mViewHeaderPolicy = src.mViewHeaderPolicy;

  for ( QList< Column * >::ConstIterator it = src.mColumns.begin(); it != src.mColumns.end() ; ++it )
    addColumn( new Column( *( *it ) ) );
}

Theme::~Theme()
{
  removeAllColumns();
}

void Theme::removeAllColumns()
{
  while ( !mColumns.isEmpty() )
    delete mColumns.takeFirst();
}

void Theme::insertColumn( int idx, Column * column )
{
  if ( idx >= mColumns.count() )
  {
    mColumns.append( column );
    return;
  }
  mColumns.insert( idx, column );
}

void Theme::setGroupHeaderBackgroundMode( GroupHeaderBackgroundMode bm )
{
  mGroupHeaderBackgroundMode = bm;
  if ( ( bm == CustomColor ) && !mGroupHeaderBackgroundColor.isValid() )
    mGroupHeaderBackgroundColor = QColor( 127, 127, 127 ); // something neutral     
}

QList< QPair< QString, int > > Theme::enumerateViewHeaderPolicyOptions()
{
  QList< QPair< QString, int > > ret;
  ret.append( QPair< QString, int >( i18n( "Never Show" ), NeverShowHeader ) );
  ret.append( QPair< QString, int >( i18n( "Always Show" ), ShowHeaderAlways ) );
  return ret;
}

QList< QPair< QString, int > > Theme::enumerateGroupHeaderBackgroundStyles()
{
  QList< QPair< QString, int > > ret;
  ret.append( QPair< QString, int >( i18n( "Plain Rectangles" ), PlainRect ) );
  ret.append( QPair< QString, int >( i18n( "Plain Joined Rectangle" ), PlainJoinedRect ) );
  ret.append( QPair< QString, int >( i18n( "Rounded Rectangles" ), RoundedRect ) );
  ret.append( QPair< QString, int >( i18n( "Rounded Joined Rectangle" ), RoundedJoinedRect ) );
  ret.append( QPair< QString, int >( i18n( "Gradient Rectangles" ), GradientRect ) );
  ret.append( QPair< QString, int >( i18n( "Gradient Joined Rectangle" ), GradientJoinedRect ) );
  ret.append( QPair< QString, int >( i18n( "Styled Rectangles" ), StyledRect ) );
  ret.append( QPair< QString, int >( i18n( "Styled Joined Rectangles" ), StyledJoinedRect ) );

  return ret;
}


void Theme::resetCache()
{
  for ( QList< Column * >::ConstIterator it = mColumns.begin(); it != mColumns.end() ; ++it )
    ( *it )->resetCache();
}

bool Theme::load( QDataStream &stream )
{
  removeAllColumns();

  int val;

  stream >> val;
  if ( val != gThemeCurrentVersion )
  {
    kDebug() << "Invalid theme version";
    return false; // b0rken (invalid version)
  }

  stream >> val;
  mGroupHeaderBackgroundMode = (GroupHeaderBackgroundMode)val;
  switch(mGroupHeaderBackgroundMode)
  {
    case Transparent:
    case AutoColor:
    case CustomColor:
      // ok
    break;
    default:
      kDebug() << "Invalid theme group header background mode";
      return false; // b0rken
    break;
  }

  stream >> mGroupHeaderBackgroundColor;

  stream >> val;
  mGroupHeaderBackgroundStyle = (GroupHeaderBackgroundStyle)val;
  switch(mGroupHeaderBackgroundStyle)
  {
    case PlainRect:
    case PlainJoinedRect:
    case RoundedRect:
    case RoundedJoinedRect:
    case GradientRect:
    case GradientJoinedRect:
    case StyledRect:
    case StyledJoinedRect:
      // ok
    break;
    default:
      kDebug() << "Invalid theme group header background style";
      return false; // b0rken
    break;
  }

  stream >> val;
  mViewHeaderPolicy = (ViewHeaderPolicy)val;
  switch(mViewHeaderPolicy)
  {
    case ShowHeaderAlways:
    case NeverShowHeader:
      // ok
    break;
    default:
      kDebug() << "Invalid theme view header policy";
      return false; // b0rken
    break;
  }

  // column count
  stream >> val;
  if ( val < 1 || val > 50 )
    return false; // plain b0rken ( negative, zero or more than 50 columns )

  for ( int i = 0; i < val ; i++ )
  {
    Column * col = new Column();
    if ( !col->load( stream ) )
    {
      kDebug() << "Column loading failed";
      delete col;
      return false;
    }
    addColumn( col );
  }  

  return true;
}

void Theme::save( QDataStream &stream ) const
{
  stream << (int)gThemeCurrentVersion;

  stream << (int)mGroupHeaderBackgroundMode;
  stream << mGroupHeaderBackgroundColor;
  stream << (int)mGroupHeaderBackgroundStyle;
  stream << (int)mViewHeaderPolicy;

  stream << (int)mColumns.count();

  int cnt = mColumns.count();

  for ( int i = 0; i < cnt ; i++ )
  {
    Column * col = mColumns.at( i );
    col->save( stream );
  }
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail

