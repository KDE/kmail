#undef QT_NO_ASCII_CAST
// kmfolderseldlg.cpp

#include "kmfolderseldlg.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmglobal.h"
#include "kmfolderdir.h"

#include <qpushbutton.h>
#include <qlistbox.h>
#include <qlayout.h>
#include <unistd.h>
#include <kapp.h>
#include <qaccel.h>
#include <kbuttonbox.h>
#include <klocale.h>

QString KMFolderSelDlg::oldSelection;

//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg(QString caption):
  KMFolderSelDlgInherited(NULL, caption, TRUE)
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
  kernel->folderMgr()->createFolderList( &str, &mFolder  );
  int i = 1;
  mListBox->insertStringList( str );
  while (mFolder.at(i - 1) != mFolder.end()) {
    cur = *mFolder.at(i - 1);
    if(!oldSelection.isNull() && oldSelection == cur->label())
      mListBox->setCurrentItem(i - 1);
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
