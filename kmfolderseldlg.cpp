// kmfolderseldlg.cpp

#include "kmfolderseldlg.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmglobal.h"
#include "kmfolderdir.h"

#include <kapp.h>
#include <qpushbt.h>
#include <qlistbox.h>
#include <qlayout.h>
#include <unistd.h>
#include <kapp.h>


//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg(const char* caption): 
  KMFolderSelDlgInherited(NULL, caption, TRUE)
{
  QButton *btnCancel, *btnOk;
  QBoxLayout* box = new QVBoxLayout(this);
  QBoxLayout* bbox = new QHBoxLayout;
  KMFolderDir* fdir = &folderMgr->dir();
  KMFolder* cur;

  initMetaObject();

  setCaption(i18n("Select folder"));

  mListBox = new QListBox(this);
  box->addWidget(mListBox, 100);
  connect(mListBox, SIGNAL(selected(int)), this, SLOT(slotSelect(int)));

  box->addLayout(bbox, 1);

  btnOk = new QPushButton(i18n("Ok"), this);
  btnOk->setMinimumSize(btnOk->size());
  connect(btnOk, SIGNAL(clicked()), this, SLOT(accept()));
  bbox->addWidget(btnOk, 1);

  btnCancel = new QPushButton(i18n("Cancel"), this);
  btnCancel->setMinimumSize(btnCancel->size());
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(slotCancel()));
  bbox->addWidget(btnCancel, 1);

  resize(100, 300);
  box->activate();

  for (cur=(KMFolder*)fdir->first(); cur; cur=(KMFolder*)fdir->next())
  {
    mListBox->insertItem(cur->label());
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
  KMFolderDir* fdir = &folderMgr->dir();
  int idx = mListBox->currentItem();

  if (idx < 0) return NULL;
  return (KMFolder*)fdir->at(idx);
}


//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotSelect(int)
{
  app->processEvents(200);
  accept();
}


//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotCancel()
{
  app->processEvents(200);
  disconnect(mListBox, SIGNAL(selected(int)), this, SLOT(slotSelect(int)));
  reject();
}


//-----------------------------------------------------------------------------
#include "kmfolderseldlg.moc"
