// kmaddrbookdlg.cpp
// Author: Stefan Taferner <taferner@kde.org>

#include "kmaddrbookdlg.h"
#include "kmaddrbook.h"
#include <assert.h>
#include <klocale.h>
#include <kapp.h>

//-----------------------------------------------------------------------------
KMAddrBookSelDlg::KMAddrBookSelDlg(KMAddrBook* aAddrBook, const char* aCap):
  KMAddrBookSelDlgInherited(NULL, aCap, TRUE), mGrid(this, 2, 2),
  mListBox(this),
  mBtnOk(klocale->translate("Ok"),this), 
  mBtnCancel(klocale->translate("Cancel"),this)
{
  const char* addr;

  initMetaObject();
  setCaption(aCap ? aCap : klocale->translate("Addressbook"));

  assert(aAddrBook != NULL);
  mAddrBook = aAddrBook;
  mAddress  = 0;

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

  connect(&mBtnOk, SIGNAL(clicked()), SLOT(slotOk()));
  connect(&mListBox, SIGNAL(selected(int)), SLOT(slotOk()));
  connect(&mBtnCancel, SIGNAL(clicked()), SLOT(slotCancel()));

  for (addr=mAddrBook->first(); addr; addr=mAddrBook->next())
  {
    mListBox.insertItem(addr);
  }
}


//-----------------------------------------------------------------------------
KMAddrBookSelDlg::~KMAddrBookSelDlg()
{
}


//-----------------------------------------------------------------------------
void KMAddrBookSelDlg::slotOk()
{
  int idx = mListBox.currentItem();

  if (idx>=0) mAddress = mListBox.text(idx);
  else mAddress = 0;

  accept();
}


//-----------------------------------------------------------------------------
void KMAddrBookSelDlg::slotCancel()
{
  mAddress = 0;
  reject();
}


//=============================================================================
//
//  Class KMAddrBookEditDlg
//
//=============================================================================

KMAddrBookEditDlg::KMAddrBookEditDlg(KMAddrBook* aAddrBook, const char* aCap):
  KMAddrBookEditDlgInherited(NULL, NULL, TRUE), mGrid(this, 3, 4),
  mListBox(this), mEdtAddress(this),
  mBtnOk(klocale->translate("Ok"),this), 
  mBtnCancel(klocale->translate("Cancel"),this),
  mBtnAdd(klocale->translate("Add"),this), 
  mBtnRemove(klocale->translate("Remove"),this)
{
  const char* addr;

  initMetaObject();
  setCaption(aCap ? aCap : klocale->translate("Addressbook Manager"));

  assert(aAddrBook != NULL);
  mAddrBook = aAddrBook;
  mIndex = -1;

  mEdtAddress.adjustSize();
  mEdtAddress.setMinimumSize(mEdtAddress.size());
  mBtnOk.adjustSize();
  mBtnOk.setMinimumSize(mBtnOk.size());
  mBtnCancel.adjustSize();
  mBtnCancel.setMinimumSize(mBtnCancel.size());
  mBtnAdd.adjustSize();
  mBtnAdd.setMinimumSize(mBtnAdd.size());
  mBtnRemove.adjustSize();
  mBtnRemove.setMinimumSize(mBtnRemove.size());

  mGrid.addMultiCellWidget(&mListBox, 0, 0, 0, 3);
  mGrid.addMultiCellWidget(&mEdtAddress, 1, 1, 0, 3);
  mGrid.addWidget(&mBtnOk, 2, 0);
  mGrid.addWidget(&mBtnCancel, 2, 1);
  mGrid.addWidget(&mBtnAdd, 2, 2);
  mGrid.addWidget(&mBtnRemove, 2, 3);

  mGrid.setRowStretch(0,10);
  mGrid.setRowStretch(1,0);
  mGrid.setRowStretch(2,0);
  mGrid.setColStretch(0,10);
  mGrid.setColStretch(1,10);
  mGrid.setColStretch(2,10);
  mGrid.setColStretch(3,10);
  mGrid.activate();

  connect(&mListBox, SIGNAL(highlighted(const char*)), 
	  SLOT(slotLbxHighlighted(const char*)));
  connect(&mBtnOk, SIGNAL(clicked()), SLOT(slotOk()));
  connect(&mBtnCancel, SIGNAL(clicked()), SLOT(slotCancel()));
  connect(&mBtnAdd, SIGNAL(clicked()), SLOT(slotAdd()));
  connect(&mBtnRemove, SIGNAL(clicked()), SLOT(slotRemove()));

  for (addr=mAddrBook->first(); addr; addr=mAddrBook->next())
  {
    mListBox.insertItem(addr);
  }
}


//-----------------------------------------------------------------------------
KMAddrBookEditDlg::~KMAddrBookEditDlg()
{
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotLbxHighlighted(const char* aItem)
{
  if (mIndex>=0)
    mListBox.changeItem(mEdtAddress.text(), mIndex);
  mEdtAddress.setText(aItem);
  mIndex = mListBox.currentItem();
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotOk()
{
  int idx, num;
  const char* addr = mEdtAddress.text();

  if (mIndex>=0)
    mListBox.changeItem(addr, mIndex);
  else if (addr && *addr)
    mListBox.insertItem(mEdtAddress.text(), mListBox.currentItem());

  mAddrBook->clear();
  num = mListBox.count();
  for(idx=0; idx<num; idx++)
  {
    addr = mListBox.text(idx);
    debug("addressbook: %s", addr);
    mAddrBook->insert(addr);
  }
  mAddrBook->store();

  accept();
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotCancel()
{
  reject();
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotAdd()
{
  const char* addr = mEdtAddress.text();

  if (!addr || !*addr) return;
  mIndex = -1;
  mListBox.insertItem(addr, mListBox.currentItem());
}


//-----------------------------------------------------------------------------
void KMAddrBookEditDlg::slotRemove()
{
  int idx = mListBox.currentItem();
  mIndex = -1;
  if (idx >= 0) mListBox.removeItem(idx);
  if (idx >= mListBox.count()) idx--;
  mListBox.setCurrentItem(idx);
}


//-----------------------------------------------------------------------------
#include "kmaddrbookdlg.moc"
