// kmfolderseldlg.cpp

#include "kmfolderseldlg.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmglobal.h"
#include "kmfolderdir.h"
#include "kmfoldertree.h"
#include "kmmainwin.h"

#include <qpushbutton.h>
#include <qlistbox.h>
#include <qlayout.h>
#include <kapp.h>
#include <qaccel.h>
#include <kbuttonbox.h>
#include <klocale.h>
#include <kdebug.h>

#include <assert.h>

QString KMFolderSelDlg::oldSelection;

//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg(QWidget * parent, QString caption):
  KMFolderSelDlgInherited(parent, 0, TRUE) // mainwin as parent, no name, but modal
{
  QPushButton *btnCancel, *btnOk;
  QBoxLayout* box = new QVBoxLayout(this, 2, 0);
  QBoxLayout* bbox = new QHBoxLayout(0);
  QGuardedPtr<KMFolder> cur;


  setCaption(caption);

  mListBox = new QListBox(this);
  box->addWidget(mListBox, 100);
  connect(mListBox, SIGNAL(selected(int)), this, SLOT(slotSelect(int)));

  box->addLayout(bbox, 1);

  KButtonBox *butbox = new KButtonBox(this);
  btnOk = butbox->addButton(i18n("OK"));
  btnOk->setDefault(TRUE);
  connect(btnOk, SIGNAL(clicked()), this, SLOT(accept()));

  btnCancel = butbox->addButton(i18n("Cancel"));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(slotCancel()));
  butbox->layout();
  box->addWidget(butbox);

  QAccel *acc = new QAccel(this);
  acc->connectItem(acc->insertItem(Key_Escape), this, SLOT(slotCancel()));

  resize(220, 300);
  box->activate();

  QStringList str;
  KMMainWin * mw = dynamic_cast<KMMainWin*>( parent );
  assert( mw );
  KMFolderTree * ft = mw->folderTree();
  assert( ft );
  ft->createFolderList( &str, &mFolder  );
  mListBox->insertStringList( str );
  int i = 0;
  while (mFolder.at(i) != mFolder.end()) {
    cur = *mFolder.at(i);
    // cur will be NULL for accounts. Don't crash on that,
    // but ignore them for now.
    if(!cur)
      mListBox->item(i)->setSelectable(false);
    else if (!oldSelection.isNull() && oldSelection == cur->label())
      mListBox->setCurrentItem(i);
    ++i;
  }

  // make sure item is visible
  if(mListBox->currentItem() != -1)
  {
    unsigned idx = 0;
    while(mListBox->numItemsVisible()-2 + mListBox->topItem() < mListBox->currentItem() && idx < mListBox->count())
	  mListBox->setTopItem(idx++);
  }

  mListBox->setFocus();
}


//-----------------------------------------------------------------------------
KMFolderSelDlg::~KMFolderSelDlg()
{
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderSelDlg::folder(void)
{
  int idx = mListBox->currentItem();

  if (idx < 0) return NULL;
  return *mFolder.at(idx);
}


//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotSelect(int)
{
  if(mListBox->currentItem() != -1)
    oldSelection = mListBox->text(mListBox->currentItem());
  accept();
}


//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotCancel()
{
  disconnect(mListBox, SIGNAL(selected(int)), this, SLOT(slotSelect(int)));
  if(mListBox->currentItem() != -1)
    oldSelection = mListBox->text(mListBox->currentItem());
  reject();
}


//-----------------------------------------------------------------------------
#include "kmfolderseldlg.moc"
