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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "recipientspicker.h"

#include "globalsettings.h"

#include <libkdepim/recentaddresses.h>

#ifndef KDEPIM_NEW_DISTRLISTS
#include <kabc/distributionlist.h>
#endif

#include <klistview.h>
#include <klocale.h>
#include <kabc/resource.h>
#include <kiconloader.h>
#include <kdialog.h>
#include <kwin.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qlabel.h>

#ifdef KDEPIM_NEW_DISTRLISTS
RecipientItem::RecipientItem( KABC::AddressBook *ab )
  : mAddressBook( ab )
{
}
#else
RecipientItem::RecipientItem()
  : mDistributionList( 0 )
{
}
#endif

#ifdef KDEPIM_NEW_DISTRLISTS
void RecipientItem::setDistributionList( const KPIM::DistributionList &list )
{
  mDistributionList = list;

  mIcon = KGlobal::iconLoader()->loadIcon( "kdmconfig", KIcon::Small );

  mKey = "D" + list.name();
}
#else
void RecipientItem::setDistributionList( KABC::DistributionList *list )
{
  mDistributionList = list;

  mIcon = KGlobal::iconLoader()->loadIcon( "kdmconfig", KIcon::Small );

  mKey = "D" + list->name();
}
#endif

void RecipientItem::setAddressee( const KABC::Addressee &a,
  const QString &email )
{
  mAddressee = a;
  mEmail = email;

  QImage img = a.photo().data();
  if ( !img.isNull() )
    mIcon = img.smoothScale( 20, 20, QImage::ScaleMin );
  else
    mIcon = KGlobal::iconLoader()->loadIcon( "personal", KIcon::Small );

  mKey = "A" + a.preferredEmail();
}

QPixmap RecipientItem::icon() const
{
  return mIcon;
}

QString RecipientItem::name() const
{
#ifdef KDEPIM_NEW_DISTRLISTS
  if ( !mAddressee.isEmpty() ) return mAddressee.realName();
  else if ( !mDistributionList.isEmpty() ) return mDistributionList.name();
  else return QString::null;
#else
  if ( !mAddressee.isEmpty() ) return mAddressee.realName();
  else if ( mDistributionList ) return mDistributionList->name();
  else return QString::null;
#endif
}

QString RecipientItem::email() const
{
#ifdef KDEPIM_NEW_DISTRLISTS
  if ( mAddressee.isEmpty() && !mDistributionList.isEmpty() ) {
    int count = mDistributionList.entries( mAddressBook ).count();
    return i18n( "1 email address", "%n email addresses", count );
  } else {
    return mEmail;
  }
#else
  if ( mAddressee.isEmpty() &&  mDistributionList ) {
    int count = mDistributionList->entries().count();
    return i18n( "1 email address", "%n email addresses", count );
  } else {
    return mEmail;
  }
#endif
  return QString::null;
}

#ifdef KDEPIM_NEW_DISTRLISTS
QString RecipientItem::recipient() const
{
  QString r;
  if ( !mAddressee.isEmpty() ) r = mAddressee.fullEmail( mEmail );
  else if ( !mDistributionList.isEmpty() ) r = mDistributionList.name();
  return r;
}
#else
QString RecipientItem::recipient() const
{
  QString r;
  if ( !mAddressee.isEmpty() ) r = mAddressee.fullEmail( mEmail );
  else if ( mDistributionList ) r = mDistributionList->name();
  return r;
}
#endif

QString RecipientItem::toolTip() const
{
  QString txt = "<qt>";

  if ( !mAddressee.isEmpty() ) {
    if ( !mAddressee.realName().isEmpty() ) {
      txt += mAddressee.realName() + "<br/>";
    }
    txt += "<b>" + mEmail + "</b>";
#ifdef KDEPIM_NEW_DISTRLISTS
  } else if ( !mDistributionList.isEmpty() ) {
    txt += "<b>" + i18n("Distribution List %1")
      .arg( mDistributionList.name() ) + "</b>";
    txt += "<ul>";
    KPIM::DistributionList::Entry::List entries = mDistributionList.entries( mAddressBook );
    KPIM::DistributionList::Entry::List::ConstIterator it;
    for( it = entries.begin(); it != entries.end(); ++it ) {
      txt += "<li>";
      txt += (*it).addressee.realName() + " ";
      txt += "<em>";
      if ( (*it).email.isEmpty() ) txt += (*it).addressee.preferredEmail();
      else txt += (*it).email;
      txt += "</em>";
      txt += "<li/>";
    }
    txt += "</ul>";
  }
#else
  } else if ( mDistributionList ) {
    txt += "<b>" + i18n("Distribution List %1")
      .arg( mDistributionList->name() ) + "</b>";
    txt += "<ul>";
    KABC::DistributionList::Entry::List entries = mDistributionList->entries();
    KABC::DistributionList::Entry::List::ConstIterator it;
    for( it = entries.begin(); it != entries.end(); ++it ) {
      txt += "<li>";
      txt += (*it).addressee.realName() + " ";
      txt += "<em>";
      if ( (*it).email.isEmpty() ) txt += (*it).addressee.preferredEmail();
      else txt += (*it).email;
      txt += "</em>";
      txt += "<li/>";
    }
    txt += "</ul>";
  }
