/** -*- c++ -*-
 * progressdialog.cpp
 *
 *  Copyright (c) 2004 Till Adam <adam@kde.org>
 *  based on imapprogressdialog.cpp ,which is
 *  Copyright (c) 2002-2003 Klaralvdalens Datakonsult AB
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qlayout.h>
#include <qprogressbar.h>
#include <qtimer.h>
#include <qheader.h>
#include <qobject.h>

#include <kpushbutton.h>
#include <klocale.h>
#include <kdialog.h>
#include <kstdguiitem.h>
#include <kiconloader.h>
#include <kdebug.h>

#include "progressdialog.h"
#include "progressmanager.h"
using KMail::ProgressItem;
using KMail::ProgressManager;

namespace KMail {

TransactionItemListView::TransactionItemListView( QWidget * parent,
                                                  const char * name,
                                                  WFlags f )
    : QListView( parent, name, f ) {

    connect ( header(),SIGNAL( sizeChange ( int, int, int ) ),
              this, SLOT( slotAdjustGeometry() ) );
    connect ( header(),SIGNAL( indexChange ( int, int, int ) ),
              this, SLOT( slotAdjustGeometry() ) );
    connect ( this, SIGNAL( expanded( QListViewItem * ) ),
              this, SLOT( slotAdjustGeometry() ) );
    connect ( this, SIGNAL( collapsed( QListViewItem * ) ),
              this, SLOT( slotAdjustGeometry() ) );

}

void TransactionItemListView::resizeEvent( QResizeEvent* e )
{
  slotAdjustGeometry();
  QListView::resizeEvent( e );
}

void TransactionItemListView::slotAdjustGeometry() {
  for (QListViewItemIterator it( this ); it.current(); it++) {
    TransactionItem *i = static_cast<TransactionItem*>( it.current() );
    i->adjustGeometry();
  }
}

// ----------------------------------------------------------------------------

TransactionItem::TransactionItem ( QListViewItem* parent,
                                   ProgressItem *i )
  : QListViewItem( parent ), mCancelButton( 0 )
{
  init( i);
}

TransactionItem::TransactionItem( QListView* parent,
                                  QListViewItem* lastItem,
                                  ProgressItem *i )
  : QListViewItem( parent, lastItem ), mCancelButton( 0 )
{
  init( i );
}

void TransactionItem::init ( ProgressItem *item )
{
  mItem = item;
  mProgress = new QProgressBar( 100, listView()->viewport() );
  mProgress->setProgress( item->progress() );
  mProgress->show();
  if ( item->canBeCanceled() ) {
    mCancelButton = new QPushButton( SmallIcon( "cancel" ), QString::null,
                                     listView()->viewport() );
    mCancelButton->setSizePolicy( QSizePolicy( QSizePolicy::Minimum,
                                               QSizePolicy::Minimum ) );

    mCancelButton->show();
    connect ( mCancelButton, SIGNAL( clicked() ),
              this, SLOT( slotItemCanceled() ));

  }
  adjustGeometry();
  setText( 0, item->label() );
  setText( 3, item->status() );
  setSelectable( false );
}

TransactionItem::~TransactionItem()
{
  delete mCancelButton;
  delete mProgress;
}

void TransactionItem::setProgress( int progress )
{
  mProgress->setProgress( progress );
}

void TransactionItem::setLabel( const QString& label )
{
  setText( 0, label );
}

void TransactionItem::setStatus( const QString& status )
{
  setText( 3, status );
}

void TransactionItem::adjustGeometry()
{
  QRect r = listView()->itemRect( this );
  QHeader *h = listView()->header();
  if ( mCancelButton ) {
    r.setLeft( h->sectionPos( 1 ) - h->offset() );
    r.setWidth( h->sectionSize( 1 ));
    mCancelButton->setGeometry( r );
  }
  r.setLeft( h->sectionPos( 2 ) - h->offset() );
  r.setWidth( h->sectionSize( 2 ));
  mProgress->setGeometry( r );
}

void TransactionItem::slotItemCanceled()
{
  item()->setStatus( i18n("cancelling ... ") );
  item()->cancel();
}




ProgressDialog::ProgressDialog( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    setCaption( i18n("Progress") );
    resize( 600, 400 );


    QBoxLayout* topLayout = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint(),
                                             "topLayout");

    mListView = new TransactionItemListView( this, "SyncEditorListView" );
    mListView->addColumn( i18n( "Transaction" ) );
    mListView->setColumnWidth( 0, 100 );
    mListView->setColumnWidthMode(0, QListView::Maximum);
    mListView->addColumn( QString::null );
    mListView->setColumnWidth( 1, 25 );
    mListView->addColumn( i18n( "Progress" ) );
    mListView->setColumnWidth( 2, 180 );
    mListView->addColumn( i18n( "Status" ) );
    mListView->setColumnWidth( 3, 100 );
    mListView->setColumnWidthMode(3, QListView::Maximum);
    mListView->setSorting( -1, false );

    mListView->setRootIsDecorated( true );

    topLayout->addWidget( mListView );

    QBoxLayout* bottomLayout = new QHBoxLayout( topLayout, KDialog::spacingHint(), "bottomLayout");
    bottomLayout->addStretch();

    KPushButton* pbClose = new KPushButton( KStdGuiItem::close(), this );
    bottomLayout->addWidget( pbClose );

    connect(pbClose, SIGNAL(clicked()), this, SLOT(close())  );

    /*
     * Get the singleton ProgressManager item which will inform us of
     * appearing and vanishing items.
     */
    ProgressManager *pm = ProgressManager::instance();
    connect ( pm, SIGNAL( progressItemAdded( ProgressItem* ) ),
              this, SLOT( slotTransactionAdded( ProgressItem* ) ) );
    connect ( pm, SIGNAL( progressItemCompleted( ProgressItem* ) ),
              this, SLOT( slotTransactionCompleted( ProgressItem* ) ) );
    connect ( pm, SIGNAL( progressItemProgress( ProgressItem*, unsigned int ) ),
              this, SLOT( slotTransactionProgress( ProgressItem*, unsigned int ) ) );
    connect ( pm, SIGNAL( progressItemStatus( ProgressItem*, const QString& ) ),
              this, SLOT( slotTransactionStatus( ProgressItem*, const QString& ) ) );
    connect ( pm, SIGNAL( progressItemLabel( ProgressItem*, const QString& ) ),
              this, SLOT( slotTransactionLabel( ProgressItem*, const QString& ) ) );
}

