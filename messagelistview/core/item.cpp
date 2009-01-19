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

#include "messagelistview/core/item.h"
#include "messagelistview/core/model.h"
#include "messagelistview/core/manager.h"

#include <kio/global.h> // for KIO::filesize_t and related functions
#include <kmime/kmime_dateformatter.h> // kdepimlibs

#include <KLocale>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

Item::~Item()
{
  killAllChildItems();

  if ( mParent )
    mParent->childItemDead( this );
}

void Item::childItemStats( ChildItemStats &stats ) const
{
  Q_ASSERT( mChildItems );

  stats.mTotalChildCount += mChildItems->count();
  for( QList< Item * >::Iterator it = mChildItems->begin(); it != mChildItems->end(); ++it )
  {
    if ( ( *it )->status().isNew() )
      stats.mNewChildCount++;
    else if ( ( *it )->status().isUnread() )
      stats.mUnreadChildCount++;
    if ( ( *it )->mChildItems )
      ( *it )->childItemStats( stats );
  }
}


Item * Item::itemBelowChild( Item * child )
{
  Q_ASSERT( mChildItems );

  int idx = child->indexGuess();
  if ( !childItemHasIndex( child, idx ) )
  {
    idx = mChildItems->indexOf( child );
    child->setIndexGuess( idx );
  }
  Q_ASSERT( idx >= 0 );

  idx++;

  if ( idx < mChildItems->count() )
    return mChildItems->at( idx );

  if ( !mParent )
    return 0;

  return mParent->itemBelowChild( this );
}

Item * Item::itemBelow()
{
  if ( mChildItems )
  {
    if ( mChildItems->count() > 0 )
      return mChildItems->at( 0 );
  }

  if ( !mParent )
    return 0;

  return mParent->itemBelowChild( this );
}

Item * Item::deepestItem()
{
  if ( mChildItems )
  {
    if ( mChildItems->count() > 0 )
      return mChildItems->at( mChildItems->count() - 1 )->deepestItem();
  }

  return this;
}

Item * Item::itemAboveChild( Item * child )
{
  if ( mChildItems )
  {
    int idx = child->indexGuess();
    if ( !childItemHasIndex( child, idx ) )
    {
      idx = mChildItems->indexOf( child );
      child->setIndexGuess( idx );
    }
    Q_ASSERT( idx >= 0 );
    idx--;

    if ( idx >= 0 )
      return mChildItems->at( idx );
  }

  return this;
}

Item * Item::itemAbove()
{
  if ( !mParent )
    return 0;

  Item *siblingAbove = mParent->itemAboveChild( this );
  if ( siblingAbove && siblingAbove != this && siblingAbove != mParent &&
       siblingAbove->childItemCount() > 0 )
  {
    return siblingAbove->deepestItem();
  }

  return mParent->itemAboveChild( this );
}

Item * Item::topmostNonRoot()
{
  Q_ASSERT( mType != InvisibleRoot );

  if ( !mParent )
    return this;

  if ( mParent->type() == InvisibleRoot )
    return this;

  return mParent->topmostNonRoot();
}


static inline void append_string( QString &buffer, const QString &append )
{
  if ( !buffer.isEmpty() )
    buffer += ", ";
  buffer += append;
}

QString Item::statusDescription() const
{
  QString ret;
  if( status().isNew() )
    append_string( ret, i18nc( "Status of an item", "New" ) );
  else if( status().isUnread() )
    append_string( ret, i18nc( "Status of an item", "Unread" ) );
  else
    append_string( ret, i18nc( "Status of an item", "Read" ) );

  if( status().hasAttachment() )
    append_string( ret, i18nc( "Status of an item", "Has Attachment" ) );

  if( status().isReplied() )
    append_string( ret, i18nc( "Status of an item", "Replied" ) );

  if( status().isForwarded() )
    append_string( ret, i18nc( "Status of an item", "Forwarded" ) );

  if( status().isSent() )
    append_string( ret, i18nc( "Status of an item", "Sent" ) );

  if( status().isImportant() )
    append_string( ret, i18nc( "Status of an item", "Important" ) );

  if( status().isToAct() )
    append_string( ret, i18nc( "Status of an item", "Action Item" ) );

  if( status().isSpam() )
    append_string( ret, i18nc( "Status of an item", "Spam" ) );

  if( status().isHam() )
    append_string( ret, i18nc( "Status of an item", "Ham" ) );

  if( status().isWatched() )
    append_string( ret, i18nc( "Status of an item", "Watched" ) );

  if( status().isIgnored() )
    append_string( ret, i18nc( "Status of an item", "Ignored" ) );

  return ret;
}

const QString & Item::formattedSize()
{
  if ( mFormattedSize.isEmpty() )
    mFormattedSize = KIO::convertSize( ( KIO::filesize_t ) size() );
  return mFormattedSize;
}

const QString & Item::formattedDate()
{
  if ( mFormattedDate.isEmpty() )
  {
    if ( static_cast< uint >( date() ) == static_cast< uint >( -1 ) )
      mFormattedDate = Manager::instance()->cachedLocalizedUnknownText();
    else
      mFormattedDate = Manager::instance()->dateFormatter()->dateString( date() );
  }
  return mFormattedDate;
}

const QString & Item::formattedMaxDate()
{
  if ( mFormattedMaxDate.isEmpty() )
  {
    if ( static_cast< uint >( maxDate() ) == static_cast< uint >( -1 ) )
      mFormattedMaxDate = Manager::instance()->cachedLocalizedUnknownText();
    else
      mFormattedMaxDate = Manager::instance()->dateFormatter()->dateString( maxDate() );
  }
  return mFormattedMaxDate;
}

