/** -*- c++ -*-
 * imapprogressdialog.cpp
 *
 * Copyright (c) 2002 Klarälvdalens Datakonsult AB
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
 */

#include "imapprogressdialog.h"

#include <kpushbutton.h>
#include <klocale.h>
#include <kdialog.h>
#include <kstdguiitem.h>

#include <qlayout.h>
#include <qstyle.h>
#include <qpainter.h>
#include <qprogressbar.h>


namespace KMail {


ProgressListViewItem::ProgressListViewItem(int col, int pro, QListView* parent,
					   const QString& label1,
                                           const QString& label2,
					   const QString& label3,
                                           const QString& label4,
					   const QString& label5,
                                           const QString& label6,
					   const QString& label7,
                                           const QString& label8 )
  : QListViewItem( parent, label1, label2, label3, label4, label5, label6,
                   label7, label8 )
{
  pbcol = col;
  prog = pro;
  mProgress = new QProgressBar( 100, 0 );
  mProgress->setProgress( prog );
}

ProgressListViewItem::ProgressListViewItem(int col, int pro, QListView* parent,
                                           ProgressListViewItem* after,
					   const QString& label1,
                                           const QString& label2,
					   const QString& label3,
                                           const QString& label4,
					   const QString& label5,
                                           const QString& label6,
					   const QString& label7,
                                           const QString& label8 )
  : QListViewItem( parent, after, label1, label2, label3, label4, label5,
                   label6, label7, label8 )
{
  pbcol = col;
  prog = pro;
  mProgress = new QProgressBar( 100, 0 );
  mProgress->setProgress( prog );
}

void ProgressListViewItem::setProgress( int progress )
{
  mProgress->setProgress( progress );
}

void ProgressListViewItem::paintCell( QPainter *p, const QColorGroup &cg,
                                 int column, int width, int alignment )
{
  QColorGroup _cg( cg );
  QColor c = _cg.text();


  if ( column == pbcol ){
    const QRect bar = QRect( 0, 0, width, height() );
    mProgress->resize( width, height() );

    QPixmap pm( bar.size() );
    QPainter paint( &pm );

    paint.fillRect( bar, listView()->paletteBackgroundColor() );
    paint.setFont( p->font() );

    QStyle::SFlags flags = QStyle::Style_Default;
    if (isEnabled())
      flags |= QStyle::Style_Enabled;

    listView()->style().drawControl(QStyle::CE_ProgressBarGroove, &paint, mProgress,
			QStyle::visualRect(listView()->style().subRect(QStyle::SR_ProgressBarGroove, mProgress), mProgress ),
			listView()->colorGroup(), flags);

    listView()->style().drawControl(QStyle::CE_ProgressBarContents, &paint, mProgress,
			QStyle::visualRect(listView()->style().subRect(QStyle::SR_ProgressBarContents, mProgress), mProgress ),
			listView()->colorGroup(), flags);

    if (mProgress->percentageVisible())
      listView()->style().drawControl(QStyle::CE_ProgressBarLabel, &paint, mProgress,
			  QStyle::visualRect(listView()->style().subRect(QStyle::SR_ProgressBarLabel, mProgress), mProgress ),
			  listView()->colorGroup(), flags);
    paint.end();

    p->drawPixmap( bar.x(), bar.y(), pm );

  }
  else {
    _cg.setColor( QColorGroup::Text, c );
    QListViewItem::paintCell( p, _cg, column, width, alignment );
  }
}

IMAPProgressDialog::IMAPProgressDialog( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl ),
      mPreviousItem( 0 )
{

    setCaption( i18n("IMAP Progress") );
    resize( 360, 328 );

    QBoxLayout* topLayout = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint(), 
					     "topLayout");

    mSyncEditorListView = new QListView( this, "SyncEditorListView" );
    mSyncEditorListView->addColumn( i18n( "Folder" ) );
    mSyncEditorListView->addColumn( i18n( "Progress" ) );
    mSyncEditorListView->addColumn( i18n( "Status" ) );
    mSyncEditorListView->setSorting( -1, false );
    mSyncEditorListView->setColumnWidth( 0, 100 );
    mSyncEditorListView->setColumnWidth( 1, 100 );
    mSyncEditorListView->setColumnWidth( 2, 200 );

    mSyncEditorListView->setColumnWidthMode(0, QListView::Maximum);
    mSyncEditorListView->setColumnWidthMode(2, QListView::Maximum);

    topLayout->addWidget( mSyncEditorListView );

    QBoxLayout* bottomLayout = new QHBoxLayout( topLayout, KDialog::spacingHint(), "bottomLayout");
    bottomLayout->addStretch();

    KPushButton* pbClose = new KPushButton( KStdGuiItem::close(), this );
    pbClose->setText( i18n( "Close" ) );
    bottomLayout->addWidget( pbClose );

    connect(pbClose, SIGNAL(clicked()), this, SLOT(close())  );
}

void IMAPProgressDialog::clear()
{
    QListViewItem* item;
    while( ( item = mSyncEditorListView->firstChild() ) != 0 ) delete item;
    mPreviousItem = 0;
}

/* retrieves the info needed to update the list view items and it's  progress bar */

void IMAPProgressDialog::syncState( const QString& folderName,
				    int progress, const QString& syncStatus )
{
  ProgressListViewItem* item = 0;
  for( QListViewItem* it = mSyncEditorListView->firstChild(); it != 0; it = it->nextSibling() ) {
    if( folderName == it->text(0) ) {
      item = static_cast<ProgressListViewItem*>(it);
      break;
    }
  }

  if ( progress > 100 )
    progress = 100;

  if( item ) {
    item->setProgress( progress );
    item->setText( 2, syncStatus );
  } else {
    mPreviousItem = new ProgressListViewItem( 1, progress,
                                              mSyncEditorListView,
                                              mPreviousItem, folderName,
                                              QString::null, syncStatus );
  }
}

void IMAPProgressDialog::closeEvent( QCloseEvent* e )
{
  e->accept();
  hide();
}


/*
 *  Destructor
 */
IMAPProgressDialog::~IMAPProgressDialog()
{
    // no need to delete child widgets.
}


};

#include "imapprogressdialog.moc"
