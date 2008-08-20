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
#include <libkdepim/ldapsearchdialog.h>

#include <libemailfunctions/email.h>

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
void RecipientItem::setDistributionList( KPIM::DistributionList &list )
{
  mDistributionList = list;

  mIcon = KGlobal::iconLoader()->loadIcon( "kdmconfig", KIcon::Small );

  mName = list.name();
  mKey = list.name();

  int count = list.entries( mAddressBook ).count();
  mEmail = i18n( "1 email address", "%n email addresses", count );

  mRecipient = mName;

  mTooltip = createTooltip( list );
}
#else
void RecipientItem::setDistributionList( KABC::DistributionList *list )
{
  mDistributionList = list;

  mIcon = KGlobal::iconLoader()->loadIcon( "kdmconfig", KIcon::Small );

  mName = list->name();
  mKey = list->name();

  int count = list->entries().count();
  mEmail = i18n( "1 email address", "%n email addresses", count );

  mRecipient = mName;

  mTooltip = createTooltip( list );
}
#endif

void RecipientItem::setAddressee( const KABC::Addressee &a,
  const QString &email )
{
  mAddressee = a;
  mEmail = email;
  mRecipient = mAddressee.fullEmail( mEmail );

  QImage img = a.photo().data();
  if ( !img.isNull() )
    mIcon = img.smoothScale( 20, 20, QImage::ScaleMin );
  else
    mIcon = KGlobal::iconLoader()->loadIcon( "personal", KIcon::Small );

  mName = mAddressee.realName();
  mKey = mAddressee.realName() + '|' + mEmail;

  mTooltip = "<qt>";
  if ( !mAddressee.realName().isEmpty() ) {
    mTooltip += mAddressee.realName() + "<br/>";
  }
  mTooltip += "<b>" + mEmail + "</b>";
}

QPixmap RecipientItem::icon() const
{
  return mIcon;
}

QString RecipientItem::name() const
{
  return mName;
}

QString RecipientItem::email() const
{
  return mEmail;
}

QString RecipientItem::recipient() const
{
  return mRecipient;
}

QString RecipientItem::tooltip() const
{
  return mTooltip;
}

#ifdef KDEPIM_NEW_DISTRLISTS
KPIM::DistributionList& RecipientItem::distributionList() {
  return mDistributionList;
}
#else
KABC::DistributionList * RecipientItem::distributionList() {
  return mDistributionList;
}
#endif

#ifdef KDEPIM_NEW_DISTRLISTS
QString RecipientItem::createTooltip( KPIM::DistributionList &distributionList ) const
{
  QString txt = "<qt>";

  txt += "<b>" + i18n( "Distribution List %1" ).arg ( distributionList.name() ) + "</b>";
  txt += "<ul>";
  KPIM::DistributionList::Entry::List entries = distributionList.entries( mAddressBook );
  KPIM::DistributionList::Entry::List::ConstIterator it;
  for( it = entries.begin(); it != entries.end(); ++it ) {
    txt += "<li>";
    txt += (*it).addressee.realName() + ' ';
    txt += "<em>";
    if ( (*it).email.isEmpty() ) txt += (*it).addressee.preferredEmail();
    else txt += (*it).email;
    txt += "</em>";
    txt += "<li/>";
  }
  txt += "</ul>";
  txt += "</qt>";

  return txt;
}
#else
QString RecipientItem::createTooltip( KABC::DistributionList *distributionList ) const
{
  QString txt = "<qt>";

  txt += "<b>" + i18n("Distribution List %1" ).arg ( distributionList->name() ) + "</b>";
  txt += "<ul>";
  KABC::DistributionList::Entry::List entries = distributionList->entries();
  KABC::DistributionList::Entry::List::ConstIterator it;
  for( it = entries.begin(); it != entries.end(); ++it ) {
    txt += "<li>";
    txt += (*it).addressee.realName() + ' ';
    txt += "<em>";
    if ( (*it).email.isEmpty() ) txt += (*it).addressee.preferredEmail();
    else txt += (*it).email;
    txt += "</em>";
    txt += "</li>";
  }
  txt += "</ul>";
  txt += "</qt>";

  return txt;
}
#endif

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
    QString tipText( i->recipientItem()->tooltip() );
    if ( !tipText.isEmpty() ) {
      tip( r, tipText );
    }
  }
}


