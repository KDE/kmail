// kmaddrbookdlg.cpp
// Author: Stefan Taferner <taferner@kde.org>

#include "kmaddrbookdlg.h"
#include "kmaddrbook.h"
#include "kmkernel.h"
#include <assert.h>
#include <kapp.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kabapi.h>

//-----------------------------------------------------------------------------
KMAddrBookSelDlg::KMAddrBookSelDlg(KMAddrBook* aAddrBook, const char* aCap):
  KMAddrBookSelDlgInherited(NULL, aCap, TRUE), mGrid(this, 2, 2),
  mListBox(this),
  mBtnOk(i18n("OK"),this), 
  mBtnCancel(i18n("Cancel"),this)
{
  QString addr;

  initMetaObject();
  setCaption(aCap ? QString(aCap) : i18n("Addressbook"));

  assert(aAddrBook != NULL);
  mAddrBook = aAddrBook;
  mAddress  = QString::null;

  mBtnOk.adjustSize();
  mBtnOk.setMinimumSize(mBtnOk.size());
  mBtnCancel.adjustSize();
  mBtnCancel.setMinimumSize(mBtnCancel.size());

  mGrid.addMultiCellWidget(&mListBox, 0, 0, 0, 1);
  mGrid.addWidget(&mBtnOk, 1, 0);
  mGrid.addWidget(&mBtnCancel, 1, 1);

  mGrid.setRowStretch(0,10);
  mGrid.setRowStretch(1,0);
  mGrid.setColStretch(0,10);
  mGrid.setColStretch(1,10);
  mGrid.activate();
  
  mListBox.setSelectionMode(QListBox::Multi);

  connect(&mBtnOk, SIGNAL(clicked()), SLOT(slotOk()));
  connect(&mListBox, SIGNAL(selected(int)), SLOT(slotOk()));
  connect(&mBtnCancel, SIGNAL(clicked()), SLOT(slotCancel()));

  if (!KMAddrBookExternal::useKAB())
    for (QString addr=mAddrBook->first(); addr; addr=mAddrBook->next())
      mListBox.insertItem(addr);
  else {
    QStringList addresses;
    KabBridge::addresses(&addresses);
    mListBox.insertStringList(addresses);
  }

  resize(350, 450);
}


//-----------------------------------------------------------------------------
KMAddrBookSelDlg::~KMAddrBookSelDlg()
{
}


//-----------------------------------------------------------------------------
void KMAddrBookSelDlg::slotOk()
{
  mAddress = QString::null;
  unsigned int idx;
  unsigned int count = 0;
  for (idx = 0; idx < mListBox.count(); idx++)
  {
    if( mListBox.isSelected(idx) ) {
      if( count > 0 ) {
        mAddress += ", ";
      }
      mAddress += mListBox.text(idx);
      count++;
    }
  }
  accept();
}


//-----------------------------------------------------------------------------
void KMAddrBookSelDlg::slotCancel()
{
  mAddress = QString::null;
  reject();
}


//=============================================================================
//
//  Class KMAddrBookEditDlg
//
//=============================================================================

#include <qvbox.h>
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
  mEdtAddress = new QLineEdit( vbox );

  connect(mListBox, SIGNAL(selectionChanged()), 
	  this, SLOT(slotEnableRemove()));
  connect(mListBox, SIGNAL(highlighted(const QString&)), 
	  this, SLOT(slotLbxHighlighted(const QString&)));
  connect(mEdtAddress, SIGNAL(textChanged(const QString&)), 
	  this, SLOT(slotEnableAdd()));
  connect(this, SIGNAL(user1Clicked()), this, SLOT(slotAdd()) );
  connect(this, SIGNAL(user2Clicked()), this, SLOT(slotRemove()) );

  mAddresses = new QStringList();
  mKeys = new QValueList<KabKey>();
  
  if (!KMAddrBookExternal::useKAB())
    for (QString addr=mAddrBook->first(); addr; addr=mAddrBook->next())
      mListBox->insertItem(addr);
  else {
    KabBridge::addresses(mAddresses,mKeys);
    mListBox->insertStringList(*mAddresses);
  }

  mEdtAddress->setFocus();
  setInitialSize( QSize( 300, 400 ) );
}



//-----------------------------------------------------------------------------
KMAddrBookEditDlg::~KMAddrBookEditDlg()
{
  delete mAddresses;
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
