// kmfolderseldlg.cpp

#include <config.h>
#include "kmfolderseldlg.h"
#include "kmfoldertree.h"
#include "kmmainwin.h"

#include <qlistbox.h>
#include <qlayout.h>
#include <qvbox.h>

#include <assert.h>
#include <qpushbutton.h>

QString KMFolderSelDlg::oldSelection;

//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg(KMMainWin * parent, const QString& caption)
    : KMFolderSelDlgInherited(parent, "folder dialog", true, caption,
                              Ok|Cancel, Ok, true) // mainwin as parent, modal
{
  QGuardedPtr<KMFolder> cur;

  mListBox = new QListBox(makeVBoxMainWidget());
  connect(mListBox, SIGNAL(selected(int)), this, SLOT(slotSelect(int)));

  resize(220, 300);

  QStringList str;
  KMFolderTree * ft = parent->folderTree();
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
