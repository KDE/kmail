/** -*- c++ -*-
 * progressdialog.cpp
 *
 *  Copyright (c) 2004 Till Adam <adam@kde.org>,
 *                     David Faure <faure@kde.org>
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
#include <qscrollview.h>
#include <qtoolbutton.h>
#include <qpushbutton.h>
#include <qvbox.h>

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

class TransactionItem;

TransactionItemView::TransactionItemView( QWidget * parent,
                                          const char * name,
                                          WFlags f )
    : QScrollView( parent, name, f ) {
  setFrameStyle( NoFrame );
  mBigBox = new QVBox( viewport() );
  mBigBox->setSpacing( 5 );
  addChild( mBigBox );
  setResizePolicy( QScrollView::AutoOneFit ); // Fit so that the box expands horizontally
}

TransactionItem* TransactionItemView::addTransactionItem( ProgressItem* item, bool first )
{
   TransactionItem *ti = new TransactionItem( mBigBox, item, first );
   ti->show();
   return ti;
}

void TransactionItemView::resizeContents( int w, int h )
{
  //kdDebug(5006) << k_funcinfo << w << "," << h << endl;
  QScrollView::resizeContents( w, h );
  // Tell the parent (progressdialog) to adapt itself to our new size
  updateGeometry();
  parentWidget()->adjustSize();
}

QSize TransactionItemView::sizeHint() const
{
  return minimumSizeHint();
}

QSize TransactionItemView::minimumSizeHint() const
{
  int f = 2 * frameWidth();
  int minw = topLevelWidget()->width() / 3;
  int maxh = topLevelWidget()->height() / 2;
  QSize sz( mBigBox->minimumSizeHint() );
  sz.setWidth( QMAX( sz.width(), minw ) + f );
  sz.setHeight( QMIN( sz.height(), maxh ) + f );
  return sz;
}

// ----------------------------------------------------------------------------

TransactionItem::TransactionItem( QWidget* parent,
                                  ProgressItem *item, bool first )
  : QVBox( parent ), mCancelButton( 0 ), mItem( item )

{
  setSpacing( 2 );
  setMargin( 2 );
  setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed ) );
  if ( !first ) {
    QFrame *f = new QFrame( this );
    f->setFrameShape( QFrame::HLine );
    f->setFrameShadow( QFrame::Raised );
    f->show();
    setStretchFactor( f, 3 );
  }

  QHBox *h = new QHBox( this );
  h->setSpacing( 5 );

  mItemLabel = new QLabel( item->label(), h );
  h->setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed ) );

  mProgress = new QProgressBar( 100, h );
  mProgress->setProgress( item->progress() );
  if ( item->canBeCanceled() ) {
    mCancelButton = new QPushButton( SmallIcon( "cancel" ), QString::null, h );
    connect ( mCancelButton, SIGNAL( clicked() ),
              this, SLOT( slotItemCanceled() ));
  }


  mItemStatus =  new QLabel( item->status(), this );
}

TransactionItem::~TransactionItem()
{

}

void TransactionItem::setProgress( int progress )
{
  mProgress->setProgress( progress );
}

void TransactionItem::setLabel( const QString& label )
{
  mItemLabel->setText( label );
}

void TransactionItem::setStatus( const QString& status )
{
  mItemStatus->setText( status );
}

void TransactionItem::slotItemCanceled()
{
  item()->setStatus( i18n("cancelling ... ") );
  item()->cancel();
}


void TransactionItem::addSubTransaction( ProgressItem* /*item*/ )
{

}


// ---------------------------------------------------------------------------

ProgressDialog::ProgressDialog( QWidget* alignWidget, QWidget* parent, const char* name )
    : OverlayWidget( alignWidget, parent, name )
{
    setFrameStyle( QFrame::Panel | QFrame::Sunken ); // QFrame
    setSpacing( 0 ); // QHBox
    setMargin( 1 );

    mScrollView = new TransactionItemView( this, "ProgressScrollView" );

    QVBox* rightBox = new QVBox( this );
    QToolButton* pbClose = new QToolButton( rightBox );
    pbClose->setAutoRaise(true);
    pbClose->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
    pbClose->setFixedSize( 16, 16 );
    pbClose->setIconSet( KGlobal::iconLoader()->loadIconSet( "fileclose", KIcon::Small, 14 ) );
    connect(pbClose, SIGNAL(clicked()), this, SLOT(close()));
    QWidget* spacer = new QWidget( rightBox ); // don't let the close button take up all the height
    rightBox->setStretchFactor( spacer, 100 );


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

void ProgressDialog::closeEvent( QCloseEvent* e )
{
  e->accept();
  hide();
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
   if ( item->parent() ) {
     if ( mTransactionsToListviewItems.contains( item->parent() ) ) {
       parent = mTransactionsToListviewItems[ item->parent() ];
       parent->addSubTransaction( item );
     }
   } else {
     TransactionItem *ti = mScrollView->addTransactionItem( item, mTransactionsToListviewItems.empty() );
     if ( ti )
       mTransactionsToListviewItems.replace( item, ti );
   }
}

void ProgressDialog::slotTransactionCompleted( ProgressItem *item )
{
   if ( mTransactionsToListviewItems.contains( item ) ) {
     TransactionItem *ti = mTransactionsToListviewItems[ item ];
     ti->setStatus(i18n("Completed"));
     mTransactionsToListviewItems.remove( item );
     QTimer::singleShot( 5000, ti, SLOT( deleteLater() ) );
   }
   // This was the last item, hide.
   if ( mTransactionsToListviewItems.empty() )
     QTimer::singleShot( 6000, this, SLOT( slotHide() ) );
}

void ProgressDialog::slotTransactionCanceled( ProgressItem* /* item */ )
{
}

void ProgressDialog::slotTransactionProgress( ProgressItem *item,
                                              unsigned int progress )
{
   if ( mTransactionsToListviewItems.contains( item ) ) {
     TransactionItem *ti = mTransactionsToListviewItems[ item ];
     ti->setProgress( progress );
   }
}

void ProgressDialog::slotTransactionStatus( ProgressItem *item,
                                            const QString& status )
{
   if ( mTransactionsToListviewItems.contains( item ) ) {
     TransactionItem *ti = mTransactionsToListviewItems[ item ];
     ti->setStatus( status );
   }
}

void ProgressDialog::slotTransactionLabel( ProgressItem *item,
                                           const QString& label )
{
   if ( mTransactionsToListviewItems.contains( item ) ) {
     TransactionItem *ti = mTransactionsToListviewItems[ item ];
     ti->setLabel( label );
   }
}

void ProgressDialog::slotHide()
{
  // check if a new item showed up since we started the timer. If not, hide
  if ( mTransactionsToListviewItems.isEmpty() )
    hide();
}

}

#include "progressdialog.moc"
