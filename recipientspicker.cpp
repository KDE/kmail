/*
    This file is part of KMail.

    Copyright (c) 2005 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
    
    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "recipientspicker.h"

#include <klistview.h>
#include <klistviewsearchline.h>
#include <klocale.h>
#include <kabc/stdaddressbook.h>
#include <kabc/resource.h>
#include <kdialog.h>

#include <qlayout.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlabel.h>

RecipientItem::RecipientItem()
{
}

void RecipientItem::setAddressee( const KABC::Addressee &a )
{
  mAddressee = a;
}

KABC::Addressee RecipientItem::addressee() const
{
  return mAddressee;
}

QString RecipientItem::recipient() const
{
  QString r = mAddressee.fullEmail();
  return r;
}

void RecipientItem::setRecipientType( const QString &type )
{
  mType = type;
}

QString RecipientItem::recipientType() const
{
  return mType;
}


RecipientViewItem::RecipientViewItem( RecipientItem *item, KListView *listView )
  : KListViewItem( listView ), mRecipientItem( item )
{
  KABC::Addressee a = item->addressee();
  
  setText( 0, item->recipientType() );
  setText( 1, a.formattedName() );
  setText( 2, a.preferredEmail() );
}

RecipientItem *RecipientViewItem::recipientItem() const
{
  return mRecipientItem;
}


RecipientsCollection::RecipientsCollection()
{
}

void RecipientsCollection::setTitle( const QString &title )
{
  mTitle = title;
}

QString RecipientsCollection::title() const
{
  return mTitle;
}

void RecipientsCollection::addItem( RecipientItem *item )
{
  mItems.append( item );
}

RecipientItem::List RecipientsCollection::items() const
{
  return mItems;
}


RecipientsPicker::RecipientsPicker( QWidget *parent )
  : QWidget( parent )
{
  setCaption( i18n("Select Recipient") );

  QBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setSpacing( KDialog::spacingHint() );
  topLayout->setMargin( KDialog::marginHint() );

  QBoxLayout *resLayout = new QHBoxLayout( topLayout );
  
  QLabel *label = new QLabel( i18n("AddressBook:"), this );
  resLayout->addWidget( label );
  
  mCollectionCombo = new QComboBox( this );
  resLayout->addWidget( mCollectionCombo );
  connect( mCollectionCombo, SIGNAL( highlighted( int ) ),
    SLOT( updateList() ) );
  connect( mCollectionCombo, SIGNAL( activated( int ) ),
    SLOT( updateList() ) );
  
  QBoxLayout *searchLayout = new QHBoxLayout( topLayout );
  
  label = new QLabel( i18n("Search:"), this );
  searchLayout->addWidget( label );
  
  mRecipientList = new KListView( this );
  mRecipientList->setAllColumnsShowFocus( true );
  topLayout->addWidget( mRecipientList );
  mRecipientList->addColumn( i18n("->") );
  mRecipientList->addColumn( i18n("Name") );
  mRecipientList->addColumn( i18n("Email") );
  connect( mRecipientList, SIGNAL( doubleClicked( QListViewItem *,
    const QPoint &, int ) ), SLOT( slotPicked( QListViewItem * ) ) );
  connect( mRecipientList, SIGNAL( returnPressed( QListViewItem * ) ),
    SLOT( slotPicked( QListViewItem * ) ) );

  mSearchLine = new KListViewSearchLine( this, mRecipientList );
  searchLayout->addWidget( mSearchLine );

  QBoxLayout *buttonLayout = new QHBoxLayout( topLayout );

  buttonLayout->addStretch( 1 );

  QPushButton *button = new QPushButton( i18n("&Ok"), this );
  buttonLayout->addWidget( button );
  connect( button, SIGNAL( clicked() ), SLOT( slotOk() ) );

  button = new QPushButton( i18n("&Cancel"), this );
  buttonLayout->addWidget( button );
  connect( button, SIGNAL( clicked() ), SLOT( close() ) );

  initCollections();

  mCollectionCombo->setCurrentItem( 0 );

  updateList();
}

void RecipientsPicker::initCollections()
{
  KABC::StdAddressBook *addressbook = KABC::StdAddressBook::self();
  
  QMap<KABC::Resource *,RecipientsCollection *> collectionMap;

  QPtrList<KABC::Resource> resources = addressbook->resources();
  KABC::Resource *res;
  for( res = resources.first(); res; res = resources.next() ) {
    RecipientsCollection *collection = new RecipientsCollection;
    collectionMap.insert( res, collection );
    collection->setTitle( res->resourceName() );
  }
  
  mAllRecipients = new RecipientsCollection;
  mAllRecipients->setTitle( i18n("All") );
  
  KABC::AddressBook::Iterator it;
  for( it = addressbook->begin(); it != addressbook->end(); ++it ) {
    RecipientItem *item = new RecipientItem;
    item->setAddressee( *it );
    mAllRecipients->addItem( item );

    QMap<KABC::Resource *,RecipientsCollection *>::ConstIterator collIt;
    collIt = collectionMap.find( it->resource() );
    if ( collIt != collectionMap.end() ) {
      (*collIt)->addItem( item );
    }
  }

  int index = 0;

  insertCollection( mAllRecipients, index++ );
  
  QMap<KABC::Resource *,RecipientsCollection *>::ConstIterator it2;
  for( it2 = collectionMap.begin(); it2 != collectionMap.end(); ++it2 ) {
    insertCollection( *it2, index++ );
  }
}

void RecipientsPicker::insertCollection( RecipientsCollection *coll, int index )
{
  mCollectionCombo->insertItem( coll->title(), index );
  mCollectionMap.insert( index, coll );
}

void RecipientsPicker::setRecipients( const Recipient::List &recipients )
{
  RecipientItem::List allRecipients = mAllRecipients->items();
  RecipientItem::List::ConstIterator itAll;
  for( itAll = allRecipients.begin(); itAll != allRecipients.end(); ++itAll ) {
    (*itAll)->setRecipientType( QString::null );
  }

  Recipient::List::ConstIterator it;
  for( it = recipients.begin(); it != recipients.end(); ++it ) {
    for( itAll = allRecipients.begin(); itAll != allRecipients.end(); ++itAll ) {
      if ( (*itAll)->recipient() == (*it).email() ) {
        (*itAll)->setRecipientType( (*it).typeLabel() );
      }
    }
  }

  updateList();
}

void RecipientsPicker::updateList()
{
  mRecipientList->clear();
  
  RecipientsCollection *coll = mCollectionMap[ mCollectionCombo->currentItem() ];
  
  RecipientItem::List items = coll->items();
  RecipientItem::List::ConstIterator it;
  for( it = items.begin(); it != items.end(); ++it ) {
    new RecipientViewItem( *it, mRecipientList );
  }

  mSearchLine->updateSearch();
}

void RecipientsPicker::slotOk()
{
  slotPicked( mRecipientList->currentItem() );
}

void RecipientsPicker::slotPicked( QListViewItem *viewItem )
{
  kdDebug() << "RecipientsPicker::slotPicked()" << endl;

  RecipientViewItem *item = static_cast<RecipientViewItem *>( viewItem );
  if ( item ) {
    RecipientItem *i = item->recipientItem();
    emit pickedRecipient( i->recipient() );
  }
  close();
}

#include "recipientspicker.moc"
