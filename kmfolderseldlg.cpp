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
  QButton* btnCancel;
  QBoxLayout* box = new QVBoxLayout(this);
  KMFolderDir* fdir = &folderMgr->dir();
  KMFolder* cur;

  initMetaObject();

  mListBox = new QListBox(this, "list");
  box->addWidget(mListBox, 100);
  connect(mListBox, SIGNAL(highlighted(int)), this, SLOT(slotSelect(int)));

  btnCancel = new QPushButton(nls->translate("Cancel"), this, "Cancel");
  btnCancel->setMinimumSize(btnCancel->size());
  connect(btnCancel, SIGNAL(pressed()), this, SLOT(slotCancel()));
  box->addWidget(btnCancel, 1);

  resize(100, 300);
  box->activate();

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
void KMFolderSelDlg::slotSelect(int)
{
  done(1);
}


//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotCancel()
{
  done(0);
}


//-----------------------------------------------------------------------------
#include "kmfolderseldlg.moc"
