/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <config.h>

#include "kmatmlistview.h"
#include <qcheckbox.h>
#include <q3header.h>

KMAtmListViewItem::KMAtmListViewItem( Q3ListView *parent )
  : QObject(),
    Q3ListViewItem( parent )
{
  mCBCompress = new QCheckBox( listView()->viewport() );
  mCBEncrypt = new QCheckBox( listView()->viewport() );
  mCBSign = new QCheckBox( listView()->viewport() );
  mCBCompress->setVisible( true );
  updateAllCheckBoxes();

  connect( mCBCompress, SIGNAL( clicked() ), this, SLOT( slotCompress() ) );
  connect( listView()->header(), SIGNAL( sizeChange(int, int, int) ),
           SLOT( slotHeaderChange( int, int, int ) ) );
  connect( listView()->header(), SIGNAL( indexChange(int, int, int) ),
           SLOT( slotHeaderChange( int, int, int ) ) );
  connect( listView()->header(), SIGNAL( clicked( int ) ), SLOT( slotHeaderClick( int ) ) );
}

KMAtmListViewItem::~KMAtmListViewItem()
{
  delete mCBEncrypt;
  mCBEncrypt = 0;
  delete mCBSign;
  mCBSign = 0;
  delete mCBCompress;
  mCBCompress = 0;
}

void KMAtmListViewItem::updateCheckBox( int headerSection, QCheckBox *cb )
{
  // Calculate some values to determine the x-position where the checkbox
  // will be drawn
  int sectionWidth = listView()->header()->sectionSize( headerSection );
  int sectionPos = listView()->header()->sectionPos( headerSection );
  int sectionOffset = sectionWidth / 2 - height() / 4;

  //Resize and move the checkbox
  cb->resize( sectionWidth - sectionOffset - 1, height() - 2 );
  listView()->moveChild( cb, sectionPos + sectionOffset, itemPos() + 1 );

  //Set the correct background color
  if ( isSelected() ) {
    cb->setBackgroundRole( QPalette::Highlight );
  } else {
    cb->setBackgroundRole( QPalette::QPalette::Base );
  }
}

void KMAtmListViewItem::updateAllCheckBoxes()
{
  updateCheckBox( 4, mCBCompress );
  updateCheckBox( 5, mCBEncrypt );
  updateCheckBox( 6, mCBSign );
}

// Each time a cell is about to be painted, the item's checkboxes are updated
// as well. This is necessary to keep the positions of the checkboxes
// up-to-date. The signals which are, in the constructor of this class,
// connected to the update slots are not sufficient because unfortunately,
// Qt does not provide a signal for changed item positions, e.g. during
// deleting or adding items. The problem with this is that this function
// does not catch updates which are off-screen, which means under some
// circumstances checkboxes have invalid positions. This should not happen
// anymore, but was the cause of bug 113458. Therefore, both the signals
// connected in the constructor and this function are necessary to keep the
// checkboxes' positions in sync, and hopefully is enough.
void KMAtmListViewItem::paintCell ( QPainter *p, const QColorGroup &cg,
                                    int column, int width, int align )
{
  switch ( column ) {
    case 4: updateCheckBox( 4, mCBCompress ); break;
    case 5: updateCheckBox( 5, mCBEncrypt ); break;
    case 6: updateCheckBox( 6, mCBSign ); break;
  }

  Q3ListViewItem::paintCell( p, cg, column, width, align );
}

int KMAtmListViewItem::compare( Q3ListViewItem *i, int col, bool ascending ) const
{
  if ( col != 1 ) {
    return Q3ListViewItem::compare( i, col, ascending );
  }

  return mAttachmentSize -
    (static_cast<KMAtmListViewItem*>(i))->mAttachmentSize;
}

void KMAtmListViewItem::enableCryptoCBs( bool on )
{
  // Show/Hide the appropriate checkboxes.
  // This should not be necessary because the caller hides the columns
  // containing the checkboxes anyway.
  mCBEncrypt->setVisible( on );
  mCBSign->setVisible( on );
}

void KMAtmListViewItem::setEncrypt( bool on )
{
  if ( mCBEncrypt ) {
    mCBEncrypt->setChecked( on );
  }
}

bool KMAtmListViewItem::isEncrypt()
{
  if ( mCBEncrypt ) {
    return mCBEncrypt->isChecked();
  } else {
    return false;
  }
}

void KMAtmListViewItem::setSign( bool on )
{
  if ( mCBSign ) {
    mCBSign->setChecked( on );
  }
}

bool KMAtmListViewItem::isSign()
{
  if ( mCBSign ) {
    return mCBSign->isChecked();
  } else {
    return false;
  }
}

void KMAtmListViewItem::setCompress( bool on )
{
  mCBCompress->setChecked( on );
}

bool KMAtmListViewItem::isCompress()
{
  return mCBCompress->isChecked();
}

void KMAtmListViewItem::slotCompress()
{
  if ( mCBCompress->isChecked() ) {
    emit compress( itemPos() );
  } else {
        emit uncompress( itemPos() );
  }
}

// Update the item's checkboxes when the position of those change
// due to different column positions
void KMAtmListViewItem::slotHeaderChange( int, int, int )
{
  updateAllCheckBoxes();
}

// Update the item's checkboxes when the list is being sorted
void KMAtmListViewItem::slotHeaderClick( int )
{
  updateAllCheckBoxes();
}

#include "kmatmlistview.moc"
