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
    mBigBox = new QVBox( viewport() );
    mBigBox->setSpacing(2);
    addChild( mBigBox );
    setResizePolicy( QScrollView::AutoOneFit );
}

TransactionItem* TransactionItemView::addTransactionItem( ProgressItem* item, bool first )
{
   TransactionItem *ti = new TransactionItem( mBigBox, item, first );
   ti->show();

   return ti;
}


// ----------------------------------------------------------------------------

TransactionItem::TransactionItem( QWidget* parent,
                                  ProgressItem *item, bool first )
  : QVBox( parent ), mCancelButton( 0 )

{
  setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ) );
  if ( !first ) {
    QFrame *f = new QFrame( this );
    f->setFrameShape( QFrame::HLine );
    f->setFrameShadow( QFrame::Raised );
    f->show();
    setStretchFactor( f, 3 );
  }

  QHBox *h = new QHBox( this );
  h->setSpacing( 5 );
  h->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ) );
  mProgress = new QProgressBar( 100, h );
  mProgress->setProgress( item->progress() );
  if ( item->canBeCanceled() ) {
    mCancelButton = new QPushButton( SmallIcon( "cancel" ), QString::null, h );
    connect ( mCancelButton, SIGNAL( clicked() ),
              this, SLOT( slotItemCanceled() ));
  }
  mItemLabel = new QLabel( item->label(), h );

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

void ProgressDialog::clear()
{

}


void ProgressDialog::closeEvent( QCloseEvent* e )
{
  e->accept();
  hide();
}


void ProgressDialog::showEvent( QShowEvent* e )
{
  resize( sizeHint() );
  OverlayWidget::showEvent( e );
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
     parent->addSubTransaction( item );
   } else {
     ti = mScrollView->addTransactionItem( item, mTransactionsToListviewItems.empty() );
   }
   if ( ti )
     mTransactionsToListviewItems.replace( item, ti );
}

void ProgressDialog::slotTransactionCompleted( ProgressItem *item )
{
   TransactionItem *ti = mTransactionsToListviewItems[ item ];
   if ( ti ) {
     ti->setStatus(i18n("Completed"));
     mTransactionsToListviewItems.remove( item );
     QTimer::singleShot( 5000, ti, SLOT( deleteLater() ) );
   }
   // This was the last item, hide.
   if ( mTransactionsToListviewItems.empty() )
     QTimer::singleShot( 5000, this, SLOT( slotHide() ) );
   else
     resize( sizeHint() );
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
