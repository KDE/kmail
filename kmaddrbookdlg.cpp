// kmaddrbookdlg.cpp
// Author: Stefan Taferner <taferner@kde.org>

#include "kmaddrbookdlg.h"
#include "kmaddrbook.h"
#include "kmkernel.h"
#include "kmrecentaddr.h"

#include <assert.h>

#include <qcheckbox.h>
#include <qvbox.h>
#include <qpushbutton.h>

#include <kglobal.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>

//-----------------------------------------------------------------------------
KMAddrBookSelDlg::KMAddrBookSelDlg(QWidget *parent, KMAddrBook* aAddrBook, const QString& aCap):
  KDialogBase(parent, "addressbook", true,
              aCap.isEmpty() ? i18n("Addressbook") : aCap,
              Ok|Cancel, Ok, true)
{
  QVBox *page = makeVBoxMainWidget();

  mListBox = new QListBox(page);
  mCheckBox = new QCheckBox(i18n("Show &recent addresses"), page);

  QString addr;
  assert(aAddrBook != NULL);
  mAddrBook = aAddrBook;
  mAddress  = QString::null;

  mListBox->setSelectionMode(QListBox::Multi);
  mListBox->setMinimumWidth(fontMetrics().maxWidth()*20);
  mListBox->setMinimumHeight(fontMetrics().lineSpacing()*15);

  readConfig();

  connect(mListBox, SIGNAL(selected(int)), SLOT(slotOk()));
  connect(mCheckBox, SIGNAL(toggled(bool)), SLOT(toggleShowRecent(bool)));

  showAddresses(AddressBookAddresses |
                (mCheckBox->isChecked() ? RecentAddresses : 0));
}


//-----------------------------------------------------------------------------
KMAddrBookSelDlg::~KMAddrBookSelDlg()
{
}

void KMAddrBookSelDlg::readConfig()
{
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cs( config, "General" );
  mCheckBox->setChecked( config->readBoolEntry("Show recent addresses in Addresses-dialog", false) );
}

void KMAddrBookSelDlg::saveConfig()
{
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cs( config, "General" );
  config->writeEntry( "Show recent addresses in Addresses-dialog",
                      mCheckBox->isChecked() );
}

void KMAddrBookSelDlg::toggleShowRecent( bool on )
{
  showAddresses(AddressBookAddresses | (on ? RecentAddresses : 0));
  saveConfig();
}

void KMAddrBookSelDlg::showAddresses( int addressTypes )
{
  mListBox->clear();

  if ( addressTypes & AddressBookAddresses ) {
    if (KMAddrBookExternal::useKABC()) {
      QStringList addresses;
      KabcBridge::addresses(&addresses);
      mListBox->insertStringList(addresses);
    }
    else if (KMAddrBookExternal::useKAB()) {
      QStringList addresses;
      KabBridge::addresses(&addresses);
      mListBox->insertStringList(addresses);
    }
    else {
      QStringList::ConstIterator it = mAddrBook->begin();
      for ( ; it != mAddrBook->end(); ++it)
        mListBox->insertItem(*it);
    }
  }
  mListBox->sort();

  if ( addressTypes & RecentAddresses )
    mListBox->insertStringList( KMRecentAddresses::self()->addresses(), 0 );
}


//-----------------------------------------------------------------------------
void KMAddrBookSelDlg::slotOk()
{
  mAddress = QString::null;
  unsigned int idx;
  unsigned int count = 0;
  for (idx = 0; idx < mListBox->count(); idx++)
  {
    if( mListBox->isSelected(idx) ) {
      if( count > 0 ) {
        mAddress += ", ";
      }
      mAddress += mListBox->text(idx);
      count++;
    }
  }
  KDialogBase::slotOk();
}


//-----------------------------------------------------------------------------
void KMAddrBookSelDlg::slotCancel()
{
  mAddress = QString::null;
  KDialogBase::slotCancel();
}


//=============================================================================
//
//  Class KMAddrBookEditDlg
//
//=============================================================================

KMAddrBookEditDlg::KMAddrBookEditDlg( KMAddrBook* aAddrBook, QWidget *parent,
				      const char *name, bool modal )
  :KDialogBase( parent, name, modal, i18n("Addressbook Manager"),
		Ok|Cancel|User1|User2,Ok,false,i18n("&Add"),i18n("&Remove") )
{
  assert(aAddrBook!=0);
  mAddrBook = aAddrBook;
  mIndex = -1;

  enableButton( User1, false );
  enableButton( User2, false );

  QVBox *vbox = makeVBoxMainWidget();
  mListBox    = new QListBox( vbox );
  mEdtAddress = new KLineEdit( vbox );

  connect(mListBox, SIGNAL(selectionChanged()),
	  this, SLOT(slotEnableRemove()));
  connect(mListBox, SIGNAL(highlighted(const QString&)),
	  this, SLOT(slotLbxHighlighted(const QString&)));
  connect(mEdtAddress, SIGNAL(textChanged(const QString&)),
	  this, SLOT(slotEnableAdd()));
  connect(this, SIGNAL(user1Clicked()), this, SLOT(slotAdd()) );
  connect(this, SIGNAL(user2Clicked()), this, SLOT(slotRemove()) );

  mKeys = new QValueList<KabKey>();

  if (!KMAddrBookExternal::useKAB()) {
    QStringList::ConstIterator it = mAddrBook->begin();
    for ( ; it != mAddrBook->end(); ++it)
      mListBox->insertItem(*it);
  }
  else {
    QStringList addresses;
    KabBridge::addresses(&addresses,mKeys);
    mListBox->insertStringList(addresses);
  }

  mEdtAddress->setFocus();
  setInitialSize( QSize( 300, 400 ) );
}