#endif

  return txt;
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
  setText( 0, item->recipientType() );
  setText( 1, item->name() );
  setText( 2, item->email() );

  setPixmap( 1, item->icon() );
}

RecipientItem *RecipientViewItem::recipientItem() const
{
  return mRecipientItem;
}


RecipientsListToolTip::RecipientsListToolTip( QWidget *parent,
  KListView *listView )
  : QToolTip( parent )
{
  mListView = listView;
}

void RecipientsListToolTip::maybeTip( const QPoint & pos )
{
  QRect r;
  QListViewItem *item = mListView->itemAt( pos );
  RecipientViewItem *i = static_cast<RecipientViewItem *>( item );

  if( item ) {
    r = mListView->itemRect( item );
    QString tipText( i->recipientItem()->toolTip() );
    if ( !tipText.isEmpty() ) {
      tip( r, tipText );
    }
  }
}


RecipientsCollection::RecipientsCollection()
{
}

RecipientsCollection::~RecipientsCollection()
{
  clear();
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

  mKeyMap.insert( item->key(), item );
}

RecipientItem::List RecipientsCollection::items() const
{
  return mItems;
}

bool RecipientsCollection::hasEquivalentItem( RecipientItem *item ) const
{
  return mKeyMap.find( item->key() ) != mKeyMap.end();
}

void RecipientsCollection::clear()
{
  mKeyMap.clear();
}

void RecipientsCollection::deleteAll()
{
  QMap<QString, RecipientItem *>::ConstIterator it;
  for( it = mKeyMap.begin(); it != mKeyMap.end(); ++it ) {
    delete *it;
  }
  clear();
}


SearchLine::SearchLine( QWidget *parent, KListView *listView )
  : KListViewSearchLine( parent, listView )
{
}

void SearchLine::keyPressEvent( QKeyEvent *ev )
{
  if ( ev->key() == Key_Down ) emit downPressed();

  KListViewSearchLine::keyPressEvent( ev );
}


RecipientsPicker::RecipientsPicker( QWidget *parent )
  : QDialog( parent, "RecipientsPicker" )
#ifndef KDEPIM_NEW_DISTRLISTS
    , mDistributionListManager( 0 )
