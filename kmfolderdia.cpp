// kmfolderdia.cpp

#include <assert.h>

#include <qdir.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qtextstream.h>

#include <klocale.h>
#include <kmessagebox.h>

#include "kmmainwin.h"
#include "kmglobal.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmacctfolder.h"
#include "kmfoldermgr.h"

#include "kmfolderdia.moc"


//-----------------------------------------------------------------------------
KMFolderDialog::KMFolderDialog(KMFolder* aFolder, KMFolderDir *aFolderDir,
			       QWidget *aParent, const QString& aCap):
  KMFolderDialogInherited( aParent, "KMFolderDialog", TRUE,
			   aCap,  KDialogBase::Ok|KDialogBase::Cancel ),
  folder((KMAcctFolder*)aFolder),mFolderDir( aFolderDir )
{
  QWidget *page = new QWidget(this);
  setMainWidget( page );
  QVBoxLayout *topLayout =  new QVBoxLayout( page, 0, spacingHint() );

  QHBoxLayout *hl = new QHBoxLayout();
  topLayout->addSpacing( spacingHint()*2 );
  topLayout->addLayout( hl );
  topLayout->addSpacing( spacingHint()*2 );
  
  QLabel *label = new QLabel( i18n("Name:"), page );
  hl->addWidget( label );
  
  nameEdit = new QLineEdit( page );  
  nameEdit->setFocus();
  nameEdit->setText(folder ? folder->name() : i18n("unnamed"));
  nameEdit->setMinimumSize(nameEdit->sizeHint());
  hl->addWidget( nameEdit );

  hl->addSpacing( spacingHint() );

  label = new QLabel( i18n("File under:" ), page );
  hl->addWidget( label );

  fileInFolder = new QComboBox(page);
  hl->addWidget( fileInFolder );

  QStringList str;
  folderMgr->createFolderList( &str, &mFolders  );
  str.prepend( i18n( "Top Level" ));
  KMFolder *curFolder;
  int i = 1;
  fileInFolder->insertStringList( str );
  for (curFolder = mFolders.first(); curFolder; curFolder = mFolders.next()) {
    if (curFolder->child() == aFolderDir)
      fileInFolder->setCurrentItem( i );
    ++i;
  }
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
    KMessageBox::error( this, message );
    return;
  }

  message = i18n( "Cannot move a parent folder into a child folder." );
  KMFolderDir* folderDir = selectedFolderDir;


  // Buggy?
  if (folder && folder->child())
    while ((folderDir != &folderMgr->dir()) &&
	   (folderDir != folder->parent())){
      if (folderDir->findRef( folder ) != -1) {
	KMessageBox::error( this, message );
	return;
      }
      folderDir = folderDir->parent();
    }
  // End buggy?


  if (folder && folder->child() && (selectedFolderDir) &&
      (selectedFolderDir->path().find( folder->child()->path() + "/" ) == 0)) {
    KMessageBox::error( this, message );
    return;
  }

  if (folder && folder->child() && (selectedFolderDir == folder->child())) {
    KMessageBox::error( this, message );
    return;
  }

  if (!folder) {
    folder = (KMAcctFolder*)folderMgr->createFolder(fldName, FALSE, selectedFolderDir );
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
