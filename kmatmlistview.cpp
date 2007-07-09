/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>
  Copyright (c) 2007 Thomas McGuire <Thomas.McGuire@gmx.net>

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

#include "kmatmlistview.h"


#include <QWidget>
#include <QCheckBox>
#include <QHBoxLayout>

#include "attachmentlistview.h"

KMAtmListViewItem::KMAtmListViewItem( KMail::AttachmentListView *parent,
                                      KMMessagePart* attachment )
  : QObject(),
    QTreeWidgetItem( parent ),
    mAttachment( attachment ),
    mParent( parent )

{
  mCBCompress = addCheckBox( 4 );
  mCBEncrypt = addCheckBox( 5 );
  mCBSign = addCheckBox( 6 );

  connect( mCBCompress, SIGNAL( clicked() ), this, SLOT( slotCompress() ) );
}

QCheckBox* KMAtmListViewItem::addCheckBox( int column )
{
  // We can not call setItemWidget() on the checkbox directly, because then
  // the checkbox would be left-aligned. Therefore we create a helper widget
  // with a layout to align the checkbox to the center.
  QWidget *w = new QWidget();
  QCheckBox *c = new QCheckBox();
  QHBoxLayout *l = new QHBoxLayout();
  l->insertWidget( 0, c, 0, Qt::AlignHCenter );
  w->setBackgroundRole( QPalette::Base );
  c->setBackgroundRole( QPalette::Base );
  w->setLayout( l );
  l->setMargin( 0 );
  l->setSpacing( 0 );
  w->show();
  treeWidget()->setItemWidget( this, column, w );
  return c;
}

KMAtmListViewItem::~KMAtmListViewItem()
{
  // The checkboxes should be automatically deleted by the treewidget
  mCBEncrypt = 0;
  mCBSign = 0;
  mCBCompress = 0;
}

bool KMAtmListViewItem::operator < ( const QTreeWidgetItem & other ) const
{
  if ( treeWidget()->sortColumn() != 1 )
    return QTreeWidgetItem::operator < ( other );
  else
    return mAttachmentSize <
        (static_cast<const KMAtmListViewItem*>( &other ))->mAttachmentSize;
}

void KMAtmListViewItem::setEncrypt( bool on )
{
  if ( mCBEncrypt ) {
    mCBEncrypt->setChecked( on );
  }
}

bool KMAtmListViewItem::isEncrypt() const
{
  if ( mParent->areCryptoCBsEnabled() ) {
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

bool KMAtmListViewItem::isSign() const
{
  if ( mParent->areCryptoCBsEnabled() ) {
    return mCBSign->isChecked();
  } else {
    return false;
  }
}

void KMAtmListViewItem::setCompress( bool on )
{
  mCBCompress->setChecked( on );
}

bool KMAtmListViewItem::isCompress() const
{
  return mCBCompress->isChecked();
}

void KMAtmListViewItem::slotCompress()
{
  if ( mCBCompress->isChecked() ) {
    emit compress( this );
  } else {
    emit uncompress( this );
  }
}

KMMessagePart* KMAtmListViewItem::attachment() const
{
  return mAttachment;
}


#include "kmatmlistview.moc"