#endif
{
//  KWin::setType( winId(), NET::Dock );

  setCaption( i18n("Select Recipient") );

  QBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setSpacing( KDialog::spacingHint() );
  topLayout->setMargin( KDialog::marginHint() );

  QBoxLayout *resLayout = new QHBoxLayout( topLayout );

  QLabel *label = new QLabel( i18n("Address book:"), this );
  resLayout->addWidget( label );

  mCollectionCombo = new QComboBox( this );
  resLayout->addWidget( mCollectionCombo );
  resLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

  connect( mCollectionCombo, SIGNAL( highlighted( int ) ),
    SLOT( updateList() ) );
  connect( mCollectionCombo, SIGNAL( activated( int ) ),
    SLOT( updateList() ) );

  QBoxLayout *searchLayout = new QHBoxLayout( topLayout );

  QToolButton *button = new QToolButton( this );
  button->setIconSet( KGlobal::iconLoader()->loadIconSet(
              KApplication::reverseLayout() ? "clear_left":"locationbar_erase", KIcon::Small, 0 ) );
  searchLayout->addWidget( button );
  connect( button, SIGNAL( clicked() ), SLOT( resetSearch() ) );

  label = new QLabel( i18n("&Search:"), this );
  searchLayout->addWidget( label );

  mRecipientList = new KListView( this );
  mRecipientList->setSelectionMode( QListView::Extended );
  mRecipientList->setAllColumnsShowFocus( true );
  mRecipientList->setFullWidth( true );
  topLayout->addWidget( mRecipientList );
  mRecipientList->addColumn( i18n("->") );
  mRecipientList->addColumn( i18n("Name") );
  mRecipientList->addColumn( i18n("Email") );
  connect( mRecipientList, SIGNAL( doubleClicked( QListViewItem *,
    const QPoint &, int ) ), SLOT( slotPicked() ) );
  connect( mRecipientList, SIGNAL( returnPressed( QListViewItem * ) ),
    SLOT( slotPicked() ) );

  new RecipientsListToolTip( mRecipientList->viewport(), mRecipientList );

  mSearchLine = new SearchLine( this, mRecipientList );
  searchLayout->addWidget( mSearchLine );
  label->setBuddy( label );
  connect( mSearchLine, SIGNAL( downPressed() ), SLOT( setFocusList() ) );

  QBoxLayout *buttonLayout = new QHBoxLayout( topLayout );

  buttonLayout->addStretch( 1 );

  mToButton = new QPushButton( i18n("Add as To"), this );
  buttonLayout->addWidget( mToButton );
  connect( mToButton, SIGNAL( clicked() ), SLOT( slotToClicked() ) );

  mCcButton = new QPushButton( i18n("Add as CC"), this );
  buttonLayout->addWidget( mCcButton );
  connect( mCcButton, SIGNAL( clicked() ), SLOT( slotCcClicked() ) );

  mBccButton = new QPushButton( i18n("Add as BCC"), this );
  buttonLayout->addWidget( mBccButton );
  connect( mBccButton, SIGNAL( clicked() ), SLOT( slotBccClicked() ) );
  // BCC isn't commonly used, so hide it for now
  //mBccButton->hide();

  QPushButton *closeButton = new QPushButton( i18n("&Cancel"), this );
  buttonLayout->addWidget( closeButton );
  connect( closeButton, SIGNAL( clicked() ), SLOT( close() ) );

  {
    using namespace KABC;
    mAddressBook = KABC::StdAddressBook::self( true );
    connect( mAddressBook, SIGNAL( addressBookChanged( AddressBook * ) ),
             this, SLOT( insertAddressBook( AddressBook * ) ) );
  }

  initCollections();

  mCollectionCombo->setCurrentItem( 0 );

  updateList();

  mSearchLine->setFocus();

  readConfig();

  setTabOrder( mCollectionCombo, mSearchLine );
  setTabOrder( mSearchLine, mRecipientList );
  setTabOrder( closeButton, mCollectionCombo );
}

RecipientsPicker::~RecipientsPicker()
{
  writeConfig();

#ifndef KDEPIM_NEW_DISTRLISTS
  delete mDistributionListManager;
#endif

  mAllRecipients->deleteAll();

  QMap<int,RecipientsCollection *>::ConstIterator it;
  for( it = mCollectionMap.begin(); it != mCollectionMap.end(); ++it ) {
    delete *it;
  }
}

void RecipientsPicker::initCollections()
{
  mAllRecipients = new RecipientsCollection;
  mAllRecipients->setTitle( i18n("All") );
  insertCollection( mAllRecipients );

  insertAddressBook( mAddressBook );

  insertDistributionLists();

  insertRecentAddresses();

  mSelectedRecipients = new RecipientsCollection;
  mSelectedRecipients->setTitle( i18n("Selected Recipients") );
  insertCollection( mSelectedRecipients );
}

void RecipientsPicker::insertAddressBook( KABC::AddressBook *addressbook )
{
  QMap<KABC::Resource *,RecipientsCollection *> collectionMap;

  QPtrList<KABC::Resource> resources = addressbook->resources();
  KABC::Resource *res;
  for( res = resources.first(); res; res = resources.next() ) {
    RecipientsCollection *collection = new RecipientsCollection;
    collectionMap.insert( res, collection );
    collection->setTitle( res->resourceName() );
  }

  QMap<QString,RecipientsCollection *> categoryMap;

  KABC::AddressBook::Iterator it;
  for( it = addressbook->begin(); it != addressbook->end(); ++it ) {
    QStringList emails = (*it).emails();
    QStringList::ConstIterator it3;
    for( it3 = emails.begin(); it3 != emails.end(); ++it3 ) {
#ifdef KDEPIM_NEW_DISTRLISTS
      RecipientItem *item = new RecipientItem( mAddressBook );
#else
      RecipientItem *item = new RecipientItem;
#endif
      item->setAddressee( *it, *it3 );
      mAllRecipients->addItem( item );

      QMap<KABC::Resource *,RecipientsCollection *>::ConstIterator collIt;
      collIt = collectionMap.find( it->resource() );
      if ( collIt != collectionMap.end() ) {
        (*collIt)->addItem( item );
      }

      QStringList categories = (*it).categories();
      QStringList::ConstIterator catIt;
      for( catIt = categories.begin(); catIt != categories.end(); ++catIt ) {
        QMap<QString, RecipientsCollection *>::ConstIterator catMapIt;
        catMapIt = categoryMap.find( *catIt );
        RecipientsCollection *collection;
        if ( catMapIt == categoryMap.end() ) {
          collection = new RecipientsCollection;
          collection->setTitle( *catIt );
          categoryMap.insert( *catIt, collection );
        } else {
          collection = *catMapIt;
        }
        collection->addItem( item );
      }
    }
  }

  QMap<KABC::Resource *,RecipientsCollection *>::ConstIterator it2;
  for( it2 = collectionMap.begin(); it2 != collectionMap.end(); ++it2 ) {
    insertCollection( *it2 );
  }

  QMap<QString, RecipientsCollection *>::ConstIterator it3;
  for( it3 = categoryMap.begin(); it3 != categoryMap.end(); ++it3 ) {
    insertCollection( *it3 );
  }
  
  updateList();
}