RecipientsCollection::RecipientsCollection( const QString &id )
{
  mId = id;
  mTitle = id;
  mIsReferenceContainer = false;
}

RecipientsCollection::~RecipientsCollection()
{
  deleteAll();
}

void RecipientsCollection::setReferenceContainer( bool isReferenceContainer )
{
  mIsReferenceContainer = isReferenceContainer;
}

bool RecipientsCollection::isReferenceContainer() const
{
  return mIsReferenceContainer;
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
  mKeyMap.insert( item->key(), item );
}

RecipientItem::List RecipientsCollection::items() const
{
  return mKeyMap.values();
}

bool RecipientsCollection::hasEquivalentItem( RecipientItem *item ) const
{
  return mKeyMap.find( item->key() ) != mKeyMap.end();
}

RecipientItem * RecipientsCollection::getEquivalentItem( RecipientItem *item) const
{
  QMap<QString, RecipientItem *>::ConstIterator it;
  it = mKeyMap.find( item->key() );
  if ( it == mKeyMap.end() )
    return 0;
  return (*it);
}

void RecipientsCollection::clear()
{
  mKeyMap.clear();
}

void RecipientsCollection::deleteAll()
{
  if ( !isReferenceContainer() ) {
    QMap<QString, RecipientItem *>::ConstIterator it;
    for( it = mKeyMap.begin(); it != mKeyMap.end(); ++it ) {
      delete *it;
    }
  }
  clear();
}

QString RecipientsCollection::id() const
{
  return mId;
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
    ,mLdapSearchDialog( 0 )
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

//  connect( mCollectionCombo, SIGNAL( highlighted( int ) ),
//    SLOT( updateList() ) );
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

  mSearchLDAPButton = new QPushButton( i18n("Search &Directory Service"), this );
  searchLayout->addWidget( mSearchLDAPButton );
  connect( mSearchLDAPButton, SIGNAL( clicked() ), SLOT( slotSearchLDAP() ) );

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

  QMap<int,RecipientsCollection *>::ConstIterator it;
  for( it = mCollectionMap.begin(); it != mCollectionMap.end(); ++it ) {
    delete *it;
  }
}

void RecipientsPicker::initCollections()
{
  mAllRecipients = new RecipientsCollection( i18n("All") );
  mAllRecipients->setReferenceContainer( true );
  mDistributionLists = new RecipientsCollection( i18n("Distribution Lists") );
  mSelectedRecipients = new RecipientsCollection( i18n("Selected Recipients") );

  insertCollection( mAllRecipients );
  insertAddressBook( mAddressBook );
  insertCollection( mDistributionLists );
  insertRecentAddresses();
  insertCollection( mSelectedRecipients );

  rebuildAllRecipientsList();
}

void RecipientsPicker::insertAddressBook( KABC::AddressBook *addressbook )
{
  QMap<KABC::Resource *,RecipientsCollection *> collectionMap;

  QPtrList<KABC::Resource> resources = addressbook->resources();
  KABC::Resource *res;
  for( res = resources.first(); res; res = resources.next() ) {
    RecipientsCollection *collection = new RecipientsCollection( res->identifier() );
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
          collection = new RecipientsCollection( *catIt );
          collection->setReferenceContainer( true );
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

  insertDistributionLists();
  rebuildAllRecipientsList();
  updateList();
}

void RecipientsPicker::insertDistributionLists()
{
  mDistributionLists->deleteAll();

#ifdef KDEPIM_NEW_DISTRLISTS
  QValueList<KPIM::DistributionList> lists = KPIM::DistributionList::allDistributionLists( mAddressBook );
  for ( uint i = 0; i < lists.count(); ++i ) {
    RecipientItem *item = new RecipientItem( mAddressBook );
    item->setDistributionList( lists[ i ] );
    mDistributionLists->addItem( item );
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
    mDistributionLists->addItem( item );
  }
#endif
}

void RecipientsPicker::insertRecentAddresses()
{
  RecipientsCollection *collection = new RecipientsCollection( i18n("Recent Addresses") );

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
    collection->addItem( item );
  }

  insertCollection( collection );
}

