/*
  progressmanager.cpp

  This file is part of KMail, the KDE mail client.

  Author: Till Adam <adam@kde.org> (C) 2004

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt.  If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.
*/

#include <qvaluevector.h>
#include <kdebug.h>

#include "progressmanager.h"
#include <klocale.h>

namespace KPIM {

KPIM::ProgressManager * KPIM::ProgressManager::mInstance = 0;
unsigned int KPIM::ProgressManager::uID = 42;

ProgressItem::ProgressItem(
       ProgressItem* parent, const QString& id,
       const QString& label, const QString& status, bool canBeCanceled,
       bool usesCrypto )
       :mId( id ), mLabel( label ), mStatus( status ), mParent( parent ),
        mCanBeCanceled( canBeCanceled ), mProgress( 0 ), mTotal( 0 ),
        mCompleted( 0 ), mWaitingForKids( false ), mCanceled( false ),
        mUsesCrypto( usesCrypto )
    {}

ProgressItem::~ProgressItem()
{
}

void ProgressItem::setComplete()
{
//   kdDebug(5006) << "ProgressItem::setComplete - " << label() << endl;
   if ( !mCanceled )
     setProgress( 100 );
   if ( mChildren.isEmpty() ) {
     emit progressItemCompleted( this );
     if ( parent() )
       parent()->removeChild( this );
     deleteLater();
   } else {
     mWaitingForKids = true;
   }
}

void ProgressItem::addChild( ProgressItem *kiddo )
{
  mChildren.replace( kiddo, true );
}

void ProgressItem::removeChild( ProgressItem *kiddo )
{
   mChildren.remove( kiddo );
   // in case we were waiting for the last kid to go away, now is the time
   if ( mChildren.count() == 0 && mWaitingForKids ) {
     emit progressItemCompleted( this );
     deleteLater();
   }
}

void ProgressItem::cancel()
{
   if ( mCanceled || !mCanBeCanceled ) return;
   kdDebug(5006) << "ProgressItem::cancel() - " << label() << endl;
   mCanceled = true;
   // Cancel all children.
   QValueList<ProgressItem*> kids = mChildren.keys();
   QValueList<ProgressItem*>::Iterator it( kids.begin() );
   QValueList<ProgressItem*>::Iterator end( kids.end() );
   for ( ; it != end; it++ ) {
     ProgressItem *kid = *it;
     if ( kid->canBeCanceled() )
       kid->cancel();
   }
   setStatus( i18n( "Aborting..." ) );
   emit progressItemCanceled( this );
}


void ProgressItem::setProgress( unsigned int v )
{
   mProgress = v;
   // kdDebug(5006) << "ProgressItem::setProgress(): " << label() << " : " << v << endl;
   emit progressItemProgress( this, mProgress );
}

void ProgressItem::setLabel( const QString& v )
{
  mLabel = v;
  emit progressItemLabel( this, mLabel );
}

void ProgressItem::setStatus( const QString& v )
{
  mStatus = v;
  emit progressItemStatus( this, mStatus );
}

void ProgressItem::setUsesCrypto( bool v )
{
  mUsesCrypto = v;
  emit progressItemUsesCrypto( this, v );
}

// ======================================

ProgressManager::ProgressManager() :QObject() {
  mInstance = this;
}

ProgressManager::~ProgressManager() { mInstance = 0; }

ProgressItem* ProgressManager::createProgressItemImpl(
     ProgressItem* parent, const QString& id,
     const QString &label, const QString &status,
     bool cancellable, bool usesCrypto )
{
   ProgressItem *t = 0;
   if ( !mTransactions[ id ] ) {
     t = new ProgressItem ( parent, id, label, status, cancellable, usesCrypto );
     mTransactions.insert( id, t );
     if ( parent ) {
       ProgressItem *p = mTransactions[ parent->id() ];
       if ( p ) {
         p->addChild( t );
       }
     }
     // connect all signals
     connect ( t, SIGNAL( progressItemCompleted( ProgressItem* ) ),
               this, SLOT( slotTransactionCompleted( ProgressItem* ) ) );
     connect ( t, SIGNAL( progressItemProgress( ProgressItem*, unsigned int ) ),
               this, SIGNAL( progressItemProgress( ProgressItem*, unsigned int ) ) );
     connect ( t, SIGNAL( progressItemAdded( ProgressItem* ) ),
               this, SIGNAL( progressItemAdded( ProgressItem* ) ) );
     connect ( t, SIGNAL( progressItemCanceled( ProgressItem* ) ),
               this, SIGNAL( progressItemCanceled( ProgressItem* ) ) );
     connect ( t, SIGNAL( progressItemStatus( ProgressItem*, const QString& ) ),
               this, SIGNAL( progressItemStatus( ProgressItem*, const QString& ) ) );
     connect ( t, SIGNAL( progressItemLabel( ProgressItem*, const QString& ) ),
               this, SIGNAL( progressItemLabel( ProgressItem*, const QString& ) ) );
     connect ( t, SIGNAL( progressItemUsesCrypto( ProgressItem*, bool ) ),
               this, SIGNAL( progressItemUsesCrypto( ProgressItem*, bool ) ) );

     emit progressItemAdded( t );
   } else {
     // Hm, is this what makes the most sense?
     t = mTransactions[id];
   }
   return t;
}

ProgressItem* ProgressManager::createProgressItemImpl(
     const QString& parent, const QString &id,
     const QString &label, const QString& status,
     bool canBeCanceled, bool usesCrypto )
{
   ProgressItem * p = mTransactions[parent];
   return createProgressItemImpl( p, id, label, status, canBeCanceled, usesCrypto );
}


// slots

void ProgressManager::slotTransactionCompleted( ProgressItem *item )
{
   mTransactions.remove( item->id() );
   emit progressItemCompleted( item );
}

void ProgressManager::slotStandardCancelHandler( ProgressItem *item )
{
  item->setComplete();
}

ProgressItem* ProgressManager::singleItem() const
{
  if ( mTransactions.count() == 1 )
    return QDictIterator< ProgressItem >( mTransactions ).current();
  return 0;
}

void ProgressManager::slotAbortAll()
{
  QDictIterator< ProgressItem > it( mTransactions );
  for ( ; it.current(); ++it ) {
    it.current()->cancel();
  }
}

} // namespace

#include "progressmanager.moc"