void RecipientsPicker::insertDistributionLists()
{
  RecipientsCollection *collection = new RecipientsCollection;
  collection->setTitle( i18n("Distribution Lists") );

#ifdef KDEPIM_NEW_DISTRLISTS
  QValueList<KPIM::DistributionList> lists = KPIM::DistributionList::allDistributionLists( mAddressBook );
  for ( uint i = 0; i < lists.count(); ++i ) {
    RecipientItem *item = new RecipientItem( mAddressBook );
    item->setDistributionList( lists[ i ] );
    mAllRecipients->addItem( item );
    collection->addItem( item );
  }
#else
  delete mDistributionListManager;
  mDistributionListManager =
    new KABC::DistributionListManager( KABC::StdAddressBook::self( true ) );

  mDistributionListManager->load();

  QStringList lists = mDistributionListManager->listNames();

  QStringList::Iterator listIt;
  for ( listIt = lists.begin(); listIt != lists.end(); ++listIt ) {
    KABC::DistributionList *list = mDistributionListManager->list( *listIt );
    RecipientItem *item = new RecipientItem;
    item->setDistributionList( list );
    mAllRecipients->addItem( item );
    collection->addItem( item );
  }
#endif

  insertCollection( collection );
}

void RecipientsPicker::insertRecentAddresses()
{
  RecipientsCollection *collection = new RecipientsCollection;
  collection->setTitle( i18n("Recent Addresses") );

  KConfig config( "kmailrc" );
  KABC::Addressee::List recents =
    KRecentAddress::RecentAddresses::self( &config )->kabcAddresses();

  KABC::Addressee::List::ConstIterator it;
  for( it = recents.begin(); it != recents.end(); ++it ) {
#ifdef KDEPIM_NEW_DISTRLISTS
    RecipientItem *item = new RecipientItem( mAddressBook );
#else
    RecipientItem *item = new RecipientItem;
#endif
    item->setAddressee( *it, (*it).preferredEmail() );
    if ( !mAllRecipients->hasEquivalentItem( item ) ) {
      mAllRecipients->addItem( item );
    }
    collection->addItem( item );
  }

  insertCollection( collection );
}

void RecipientsPicker::insertCollection( RecipientsCollection *coll )
{
  int index = mCollectionMap.count();

  kdDebug() << "RecipientsPicker::insertCollection() " << coll->title()
    << "  index: " << index << endl;

  mCollectionCombo->insertItem( coll->title(), index );
  mCollectionMap.insert( index, coll );
}

void RecipientsPicker::updateRecipient( const Recipient &recipient )
{
  RecipientItem::List allRecipients = mAllRecipients->items();
  RecipientItem::List::ConstIterator itAll;
  for( itAll = allRecipients.begin(); itAll != allRecipients.end(); ++itAll ) {
    if ( (*itAll)->recipient() == recipient.email() ) {
      (*itAll)->setRecipientType( recipient.typeLabel() );
    }
  }
  updateList();
}

