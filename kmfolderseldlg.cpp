// kmfolderseldlg.cpp

#include <config.h>
#include "kmfolderseldlg.h"
#include "kmfoldertree.h"
#include "kmmainwidget.h"

#include <qvbox.h>

#include <assert.h>

QString KMFolderSelDlg::oldSelection;

//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg(KMMainWidget * parent, const QString& caption)
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
    // cur will be 0 for accounts. Don't crash on that,
    // but ignore them for now.
    if(!cur)
      mListBox->item(i)->setSelectable(false);
    else if (!oldSelection.isNull() && oldSelection == cur->idString())
      mListBox->setCurrentItem(i);
    ++i;
  }

  // make sure that the current item is visible
  mListBox->centerCurrentItem();

  mListBox->setFocus();
}


//-----------------------------------------------------------------------------
KMFolderSelDlg::~KMFolderSelDlg()
{
  if(mListBox->currentItem() != -1) {
    QGuardedPtr<KMFolder> cur = *mFolder.at(mListBox->currentItem());
    if( cur )
      oldSelection = cur->idString();
  }
}


//-----------------------------------------------------------------------------
KMFolder* KMFolderSelDlg::folder(void)
{
  int idx = mListBox->currentItem();

  if (idx < 0) return 0;
  return *mFolder.at(idx);
}


//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotSelect(int)
{
  accept();
}


//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotCancel()
{
  disconnect(mListBox, SIGNAL(selected(int)), this, SLOT(slotSelect(int)));
  reject();
}


//-----------------------------------------------------------------------------
#include "kmfolderseldlg.moc"