//-----------------------------------------------------------------------------
KMAddrBookEditDlg::~KMAddrBookEditDlg()
{
  delete mKeys;
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotLbxHighlighted(const QString& aItem)
{
  enableButton( User2, true ); // Remove

  int oldIndex = mIndex;
  disconnect( mListBox, SIGNAL(highlighted(const QString&)),
	  this, SLOT(slotLbxHighlighted(const QString&)));
  mIndex = mListBox->currentItem();

  // Change of behaviour between QT 2.1b1 and QT2.1b2
  //  changeItem below changes the currentItem!

  if ((oldIndex>=0) && (mEdtAddress->text() != mListBox->text(oldIndex))) {
    if (KMAddrBookExternal::useKAB())
      KabBridge::replace( mEdtAddress->text(), *(mKeys->at(oldIndex)) );
    mListBox->changeItem(mEdtAddress->text(), oldIndex);
  }
  mListBox->setCurrentItem( mIndex );  // keep currentItem the same
  mEdtAddress->setText(aItem);

  connect( mListBox, SIGNAL(highlighted(const QString&)),
	  SLOT(slotLbxHighlighted(const QString&)));
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotOk()
{
  int idx, num;
  QString addr = mEdtAddress->text();
  disconnect( mListBox, SIGNAL(highlighted(const QString&)),
	  this, SLOT(slotLbxHighlighted(const QString&)));

  if (mIndex>=0) {
    if (KMAddrBookExternal::useKAB())
      KabBridge::replace( mEdtAddress->text(), *(mKeys->at(mIndex)) );
    mListBox->changeItem(addr, mIndex);
  }
  else if (!addr.isEmpty()) {
    if (KMAddrBookExternal::useKAB()) {
      KabKey key;
      if (KabBridge::add( mEdtAddress->text(), key)) {
	int cur = mListBox->currentItem();
	if (cur < 0)
	  cur = 0;
	mKeys->insert( mKeys->at(cur), key );
	mListBox->insertItem(mEdtAddress->text(), mListBox->currentItem());
      }
    } else
      mListBox->insertItem(mEdtAddress->text(), mListBox->currentItem());
  }

  if (!KMAddrBookExternal::useKAB()) { // Update KMail address book
    mAddrBook->clear();
    num = mListBox->count();
    for(idx=0; idx<num; idx++)
      {
	addr = mListBox->text(idx);
	mAddrBook->insert(addr);
      }
    if(mAddrBook->store() == IO_FatalError)
      {
	KMessageBox::sorry(0, i18n("The addressbook could not be stored."));
	return;
      }
  }

  accept();
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotCancel()
{
  reject();
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotEnableAdd()
{
  enableButton( User1, true );
  actionButton(User1)->setDefault(true);
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotAdd()
{
  QString addr = mEdtAddress->text();
  if( addr.isEmpty() )
  {
    return;
  }

  mIndex = -1;

  if (KMAddrBookExternal::useKAB()) {
    KabKey key;
    if (KabBridge::add( addr, key)) {
      mKeys->insert( mKeys->at(0), key );
      mListBox->insertItem(addr, 0);
    }
  } else
    mListBox->insertItem( addr, 0 );

  mListBox->setContentsPos(0, 0);
  mEdtAddress->setText("");
  mEdtAddress->setFocus();

  enableButton( User1, false ); // Add
  actionButton(User1)->setDefault(false);
  actionButton(Ok)->setDefault(true);
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotEnableRemove()
{
  enableButton( User2, true ); // Remove
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotRemove()
{
  int idx = mListBox->currentItem();
  mIndex = -1;
  if (idx >= 0) {
    mListBox->removeItem(idx);
    if (KMAddrBookExternal::useKAB()) {
      KabBridge::remove( *(mKeys->at(idx)) );
      mKeys->remove( mKeys->at(idx) );
    }
  }
  if (idx >= (int)mListBox->count()) idx--;
  mListBox->setCurrentItem(idx);
  if( mListBox->count() == 0 ) {
    enableButton( User2, false ); // Remove
  }
}


//-----------------------------------------------------------------------------
#include "kmaddrbookdlg.moc"