void RecipientsPicker::setRecipients( const Recipient::List &recipients )
{
  RecipientItem::List allRecipients = mAllRecipients->items();
  RecipientItem::List::ConstIterator itAll;
  for( itAll = allRecipients.begin(); itAll != allRecipients.end(); ++itAll ) {
    (*itAll)->setRecipientType( QString::null );
  }

  mSelectedRecipients->clear();

  Recipient::List::ConstIterator it;
  for( it = recipients.begin(); it != recipients.end(); ++it ) {
    RecipientItem *item = 0;
    for( itAll = allRecipients.begin(); itAll != allRecipients.end(); ++itAll ) {
      if ( (*itAll)->recipient() == (*it).email() ) {
        (*itAll)->setRecipientType( (*it).typeLabel() );
        item = *itAll;
      }
    }
    if ( !item ) {
      KABC::Addressee a;
      QString name;
      QString email;
      KABC::Addressee::parseEmailAddress( (*it).email(), name, email );
      a.setNameFromString( name );
      a.insertEmail( email );

#ifdef KDEPIM_NEW_DISTRLISTS
      item = new RecipientItem( mAddressBook );
#else
      item = new RecipientItem;
#endif
      item->setAddressee( a, a.preferredEmail() );
      item->setRecipientType( (*it).typeLabel() );
      mAllRecipients->addItem( item );
    }
    mSelectedRecipients->addItem( item );
  }

  updateList();
}

void RecipientsPicker::setDefaultButton( QPushButton *button )
{
//  button->setText( "<qt><b>" + button->text() + "</b></qt>" );
  button->setDefault( true );
}

void RecipientsPicker::setDefaultType( Recipient::Type type )
{
  mDefaultType = type;

  if ( type == Recipient::To ) {
    setDefaultButton( mToButton );
  } else if ( type == Recipient::Cc ) {
    setDefaultButton( mCcButton );
  } else if ( type == Recipient::Bcc ) {
    setDefaultButton( mBccButton );
  }
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

void RecipientsPicker::slotToClicked()
{
  pick( Recipient::To );
}

void RecipientsPicker::slotCcClicked()
{
  pick( Recipient::Cc );
}

void RecipientsPicker::slotBccClicked()
{
  pick( Recipient::Bcc );
}

void RecipientsPicker::slotPicked( QListViewItem *viewItem )
{
  RecipientViewItem *item = static_cast<RecipientViewItem *>( viewItem );
  if ( item ) {
    RecipientItem *i = item->recipientItem();
    emit pickedRecipient( Recipient( i->recipient(), Recipient::Undefined ) );
  }
  close();
}

void RecipientsPicker::slotPicked()
{
  pick( mDefaultType );
}

void RecipientsPicker::pick( Recipient::Type type )
{
  kdDebug() << "RecipientsPicker::pick " << int( type ) << endl;

  int count = 0;
  QListViewItemIterator it( mRecipientList , 
            QListViewItemIterator::Visible | QListViewItemIterator::Selected );
  for ( ; it.current(); ++it )
      ++count;

  if ( count > GlobalSettings::self()->maximumRecipients() ) {
    KMessageBox::sorry( this,
        i18n("You selected 1 recipient. The maximum supported number of "
             "recipients is %1. Please adapt the selection.",
             "You selected %n recipients. The maximum supported number of "
             "recipients is %1. Please adapt the selection.", count)
      .arg( GlobalSettings::self()->maximumRecipients() ) );
    return;
  }

  it = QListViewItemIterator( mRecipientList , 
            QListViewItemIterator::Visible | QListViewItemIterator::Selected );
  for ( ; it.current(); ++it ) {
    RecipientViewItem *item = static_cast<RecipientViewItem *>( it.current() );
    if ( item ) {
      RecipientItem *i = item->recipientItem();
      Recipient r = i->recipient();
      r.setType( type );
      emit pickedRecipient( r );
    }
  }
  close();
}

void RecipientsPicker::keyPressEvent( QKeyEvent *ev )
{
  if ( ev->key() == Key_Escape ) close();

  QWidget::keyPressEvent( ev );
}

void RecipientsPicker::readConfig()
{
  KConfig *cfg = KGlobal::config();
  cfg->setGroup( "RecipientsPicker" );
  QSize size = cfg->readSizeEntry( "Size" );
  if ( !size.isEmpty() ) {
    resize( size );
  }
  int currentCollection = cfg->readNumEntry( "CurrentCollection", -1 );
  if ( currentCollection >= 0 &&
       currentCollection < mCollectionCombo->count() ) {
    mCollectionCombo->setCurrentItem( currentCollection );
  }
}

void RecipientsPicker::writeConfig()
{
  KConfig *cfg = KGlobal::config();
  cfg->setGroup( "RecipientsPicker" );
  cfg->writeEntry( "Size", size() );
  cfg->writeEntry( "CurrentCollection", mCollectionCombo->currentItem() );
}

void RecipientsPicker::setFocusList()
{
  mRecipientList->setFocus();
}

void RecipientsPicker::resetSearch()
{
  mSearchLine->setText( QString::null );
}

#include "recipientspicker.moc"