bool Item::recomputeMaxDate()
{
  time_t newMaxDate = mDate;

  if ( mChildItems )
  {
    for ( QList< Item * >::ConstIterator it = mChildItems->constBegin(); it != mChildItems->constEnd(); ++it )
    {
      if ( ( *it )->mMaxDate > newMaxDate )
        newMaxDate = ( *it )->mMaxDate;
    }
  }
    
  if ( newMaxDate != mMaxDate )
  {
    setMaxDate( newMaxDate );
    return true;
  }
  return false;
}

void Item::setViewable( Model *model,bool bViewable )
{
  if ( mIsViewable == bViewable )
    return;

  if ( !mChildItems )
  {
    mIsViewable = bViewable;
    return;
  }

  if ( mChildItems->count() < 1 )
  {
    mIsViewable = bViewable;
    return;
  }

  if ( bViewable )
  {
    if ( model )
    {
      // fake having no children, for a second
      QList< Item * > * tmp = mChildItems;
      mChildItems = 0;
      //qDebug("BEGIN INSERT ROWS FOR PARENT %x: from %d to %d, (will) have %d children",this,0,tmp->count()-1,tmp->count());
      model->beginInsertRows( model->index( this, 0 ), 0, tmp->count() - 1 );
      mChildItems = tmp;
      mIsViewable = true;
      model->endInsertRows();
    } else {
      mIsViewable = true;
    }

    for ( QList< Item * >::Iterator it = mChildItems->begin(); it != mChildItems->end() ;++it )
     ( *it )->setViewable( model, bViewable );
  } else {
    for ( QList< Item * >::Iterator it = mChildItems->begin(); it != mChildItems->end() ;++it )
      ( *it )->setViewable( model, bViewable );

    // It seems that we can avoid removing child items here since the parent has been removed: this is a hack tough
    // and should check if Qt4 still supports it in the next (hopefully largely fixed) release

    if ( model )
    {
      // fake having no children, for a second
      model->beginRemoveRows( model->index( this, 0 ), 0, mChildItems->count() - 1 );
      QList< Item * > * tmp = mChildItems;
      mChildItems = 0;
      mIsViewable = false;
      model->endRemoveRows();
      mChildItems = tmp;
    } else {
      mIsViewable = false;
    }
  }
}

void Item::killAllChildItems()
{
  if ( !mChildItems )
    return;

  while( !mChildItems->isEmpty() )
    delete mChildItems->first(); // this will call childDead() which will remove the child from the list

  delete mChildItems;
  mChildItems = 0;
}

// FIXME: Try to "cache item insertions" and call beginInsertRows() and endInsertRows() in a chunked fashion...

void Item::rawAppendChildItem( Item * child )
{
  if ( !mChildItems )
    mChildItems = new QList< Item * >();
  mChildItems->append( child );
}

int Item::appendChildItem( Model *model, Item *child )
{
  if ( !mChildItems )
    mChildItems = new QList< Item * >();
  int idx = mChildItems->count();
  if ( mIsViewable )
  {
    if ( model )
      model->beginInsertRows( model->index( this, 0 ), idx, idx ); // THIS IS EXTREMELY UGLY, BUT IT'S THE ONLY POSSIBLE WAY WITH QT4 AT THE TIME OF WRITING
    mChildItems->append( child );
    child->setIndexGuess( idx );
    if ( model )
      model->endInsertRows(); // THIS IS EXTREMELY UGLY, BUT IT'S THE ONLY POSSIBLE WAY WITH QT4 AT THE TIME OF WRITING
    child->setViewable( model, true );
  } else {
    mChildItems->append( child );
    child->setIndexGuess( idx );    
  }
  return idx;
}


void Item::dump( const QString &prefix )
{
  QString out = QString("%1 %x VIEWABLE:%2").arg(prefix).arg(mIsViewable ? "yes" : "no");
  qDebug( out.toUtf8().data(),this );

  QString nPrefix = prefix;
  nPrefix += QString("  ");

  if (!mChildItems )
    return;

  for ( QList< Item * >::Iterator it = mChildItems->begin(); it != mChildItems->end() ;++it )
    (*it)->dump(nPrefix);
}

void Item::takeChildItem( Model *model, Item *child )
{
  if ( !mChildItems )
    return; // Ugh... not our child ?

  if ( !mIsViewable )
  {
    //qDebug("TAKING NON VIEWABLE CHILD ITEM %x",child);
    // We can highly optimise this case
    mChildItems->removeOne( child );
#if 0
    // This *could* be done, but we optimize and avoid it.
    if ( mChildItems->count() < 1 )
    {
      delete mChildItems;
      mChildItems = 0;
    }
#endif
    child->setParent( 0 );
    return;
  }

  // Can't optimise: must call the model functions
  int idx = child->indexGuess();
  if ( mChildItems->count() > idx )
  {
    if ( mChildItems->at( idx ) != child ) // bad guess :/
      idx = mChildItems->indexOf( child );
  } else 
    idx = mChildItems->indexOf( child ); // bad guess :/

  if ( idx < 0 )
    return; // Aaargh... not our child ?

  child->setViewable( model, false );
  if ( model )
    model->beginRemoveRows( model->index( this, 0 ), idx, idx );
  child->setParent( 0 );
  mChildItems->removeAt( idx );
  if ( model )
    model->endRemoveRows();

#if 0
  // This *could* be done, but we optimize and avoid it.
  if ( mChildItems->count() < 1 )
  {
    delete mChildItems;
    mChildItems = 0;
  }
#endif
}


void Item::childItemDead( Item *child )
{
  // mChildItems MUST be non zero here, if it's not then it's a bug in THIS FILE
  mChildItems->removeOne( child ); // since we always have ONE (if we not, it's a bug)
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail
