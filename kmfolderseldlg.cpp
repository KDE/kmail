// kmfolderseldlg.cpp

#include "kmfolderseldlg.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmglobal.h"
#include "kmfolderdir.h"

#include <klocale.h>
#include <qpushbt.h>
#include <qlistbox.h>
#include <qlayout.h>


//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg(const char* caption): 
  KMFolderSelDlgInherited(NULL, caption, TRUE)
{
  QButton* btnOk; 
  QButton* btnCancel;
  QGridLayout* grid = new QGridLayout(this, 2, 2);
  KMFolderDir* fdir = &folderMgr->dir();
  KMFolder* cur;

  initMetaObject();

  grid->setRowStretch(0, 100);

  mListBox = new QListBox(this, "list");
  grid->addMultiCellWidget(mListBox, 0, 0, 0, 1);

  btnOk = new QPushButton(nls->translate("Ok"), this, "Ok");
  btnOk->setMinimumSize(btnOk->size());
  connect(btnOk, SIGNAL(pressed()), this, SLOT(slotOkPressed()));
  grid->addWidget(btnOk, 1, 0);

  btnCancel = new QPushButton(nls->translate("Cancel"), this, "Cancel");
  btnCancel->setMinimumSize(btnCancel->size());
  connect(btnCancel, SIGNAL(pressed()), this, SLOT(slotCancelPressed()));
  grid->addWidget(btnCancel, 1, 1);

  resize(100, 300);
  grid->activate();

  for (cur=(KMFolder*)fdir->first(); cur; cur=(KMFolder*)fdir->next())
  {
    mListBox->insertItem(cur->label());
  }
}


//-----------------------------------------------------------------------------
KMFolderSelDlg::~KMFolderSelDlg()
{
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderSelDlg::folder(void)
{
  KMFolderDir* fdir = &folderMgr->dir();
  int idx = mListBox->currentItem();

  if (idx < 0) return NULL;
  return (KMFolder*)fdir->at(idx);
}


//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotOkPressed()
{
  done(1);
}


//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotCancelPressed()
{
  done(0);
}


//-----------------------------------------------------------------------------
#include "kmfolderseldlg.moc"
