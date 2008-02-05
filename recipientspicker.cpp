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
#ifdef KDEPIM_NEW_DISTRLISTS
#include <libkdepim/distributionlist.h>
#else
#include <kabc/distributionlist.h>
#endif

#include <klocale.h>
#include <kabc/resource.h>
#include <kiconloader.h>
#include <kdialog.h>
#include <kwindowsystem.h>
#include <kmessagebox.h>
#include <kconfiggroup.h>

#include <QBoxLayout>
#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QPixmap>
#include <QPushButton>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

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

  mIcon = KIconLoader::global()->loadIcon( "kontact_contacts", KIconLoader::Small );

  mName = list.name();
  mKey = list.name();

  int count = list.entries( mAddressBook ).count();
  mEmail = i18np( "1 email address", "%1 email addresses", count );

  mRecipient = mName;

  mTooltip = createTooltip( list );
}
#else
void RecipientItem::setDistributionList( KABC::DistributionList *list )
{
  mDistributionList = list;

  mIcon = KIconLoader::global()->loadIcon( "kontact_contacts", KIconLoader::Small );

  mName = list->name();
  mKey = list->name();

  int count = list->entries().count();
  mEmail = i18np( "1 email address", "%1 email addresses", count );

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
    mIcon = QPixmap::fromImage( img.scaled( 20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
  else
    mIcon = KIconLoader::global()->loadIcon( "user-identity", KIconLoader::Small );

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
KPIM::DistributionList& RecipientItem::distributionList() const {
  return mDistributionList;
}
#else
KABC::DistributionList * RecipientItem::distributionList() const {
  return mDistributionList;
}
#endif

#ifdef KDEPIM_NEW_DISTRLISTS
QString RecipientItem::createTooltip( KPIM::DistributionList &distributionList ) const
{
  QString txt = "<qt>";

  txt += "<b>" + i18n( "Distribution List %1", distributionList.name() ) + "</b>";
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

  txt += "<b>" + i18n("Distribution List %1", distributionList->name() ) + "</b>";
  txt += "<ul>";
  KABC::DistributionList::Entry::List entries = distributionList->entries();
  KABC::DistributionList::Entry::List::ConstIterator it;
  for( it = entries.begin(); it != entries.end(); ++it ) {
    txt += "<li>";
    txt += (*it).addressee().realName() + ' ';
    txt += "<em>";
    if ( (*it).email().isEmpty() ) txt += (*it).addressee().preferredEmail();
    else txt += (*it).email();
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

RecipientViewItem::RecipientViewItem( RecipientItem *item, QTreeWidget *listView )
  : QTreeWidgetItem( listView ), mRecipientItem( item )
{
  setText( 0, item->recipientType() );
  setText( 1, item->name() );
  setText( 2, item->email() );

  setIcon( 1, item->icon() );
}

RecipientItem *RecipientViewItem::recipientItem() const
{
  return mRecipientItem;
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

SearchLine::SearchLine( QWidget *parent, QTreeWidget *listView )
  : KTreeWidgetSearchLine( parent, listView )
{
}

void SearchLine::keyPressEvent( QKeyEvent *ev )
{
  if ( ev->key() == Qt::Key_Down ) emit downPressed();

  KTreeWidgetSearchLine::keyPressEvent( ev );
}

RecipientsTreeWidget::RecipientsTreeWidget( QWidget *parent )
  : QTreeWidget( parent )
{}

void RecipientsTreeWidget::keyPressEvent ( QKeyEvent *event ) {
  if ( event->key() == Qt::Key_Return )
    emit returnPressed();
  QTreeWidget::keyPressEvent( event );
}

RecipientsPicker::RecipientsPicker( QWidget *parent )
  : QDialog( parent )
{
  setObjectName("RecipientsPicker");
  setWindowTitle( i18n("Select Recipient") );

  QBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setSpacing( KDialog::spacingHint() );
  topLayout->setMargin( KDialog::marginHint() );

  QBoxLayout *resLayout = new QHBoxLayout();
  topLayout->addItem( resLayout );

  QLabel *label = new QLabel( i18n("Address book:"), this );
  resLayout->addWidget( label );

  mCollectionCombo = new QComboBox( this );
  resLayout->addWidget( mCollectionCombo );
  resLayout->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding ) );

  connect( mCollectionCombo, SIGNAL( activated( int ) ), SLOT( updateList() ) );

  QBoxLayout *searchLayout = new QHBoxLayout();
  topLayout->addItem( searchLayout );

  label = new QLabel( i18n("&Search:"), this );
  searchLayout->addWidget( label );

  mRecipientList = new RecipientsTreeWidget( this );
  mRecipientList->setSelectionMode( QAbstractItemView::ExtendedSelection );
  mRecipientList->setAllColumnsShowFocus( true );
  mRecipientList->setIndentation( 0 );
  mRecipientList->setAlternatingRowColors( true );
  mRecipientList->setSortingEnabled( true );
  mRecipientList->sortItems( 1, Qt::AscendingOrder );
  mRecipientList->setHeaderLabels( QStringList() << i18n("->")
                                                 << i18n("Name")
                                                 << i18n("Email") );
  mRecipientList->setColumnWidth( 0, 80);
  mRecipientList->setColumnWidth( 1, 200 );
  mRecipientList->setColumnWidth( 2, 200 );
  topLayout->addWidget( mRecipientList );

  connect( mRecipientList, SIGNAL( itemDoubleClicked(QTreeWidgetItem*,int) ),
           SLOT( slotPicked() ) );
  connect( mRecipientList, SIGNAL( returnPressed() ),
           SLOT( slotPicked() ) );

  mSearchLine = new SearchLine( this, mRecipientList );
  searchLayout->addWidget( mSearchLine );
  label->setBuddy( label );
  connect( mSearchLine, SIGNAL( downPressed() ), SLOT( setFocusList() ) );

  QBoxLayout *buttonLayout = new QHBoxLayout();
  topLayout->addItem( buttonLayout );

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

  mCollectionCombo->setCurrentIndex( 0 );

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

  QList<KABC::Resource*> resources = addressbook->resources();
  QList<KABC::Resource*>::const_iterator rit;
  for( rit = resources.constBegin(); rit != resources.constEnd() ; ++rit ) {
    RecipientsCollection *collection = new RecipientsCollection( (*rit)->identifier() );
    collectionMap.insert( *rit, collection );
    collection->setTitle( (*rit)->resourceName() );
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
      collIt = collectionMap.find( (*it).resource() );
      if ( collIt != collectionMap.end() ) {
        (*collIt)->addItem( item );
      } else {
        kDebug(5006) << "Collection for resource not found. shouldn't happen";
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
  QList<KPIM::DistributionList> lists = KPIM::DistributionList::allDistributionLists( mAddressBook );
  for ( int i = 0; i < lists.count(); ++i ) {
    RecipientItem *item = new RecipientItem( mAddressBook );
    item->setDistributionList( lists[ i ] );
    mDistributionLists->addItem( item );
  }
#else
  QList<KABC::DistributionList*> lists = mAddressBook->allDistributionLists();
  foreach ( KABC::DistributionList *list, lists ) {
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
    KPIM::RecentAddresses::self( &config )->kabcAddresses();

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

  mCollectionCombo->insertItem( index, coll->title() );
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
  button->setDefault( true );
}

void RecipientsPicker::setDefaultType( Recipient::Type type )
{
  mDefaultType = type;
  mToButton->setDefault( type == Recipient::To );
  mCcButton->setDefault( type == Recipient::Cc );
  mBccButton->setDefault( type == Recipient::Bcc );
}

void RecipientsPicker::rebuildAllRecipientsList()
{
  mAllRecipients->clear();

  QMap<int,RecipientsCollection *>::ConstIterator it;
  for( it = mCollectionMap.begin(); it != mCollectionMap.end(); ++it ) {
    // skip self
    if ( (*it) == mAllRecipients )
      continue;

    kDebug(5006) << "processing collection" << (*it)->id() << (*it)->title();

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

  RecipientsCollection *coll = mCollectionMap[ mCollectionCombo->currentIndex() ];

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
    RecipientViewItem *newItem = new RecipientViewItem( *it, mRecipientList );
    for ( int i = 0; i < mRecipientList->columnCount(); i++ )
      newItem->setToolTip( i, newItem->recipientItem()->tooltip() );
    mRecipientList->addTopLevelItem( newItem );
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

void RecipientsPicker::slotPicked( QTreeWidgetItem *viewItem )
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
  kDebug(5006) <<"RecipientsPicker::pick" << int( type );

  int count = mRecipientList->selectedItems().count();

  if ( count > GlobalSettings::self()->maximumRecipients() ) {
    KMessageBox::sorry( this,
        i18np("You selected 1 recipient. The maximum supported number of "
             "recipients is %2. Please adapt the selection.",
             "You selected %1 recipients. The maximum supported number of "
             "recipients is %2. Please adapt the selection.", count,
        GlobalSettings::self()->maximumRecipients() ) );
    return;
  }

  QList<QTreeWidgetItem*> selectedItems = mRecipientList->selectedItems();
  QList<QTreeWidgetItem*>::iterator it = selectedItems.begin();
  for ( ; it != selectedItems.end(); ++it ) {
    RecipientViewItem *item = static_cast<RecipientViewItem *>( *it );
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
  if ( ev->key() == Qt::Key_Escape )
    close();

  QDialog::keyPressEvent( ev );
}

void RecipientsPicker::readConfig()
{
  KSharedConfig::Ptr cfg = KGlobal::config();
  KConfigGroup group( cfg, "RecipientsPicker" );
  QSize size = group.readEntry( "Size", QSize() );
  if ( !size.isEmpty() ) {
    resize( size );
  }
  int currentCollection = group.readEntry( "CurrentCollection", -1 );
  if ( currentCollection >= 0 &&
       currentCollection < mCollectionCombo->count() ) {
    mCollectionCombo->setCurrentIndex( currentCollection );
  }
}

void RecipientsPicker::writeConfig()
{
  KSharedConfig::Ptr cfg = KGlobal::config();
  KConfigGroup group( cfg, "RecipientsPicker" );
  group.writeEntry( "Size", size() );
  group.writeEntry( "CurrentCollection", mCollectionCombo->currentIndex() );
}

void RecipientsPicker::setFocusList()
{
  mRecipientList->setFocus();
}


#include "recipientspicker.moc"
