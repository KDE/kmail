// kmfolderdia.cpp

#include <qstring.h>
#include <qlabel.h>
#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <kapp.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlistbox.h>
#include <qmessagebox.h>
#include <qlayout.h>
#include <qhbox.h>

#include "kmmainwin.h"
#include "kmglobal.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmacctfolder.h"
#include "kmfoldermgr.h"

#include <assert.h>
#include <klocale.h>

#include "kmfolderdia.moc"


//-----------------------------------------------------------------------------
KMFolderDialog::KMFolderDialog(KMFolder* aFolder, KMFolderDir *aFolderDir,
			       QWidget *aParent, const QString& aCap):
  KMFolderDialogInherited( aParent, "KMFolderDialog", TRUE,
			   aCap,  KDialogBase::Ok|KDialogBase::Cancel ),
  mFolderDir( aFolderDir )
{
  QLabel *label;
  QString type;

  folder = (KMAcctFolder*)aFolder;

  QHBox *hb = new QHBox(this );
  hb->setSpacing( 10 );
  hb->setMargin( 10 );

  label = new QLabel(hb);
  label->setText(i18n("Name:"));
  
  nameEdit = new QLineEdit(hb);  
  nameEdit->setFocus();
  nameEdit->setText(folder ? folder->name() : i18n("unnamed"));
  nameEdit->setMinimumSize(nameEdit->sizeHint());

  new QFrame( hb ); // Filler

  label = new QLabel(hb);
  label->setText( i18n("File under:" ));

  fileInFolder = new QComboBox(hb);
  QStringList str;
  folderMgr->createFolderList( &str, &mFolders  );
  str.prepend( i18n( "Top Level" ));
  KMFolder *curFolder;
  int i = 1;
  fileInFolder->insertStringList( str );
  for (curFolder = mFolders.first(); curFolder; curFolder = mFolders.next()) {
    if (curFolder->child() == aFolderDir)
      fileInFolder->setCurrentItem( i  );
    ++i;
  }

  setMainWidget( hb );
  hb->setMinimumSize( hb->sizeHint() );

  setResizeMode( KDialogBase::ResizeMinimum );

}


//-----------------------------------------------------------------------------
void KMFolderDialog::slotOk()
{
  QString acctName;
  QString fldName, oldFldName;
  KMFolderDir *selectedFolderDir = &(folderMgr->dir());
  int curFolder = fileInFolder->currentItem();

  if (folder) oldFldName = folder->name();
  if (*nameEdit->text()) fldName = nameEdit->text();
  else fldName = oldFldName;
  if (fldName.isEmpty()) fldName = i18n("unnamed");
  if (curFolder != 0)
    selectedFolderDir = mFolders.at(curFolder - 1)->createChildFolder();

  QString message = i18n( "Failed to create folder '" ) + 
    (const char*)fldName + 
    i18n( "', folder already exists." );
  if ((selectedFolderDir->hasNamedFolder(fldName)) &&
      (!((folder) && 
	 (selectedFolderDir == folder->parent()) &&
	 (folder->name() == fldName)))) {
    QMessageBox::information(0, kapp->caption(), message, i18n("OK"));
    return;
  }

  message = i18n( "Cannot move a parent folder into a child folder." );
  KMFolderDir* folderDir = selectedFolderDir;


  // Buggy?
  if (folder && folder->child())
    while ((folderDir != &folderMgr->dir()) &&
	   (folderDir != folder->parent())){
      if (folderDir->findRef( folder ) != -1) {
	QMessageBox::information(0, kapp->caption(), message, i18n("OK"));
	return;
      }
      folderDir = folderDir->parent();
    }
  // End buggy?


  if (folder && folder->child() && (selectedFolderDir) &&
      (selectedFolderDir->path().find( folder->child()->path() + "/" ) == 0)) {
    QMessageBox::information(0, kapp->caption(), message, i18n("OK"));
    return;
  }

  if (folder && folder->child() && (selectedFolderDir == folder->child())) {
    QMessageBox::information(0, kapp->caption(), message, i18n("OK"));
    return;
  }

  if (!folder) {
    folder = (KMAcctFolder*)folderMgr->createFolder(fldName, FALSE, mFolderDir );
  }
  else if ((oldFldName != fldName) || (folder->parent() != selectedFolderDir))
    {
      if (folder->parent() != selectedFolderDir)
	folder->rename(fldName, selectedFolderDir );
      else
	folder->rename(fldName);
      folderMgr->contentsChanged();
    }

  KMFolderDialogInherited::slotOk();
}
