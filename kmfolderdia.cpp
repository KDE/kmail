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
#include <qgroupbox.h>

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
  KDialogBase( KDialogBase::Plain,
               aCap, KDialogBase::Ok|KDialogBase::Cancel,
               KDialogBase::Ok, aParent, "KMFolderDialog", TRUE ),
  folder((KMAcctFolder*)aFolder),mFolderDir( aFolderDir )
{
  mFolder = aFolder;
  kdDebug(5006)<<"KMFolderDialog::KMFolderDialog()\n";

  QFrame *page = plainPage();

  QVBoxLayout *topLayout =  new QVBoxLayout( page, 0, spacingHint(),
                                             "topLayout" );

  QGroupBox *fpGroup = new QGroupBox( i18n("Folder Position"), page, "fpGroup" );
  fpGroup->setColumnLayout( 0,  Qt::Vertical );

  topLayout->addWidget( fpGroup );

  QHBoxLayout *hl = new QHBoxLayout( fpGroup->layout() );
  hl->setSpacing( 6 );

  QLabel *label = new QLabel( i18n("&Name:"), fpGroup );
  hl->addWidget( label );

  nameEdit = new QLineEdit( fpGroup );
  nameEdit->setFocus();
  nameEdit->setText(folder ? folder->name() : i18n("unnamed"));
  nameEdit->setMinimumSize(nameEdit->sizeHint());
  nameEdit->selectAll();
  label->setBuddy( nameEdit );
  hl->addWidget( nameEdit );

  QLabel *label2 = new QLabel( i18n("&Belongs to:" ), fpGroup );
  hl->addWidget( label2 );

  fileInFolder = new QComboBox(fpGroup);
  hl->addWidget( fileInFolder );
  label2->setBuddy( fileInFolder );

  QGroupBox *mtGroup = new QGroupBox( i18n("Folder Type"), page, "mtGroup" );
  mtGroup->setColumnLayout( 0,  Qt::Vertical );

  topLayout->addWidget( mtGroup );

  QHBoxLayout *ml = new QHBoxLayout( mtGroup->layout() );
  ml->setSpacing( 6 );

  QLabel *label_type = new QLabel( i18n("Mailbox Format:" ), mtGroup );
  ml->addWidget( label_type );
  mailboxType = new QComboBox(mtGroup);
  mailboxType->insertItem("mbox (default)", 0);
  mailboxType->insertItem("maildir", 1);
  if (aFolder) mailboxType->setEnabled(false);
  ml->addWidget( mailboxType );
  ml->addStretch( 1 );

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

  if (aFolder && (aFolder->protocol() == "imap")) {
    label->setEnabled( false );
    nameEdit->setEnabled( false );
    label2->setEnabled( false );
    fileInFolder->setEnabled( false );
  }

  // Mailing-list data tab
  //
  QGroupBox *mlGroup = new QGroupBox(  i18n("Associated Mailing List" ), page );
  mlGroup->setColumnLayout( 0,  Qt::Vertical );
  QGridLayout *mlLayout = new QGridLayout(mlGroup->layout());
  mlLayout->setSpacing( 6 );
  holdsMailingList = new QCheckBox( i18n("&Folder holds a mailing list"), mlGroup);
  QObject::connect( holdsMailingList, SIGNAL(toggled(bool)),
                    SLOT(slotHoldsML(bool)) );
  topLayout->addWidget( mlGroup );
  mlLayout->addMultiCellWidget(holdsMailingList, 0, 0, 0, 1);

  mlLayout->setColStretch(0, 1);
  mlLayout->setColStretch(1, 100);

  label = new QLabel( i18n("&Post Address:"), mlGroup );
  mlLayout->addWidget( label, 1, 0 );
  mailingListPostAddress = new QLineEdit( mlGroup );
  label->setBuddy( mailingListPostAddress );
  mlLayout->addWidget( mailingListPostAddress, 1, 1 );
  mailingListPostAddress->setEnabled(false);

  QGroupBox *idGroup = new QGroupBox(  i18n("Identity" ), page );
  idGroup->setColumnLayout( 0, Qt::Vertical );
  QHBoxLayout *idLayout = new QHBoxLayout(idGroup->layout());
  idLayout->setSpacing( 6 );
  topLayout->addWidget( idGroup );

  label = new QLabel( i18n("&Sender:"), idGroup );
  idLayout->addWidget( label );
  identity = new QComboBox( idGroup );
  identity->insertStringList( KMIdentity::identities() );
  label->setBuddy( identity );
  idLayout->addWidget( identity, 3 );

  QGroupBox *mcGroup = new QGroupBox(  i18n("Misc" ), page );
  mcGroup->setColumnLayout( 0, Qt::Vertical );
  QHBoxLayout *mcLayout = new QHBoxLayout(mcGroup->layout());
  topLayout->addWidget( mcGroup );

  markAnyMessage = new QCheckBox( i18n( "&Mark any message in this folder" ), mcGroup );
  mcLayout->addWidget( markAnyMessage );
  mcGroup->hide();

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
    // mailingListIdentity->setEnabled(folder->isMailingList());
    holdsMailingList->setChecked(folder->isMailingList());
    // markAnyMessage->setChecked( folder->isAnyMessageMarked() );

    if (folder->protocol() == "maildir")
      mailboxType->setCurrentItem(1);
    else
      mailboxType->setCurrentItem(0);

    for (int i=0; i < identity->count(); ++i)
      if (identity->text(i) == folder->identity()) {
         identity->setCurrentItem(i);
         break;
      }
  }

  kdDebug(5006)<<"Exiting KMFolderDialog::KMFolderDialog()\n";
}


//-----------------------------------------------------------------------------
void KMFolderDialog::slotOk()
{
  if (!mFolder || (mFolder->protocol() != "imap"))
  {
    QString acctName;
    QString fldName, oldFldName;
    KMFolderDir *selectedFolderDir = &(kernel->folderMgr()->dir());
    int curFolder = fileInFolder->currentItem();

    if (folder) oldFldName = folder->name();
    if (!nameEdit->text().isEmpty()) fldName = nameEdit->text();
    else fldName = oldFldName;
    fldName.replace(QRegExp("/"), "");
    fldName.replace(QRegExp("^\\."), "");
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
      if (mailboxType->currentItem() == 1)
        folder = (KMAcctFolder*)kernel->folderMgr()->createFolder(fldName, FALSE, KMFolderTypeMaildir, selectedFolderDir );
      else
        folder = (KMAcctFolder*)kernel->folderMgr()->createFolder(fldName, FALSE, KMFolderTypeMbox, selectedFolderDir );
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
    folder->setIdentity( identity->currentText() );
// folder->setMarkAnyMessage( markAnyMessage->isChecked() );
  }

  KDialogBase::slotOk();
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

//  mailingListIdentity->setEnabled(holdsML);
}

