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

#include "config.h"

#include "recipientspicker.h"

#include "globalsettings.h"

#include <libkdepim/recentaddresses.h>

#include <k3listview.h>
#include <klocale.h>
#include <kabc/resource.h>
#include <kiconloader.h>
#include <kdialog.h>
#include <kwin.h>
#include <kabc/distributionlist.h>
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
#include <QVBoxLayout>

RecipientItem::RecipientItem()
  : mDistributionList( 0 )
{
}

void RecipientItem::setDistributionList( KABC::DistributionList *list )
{
  mDistributionList = list;

  mIcon = KIconLoader::global()->loadIcon( "kdmconfig", K3Icon::Small );

  mKey = 'D' + list->name();
}

void RecipientItem::setAddressee( const KABC::Addressee &a,
  const QString &email )
{
  mAddressee = a;
  mEmail = email;

  QImage img = a.photo().data();
  if ( !img.isNull() )
    mIcon = QPixmap::fromImage( img.scaled( 20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
  else
    mIcon = KIconLoader::global()->loadIcon( "personal", K3Icon::Small );

  mKey = 'A' + a.preferredEmail();
}

QPixmap RecipientItem::icon() const
{
  return mIcon;
}

QString RecipientItem::name() const
{
  if ( !mAddressee.isEmpty() ) return mAddressee.realName();
  else if ( mDistributionList ) return mDistributionList->name();
  else return QString();
}

QString RecipientItem::email() const
{
  if ( mAddressee.isEmpty() &&  mDistributionList ) {
    int count = mDistributionList->entries().count();
    return i18np( "1 email address", "%n email addresses", count );
  } else {
    return mEmail;
  }
  return QString();
}

QString RecipientItem::recipient() const
{
  QString r;
  if ( !mAddressee.isEmpty() ) r = mAddressee.fullEmail( mEmail );
  else if ( mDistributionList ) r = mDistributionList->name();
  return r;
}

QString RecipientItem::toolTip() const
{
  QString txt = "<qt>";

  if ( !mAddressee.isEmpty() ) {
    if ( !mAddressee.realName().isEmpty() ) {
      txt += mAddressee.realName() + "<br/>";
    }
    txt += "<b>" + mEmail + "</b>";
  } else if ( mDistributionList ) {
    txt += "<b>" + i18n("Distribution List %1",
        mDistributionList->name() ) + "</b>";
    txt += "<ul>";
    KABC::DistributionList::Entry::List entries = mDistributionList->entries();
    KABC::DistributionList::Entry::List::ConstIterator it;
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
  }

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


RecipientViewItem::RecipientViewItem( RecipientItem *item, K3ListView *listView )
  : K3ListViewItem( listView ), mRecipientItem( item )
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


#ifdef __GNUC__
#warning Port me!
#endif
#if 0
RecipientsListToolTip::RecipientsListToolTip( QWidget *parent,
  K3ListView *listView )
  : QToolTip( parent )
{
  mListView = listView;
}

void RecipientsListToolTip::maybeTip( const QPoint & pos )
{
  QRect r;
  Q3ListViewItem *item = mListView->itemAt( pos );
  RecipientViewItem *i = static_cast<RecipientViewItem *>( item );

  if( item ) {
    r = mListView->itemRect( item );
    QString tipText( i->recipientItem()->toolTip() );
    if ( !tipText.isEmpty() ) {
      tip( r, tipText );
    }
  }
}
#endif


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


SearchLine::SearchLine( QWidget *parent, K3ListView *listView )
  : K3ListViewSearchLine( parent, listView )
{
}

void SearchLine::keyPressEvent( QKeyEvent *ev )
{
  if ( ev->key() == Qt::Key_Down ) emit downPressed();

  K3ListViewSearchLine::keyPressEvent( ev );
}


RecipientsPicker::RecipientsPicker( QWidget *parent )
  : QDialog( parent ),
  mDistributionListManager( 0 )
{
  setObjectName("RecipientsPicker");
//  KWin::setType( winId(), NET::Dock );

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
  resLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

  connect( mCollectionCombo, SIGNAL( highlighted( int ) ),
    SLOT( updateList() ) );
  connect( mCollectionCombo, SIGNAL( activated( int ) ),
    SLOT( updateList() ) );

  QBoxLayout *searchLayout = new QHBoxLayout();
  topLayout->addItem( searchLayout );

  label = new QLabel( i18n("&Search:"), this );
  searchLayout->addWidget( label );

  mRecipientList = new K3ListView( this );
  mRecipientList->setSelectionMode( Q3ListView::Extended );
  mRecipientList->setAllColumnsShowFocus( true );
  mRecipientList->setFullWidth( true );
  topLayout->addWidget( mRecipientList );
  mRecipientList->addColumn( i18n("->") );
  mRecipientList->addColumn( i18n("Name") );
  mRecipientList->addColumn( i18n("Email") );
  connect( mRecipientList, SIGNAL( doubleClicked( Q3ListViewItem *,
    const QPoint &, int ) ), SLOT( slotPicked() ) );
  connect( mRecipientList, SIGNAL( returnPressed( Q3ListViewItem * ) ),
    SLOT( slotPicked() ) );

#ifdef __GNUC__
#warning Port me!
#endif
#if 0
  new RecipientsListToolTip( mRecipientList->viewport(), mRecipientList );
#endif
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

  delete mDistributionListManager;

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

  QList<KABC::Resource*> resources = addressbook->resources();
  QList<KABC::Resource*>::const_iterator rit;
  for( rit = resources.constBegin(); rit != resources.constEnd() ; ++rit ) {
    RecipientsCollection *collection = new RecipientsCollection;
    collectionMap.insert( *rit, collection );
    collection->setTitle( (*rit)->resourceName() );
  }

  QMap<QString,RecipientsCollection *> categoryMap;

  KABC::AddressBook::Iterator it;
  for( it = addressbook->begin(); it != addressbook->end(); ++it ) {
    QStringList emails = (*it).emails();
    QStringList::ConstIterator it3;
    for( it3 = emails.begin(); it3 != emails.end(); ++it3 ) {
      RecipientItem *item = new RecipientItem;
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
    RecipientItem *item = new RecipientItem;
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

  kDebug() << "RecipientsPicker::insertCollection() " << coll->title()
    << "  index: " << index << endl;

  mCollectionCombo->addItem( coll->title(), index );
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
    (*itAll)->setRecipientType( QString() );
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

      item = new RecipientItem;
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

  RecipientsCollection *coll = mCollectionMap[ mCollectionCombo->currentIndex() ];

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

void RecipientsPicker::slotPicked( Q3ListViewItem *viewItem )
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
  kDebug() << "RecipientsPicker::pick " << int( type ) << endl;

  int count = 0;
  Q3ListViewItemIterator it( mRecipientList ,
            Q3ListViewItemIterator::Visible | Q3ListViewItemIterator::Selected );
  for ( ; it.current(); ++it )
      ++count;

  if ( count > GlobalSettings::self()->maximumRecipients() ) {
    KMessageBox::sorry( this,
        i18np("You selected 1 recipient. The maximum supported number of "
             "recipients is %1. Please adapt the selection.",
             "You selected %n recipients. The maximum supported number of "
             "recipients is %1. Please adapt the selection.", count,
        GlobalSettings::self()->maximumRecipients() ) );
    return;
  }

  it = Q3ListViewItemIterator( mRecipientList ,
            Q3ListViewItemIterator::Visible | Q3ListViewItemIterator::Selected );
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
  if ( ev->key() == Qt::Key_Escape ) close();

  QWidget::keyPressEvent( ev );
}

void RecipientsPicker::readConfig()
{
  KSharedConfig::Ptr cfg = KGlobal::config();
  KConfigGroup group( cfg, "RecipientsPicker" );
  QSize size = group.readEntry( "Size", QVariant( QSize() ) ).toSize();
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
