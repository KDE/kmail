// kmfolderdia.cpp

#include <assert.h>

#include <qcheckbox.h>
#include <qdir.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qvbox.h>
#include <qcombobox.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include "kmmainwin.h"
#include "kmglobal.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmacctfolder.h"
#include "kmfoldermgr.h"
#include "kmidentity.h"

#include "kmfolderdia.moc"


//-----------------------------------------------------------------------------
KMFolderDialog::KMFolderDialog(KMFolder* aFolder, KMFolderDir *aFolderDir,
			       QWidget *aParent, const QString& aCap):
  KMFolderDialogInherited( KDialogBase::Tabbed,
                           aCap, KDialogBase::Ok|KDialogBase::Cancel,
                           KDialogBase::Ok, aParent, "KMFolderDialog", TRUE ),
  folder((KMAcctFolder*)aFolder),mFolderDir( aFolderDir )
{
  mFolder = aFolder;
  kdDebug()<<"KMFolderDialog::KMFolderDialog()\n";

  // Main tab
  //
  QFrame *page = addPage( i18n("Folder Position"), i18n("Where the folder is located in the tree") );
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
  nameEdit->selectAll();
  hl->addWidget( nameEdit );

  hl->addSpacing( spacingHint() );

  QLabel *label2 = new QLabel( i18n("File under:" ), page );
  hl->addWidget( label2 );

  fileInFolder = new QComboBox(page);
  hl->addWidget( fileInFolder );

  QStringList str;
  kernel->folderMgr()->createFolderList( &str, &mFolders  );
  str.prepend( i18n( "Top Level" ));
  QGuardedPtr<KMFolder> curFolder;
  int i = 1;
  while (mFolders.at(i - 1) != mFolders.end()) {
    curFolder = *mFolders.at(i - 1);
    if (curFolder->isSystemFolder()) {
      mFolders.remove(mFolders.at(i-1));
      str.remove(str.at(i));
    } else
      ++i;
  }
  fileInFolder->insertStringList( str );

  for( i = 1; mFolders.at(i - 1) != mFolders.end(); ++i ) {
    curFolder = *mFolders.at(i - 1);
    if (curFolder->child() == aFolderDir)
      fileInFolder->setCurrentItem( i );
  }

  if (aFolder && aFolder->account()) {
    label->setEnabled( false );
    nameEdit->setEnabled( false );
    label2->setEnabled( false );
    fileInFolder->setEnabled( false );
  }

  // Mailing-list data tab
  //
  page = addPage( i18n("Associated Mailing List"), i18n("Email addresses of the mailing-list related to this folder") );

  topLayout =  new QVBoxLayout( page, 0, spacingHint() );

  //hl = new QHBoxLayout();
  topLayout->addSpacing( spacingHint()*2 );

  holdsMailingList = new QCheckBox( i18n("folder holds a mailing-list"), page);
  QObject::connect( holdsMailingList, SIGNAL(toggled(bool)),
                    this, SLOT(slotHoldsML(bool)) );

  topLayout->addWidget(holdsMailingList);

  QGridLayout *grid = new QGridLayout(page, 2, 2, 0, 8);
  grid->setColStretch(0, 1);
  grid->setColStretch(1, 100);

  topLayout->addSpacing( spacingHint()*2 );
  topLayout->addLayout( grid );
  topLayout->addSpacing( spacingHint()*2 );

  label = new QLabel( i18n("Identity:"), page );
  grid->addWidget( label, 0, 0 );
  mailingListIdentity = new QComboBox( page );
  mailingListIdentity->insertStringList( KMIdentity::identities() );
  mailingListIdentity->setMinimumSize(mailingListIdentity->sizeHint());
  grid->addWidget( mailingListIdentity, 0, 1 );

  label = new QLabel( i18n("Post Address:"), page );
  grid->addWidget( label, 1, 0 );
  mailingListPostAddress = new QLineEdit( page );
  mailingListPostAddress->setMinimumSize(mailingListPostAddress->sizeHint());
  grid->addWidget( mailingListPostAddress, 1, 1 );

//   hl = new QHBoxLayout();
//   topLayout->addLayout( hl );

//   label = new QLabel( i18n("Admin Address:"), page );
//   hl->addWidget( label );
//   mailingListAdminAddress = new QLineEdit( page );
//   mailingListAdminAddress->setMinimumSize(mailingListAdminAddress->sizeHint());
//   hl->addWidget( mailingListAdminAddress );

  if (folder)
  {
    mailingListPostAddress->setText(folder->mailingListPostAddress());
//     mailingListAdminAddress->setText(folder->mailingListAdminAddress());
    mailingListPostAddress->setEnabled(folder->isMailingList());
//     mailingListAdminAddress->setEnabled(folder->isMailingList());
    mailingListIdentity->setEnabled(folder->isMailingList());
    holdsMailingList->setChecked(folder->isMailingList());

    for (int i=0; i < mailingListIdentity->count(); ++i)
      if (mailingListIdentity->text(i) == folder->mailingListIdentity()) {
        mailingListIdentity->setCurrentItem(i);
        break;
      }
  }
  kdDebug()<<"Exiting KMFolderDialog::KMFolderDialog()\n";
}


//-----------------------------------------------------------------------------
void KMFolderDialog::slotOk()
{
  if (!mFolder || !mFolder->account())
  {
    QString acctName;
    QString fldName, oldFldName;
    KMFolderDir *selectedFolderDir = &(kernel->folderMgr()->dir());
    int curFolder = fileInFolder->currentItem();

    if (folder) oldFldName = folder->name();
    if (!nameEdit->text().isEmpty()) fldName = nameEdit->text();
    else fldName = oldFldName;
    fldName.replace(QRegExp("/"), "");
    if (fldName.isEmpty()) fldName = i18n("unnamed");
    if (curFolder != 0)
      selectedFolderDir = (*mFolders.at(curFolder - 1))->createChildFolder();

    QString message = i18n( "Failed to create folder '%1', folder already exists." ).arg(fldName);
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
      while ((folderDir != &kernel->folderMgr()->dir()) &&
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
      folder = (KMAcctFolder*)kernel->folderMgr()->createFolder(fldName, FALSE, selectedFolderDir );
    }
    else if ((oldFldName != fldName) || (folder->parent() != selectedFolderDir))
    {
      if (folder->parent() != selectedFolderDir)
	folder->rename(fldName, selectedFolderDir );
      else
	folder->rename(fldName);
      kernel->folderMgr()->contentsChanged();
    }
  }

  if (folder)
  {
    folder->setMailingList( holdsMailingList->isChecked() );
    folder->setMailingListPostAddress( mailingListPostAddress->text() );
//   folder->setMailingListAdminAddress( mailingListAdminAddress->text() );
    folder->setMailingListAdminAddress( QString::null );
    folder->setMailingListIdentity( mailingListIdentity->currentText() );
  }

  KMFolderDialogInherited::slotOk();
}

//-----------------------------------------------------------------------------
void KMFolderDialog::slotHoldsML( bool holdsML )
{
  if ( holdsML )
  {
    mailingListPostAddress->setEnabled(true);
//     mailingListAdminAddress->setEnabled(true);
  }
  else
  {
    mailingListPostAddress->setEnabled(false);
//     mailingListAdminAddress->setEnabled(false);
  }

  mailingListIdentity->setEnabled(holdsML);
}