void ProgressDialog::clear()
{
   QListViewItem* item;
   while( ( item = mListView->firstChild() ) != 0 ) delete item;
   mPreviousItem = 0;
}


void ProgressDialog::closeEvent( QCloseEvent* e )
{
  e->accept();
  hide();
}


void ProgressDialog::showEvent( QShowEvent* e )
{
  mListView->slotAdjustGeometry();
  QDialog::showEvent( e );
}


/*
 *  Destructor
 */
ProgressDialog::~ProgressDialog()
{
    // no need to delete child widgets.
}

void ProgressDialog::slotTransactionAdded( ProgressItem *item )
{
   TransactionItem *parent = 0;
   TransactionItem *ti = 0;
   if ( item->parent() ) {
     parent = mTransactionsToListviewItems[ item->parent() ];
     ti = new TransactionItem( parent, item );
   } else {
     ti = new TransactionItem( mListView, mListView->lastItem(), item );
   }
   mTransactionsToListviewItems.replace( item, ti );
   // First item to appear, show.
   // FIXME Maybe it's better to only auto show if the dialog was timed out
   // and not closed by the user. dunno
   // if ( mTransactionsToListviewItems.size() == 1 ) show();
}

void ProgressDialog::slotTransactionCompleted( ProgressItem *item )
{
   TransactionItem *ti = mTransactionsToListviewItems[ item ];
   if ( ti ) {
     ti->setStatus(i18n("Completed"));
     mTransactionsToListviewItems.remove( item );
     QTimer::singleShot( 5000, ti, SLOT( deleteLater() ) );
   }
   mListView->slotAdjustGeometry();
   // This was the last item, hide.
   if ( mTransactionsToListviewItems.size() == 0 )
     QTimer::singleShot( 5000, this, SLOT( slotHide() ) );
}

void ProgressDialog::slotTransactionCanceled( ProgressItem * )
{
}

void ProgressDialog::slotTransactionProgress( ProgressItem *item,
                                              unsigned int progress )
{
   TransactionItem *ti = mTransactionsToListviewItems[ item ];
   if ( ti ) {
     ti->setProgress( progress );
   }
}

void ProgressDialog::slotTransactionStatus( ProgressItem *item,
                                            const QString& status )
{
   TransactionItem *ti = mTransactionsToListviewItems[ item ];
   if ( ti ) {
     ti->setStatus( status );
   }
}

void ProgressDialog::slotTransactionLabel( ProgressItem *item,
                                           const QString& label )
{
   TransactionItem *ti = mTransactionsToListviewItems[ item ];
   if ( ti ) {
     ti->setLabel( label );
   }
}

void ProgressDialog::slotHide()
{
  // check if a new item showed up since we started the timer. If not, hide
  if ( mTransactionsToListviewItems.size() == 0 )
    hide();
}

}
#include "progressdialog.moc"