void RecipientsPicker::insertCollection( RecipientsCollection *coll )
{
  int index = 0;
  QMap<int,RecipientsCollection *>::ConstIterator it;
  for ( it = mCollectionMap.begin(); it != mCollectionMap.end(); ++it ) {
    if ( (*it)->id() == coll->id() ) {
      delete *it;
      mCollectionMap.remove( index );
      mCollectionMap.insert( index, coll );
      return;
    }
    index++;
  }

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
  mSelectedRecipients->deleteAll();

  Recipient::List::ConstIterator it;
  for( it = recipients.begin(); it != recipients.end(); ++it ) {
    RecipientItem *item = 0;

    // if recipient is a distribution list, create
    // a detached copy.
    RecipientItem::List items = mDistributionLists->items();
    RecipientItem::List::ConstIterator distIt;
#ifdef KDEPIM_NEW_DISTRLISTS
    for ( distIt = items.begin(); distIt != items.end(); ++distIt ) {
      if ( (*it).email() == (*distIt)->name() ) {
        item = new RecipientItem( mAddressBook );
        item->setDistributionList( (*distIt)->distributionList() );
      }
    }
#else
    for ( distIt = items.begin(); distIt != items.end(); ++distIt ) {
      if ( (*it).email() == (*distIt)->name() ) {
        item = new RecipientItem();
        item->setDistributionList( (*distIt)->distributionList() );
      }
    }
#endif

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
    }

    item->setRecipientType( (*it).typeLabel() );
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

void RecipientsPicker::rebuildAllRecipientsList()
{
  mAllRecipients->clear();

  QMap<int,RecipientsCollection *>::ConstIterator it;
  for( it = mCollectionMap.begin(); it != mCollectionMap.end(); ++it ) {
    // skip self
    if ( (*it) == mAllRecipients )
      continue;

    RecipientItem::List coll = (*it)->items();

    RecipientItem::List::ConstIterator rcptIt;
    for ( rcptIt = coll.begin(); rcptIt != coll.end(); ++rcptIt ) {
      mAllRecipients->addItem( *rcptIt );
    }
  }
}

void RecipientsPicker::updateList()
{
  mRecipientList->clear();

  RecipientsCollection *coll = mCollectionMap[ mCollectionCombo->currentItem() ];

  RecipientItem::List items = coll->items();
  RecipientItem::List::ConstIterator it;
  for( it = items.begin(); it != items.end(); ++it ) {
    if ( coll != mSelectedRecipients ) {
      RecipientItem *selItem = mSelectedRecipients->getEquivalentItem( *it );
      if ( selItem ) {
        (*it)->setRecipientType( selItem->recipientType() );
      } else {
        (*it)->setRecipientType( QString() );
      }
    }
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

void RecipientsPicker::slotSearchLDAP()
{
    if ( !mLdapSearchDialog ) {
        mLdapSearchDialog = new KPIM::LDAPSearchDialog( this );
        connect( mLdapSearchDialog, SIGNAL( addresseesAdded() ),
                 SLOT(ldapSearchResult() ) );
    }
    mLdapSearchDialog->setSearchText( mSearchLine->text() );
    mLdapSearchDialog->show();

}

void RecipientsPicker::ldapSearchResult()
{
    QStringList emails = QStringList::split(',', mLdapSearchDialog->selectedEMails() );
    QStringList::iterator it( emails.begin() );
    QStringList::iterator end( emails.end() );
    for ( ; it != end; ++it ){
        QString name;
        QString email;
        KPIM::getNameAndMail( (*it), name, email );
        KABC::Addressee ad;
        ad.setNameFromString( name );
        ad.insertEmail( email );
#ifdef KDEPIM_NEW_DISTRLISTS
        RecipientItem *item = new RecipientItem( mAddressBook );
#else
        RecipientItem *item = new RecipientItem;
#endif
        item->setAddressee( ad, ad.preferredEmail() );
        emit pickedRecipient( Recipient( item->recipient(), Recipient::Undefined ) );
    }
}

#include "recipientspicker.moc"
