// kmfolderseldlg.cpp

#include "kmfolderseldlg.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmglobal.h"

#include <klocale.h>
#include <qbutton.h>
#include <qlist.h>
#include <qlayout.h>


//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg(const char* caption): 
  KMFolderSelDlgInherited()
{
  QButton* btnOk; 
  QButton* btnCancel;
  QGridLayout* grid = new QGridLayout(this, 2, 1);

  grid->setRowStretch(0, 100);

  mListBox = new QListBox(this, "list");
  grid->addMultiCellWidget(mListBox, 0, 0, 0, 1);

  btnOk = new QPushButton(nls->translate("Ok"), this, "Ok");
  btnOk->setMinimumSize(btnOk->sizeHints());
  connect(btnOk, SIGNAL(pressed()), this, SLOT(slotOkPressed()));
  grid->addWidget(btnOk, 1, 0);

  btnCancel = new QPushButton(nls->translate("Cancel"), this, "Cancel");
  btnCancel->setMinimumSize(btnCancel->sizeHints());
  connect(btnCancel, SIGNAL(pressed()), this, SLOT(slotCancelPressed()));
  grid->addWidget(btnCancel, 1, 1);

  grid->activate();
}


//-----------------------------------------------------------------------------
KMFolderSelDlg::~KMFolderSelDlg()
{
}
