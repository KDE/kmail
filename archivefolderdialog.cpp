/* Copyright 2009 Klarälvdalens Datakonsult AB

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "archivefolderdialog.h"

#include "backupjob.h"
#include "kmkernel.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "folderrequester.h"
#include "util.h"

#include <klocale.h>
#include <kcombobox.h>
#include <kurlrequester.h>
#include <kmessagebox.h>
#include <kfiledialog.h>

#include <qlabel.h>
#include <qcheckbox.h>
#include <qlayout.h>

using namespace KMail;

static QString standardArchivePath( const QString &folderName )
{
  return KGlobalSettings::documentPath() +
          i18n( "Start of the filename for a mail archive file" , "Archive" ) + "_" + folderName +
           "_" + QDate::currentDate().toString( Qt::ISODate ) + ".tar.bz2";
}

ArchiveFolderDialog::ArchiveFolderDialog( QWidget *parent )
  : KDialogBase( parent, "archive_folder_dialog", false, i18n( "Archive Folder" ),
                 KDialogBase::Ok | KDialogBase::Cancel,
                 KDialogBase::Ok, true ),
    mParentWidget( parent )
{
  QWidget *mainWidget = new QWidget( this );
  QGridLayout *mainLayout = new QGridLayout( mainWidget );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setMargin( KDialog::marginHint() );
  setMainWidget( mainWidget );

  int row = 0;

  // TODO: better label for "Ok" button
  // TODO: Explaination label
  // TODO: Use QFormLayout in KDE4

  QLabel *folderLabel = new QLabel( i18n( "&Folder:" ), mainWidget );
  mainLayout->addWidget( folderLabel, row, 0 );
  mFolderRequester = new FolderRequester( mainWidget, kmkernel->getKMMainWidget()->folderTree() );
  mFolderRequester->setMustBeReadWrite( false );
  connect( mFolderRequester, SIGNAL(folderChanged(KMFolder *)),
           SLOT(slotFolderChanged(KMFolder *)) );
  folderLabel->setBuddy( mFolderRequester );
  mainLayout->addWidget( mFolderRequester, row, 1 );
  row++;

  QLabel *formatLabel = new QLabel( i18n( "F&ormat:" ), mainWidget );
  mainLayout->addWidget( formatLabel, row, 0 );
  mFormatComboBox = new KComboBox( mainWidget );
  formatLabel->setBuddy( mFormatComboBox );

  // These combobox values have to stay in sync with the ArchiveType enum from BackupJob!
  mFormatComboBox->insertItem( i18n( "Compressed Zip Archive (.zip)" ) );
  mFormatComboBox->insertItem( i18n( "Uncompressed Archive (.tar)" ) );
  mFormatComboBox->insertItem( i18n( "BZ2-Compressed Tar Archive (.tar.bz2)" ) );
  mFormatComboBox->insertItem( i18n( "GZ-Compressed Tar Archive (.tar.gz)" ) );
  mFormatComboBox->setCurrentItem( 2 );
  connect( mFormatComboBox, SIGNAL(activated(int)),
           this, SLOT(slotFixFileExtension()) );
  mainLayout->addWidget( mFormatComboBox, row, 1 );
  row++;

  QLabel *fileNameLabel = new QLabel( i18n( "&Archive File:" ), mainWidget );
  mainLayout->addWidget( fileNameLabel, row, 0 );
  mUrlRequester = new KURLRequester( mainWidget );
  mUrlRequester->setMode( KFile::LocalOnly );
  mUrlRequester->setFilter( "*.tar *.zip *.tar.gz *.tar.bz2" );
  mUrlRequester->fileDialog()->setKeepLocation( true );
  fileNameLabel->setBuddy( mUrlRequester );
  connect( mUrlRequester, SIGNAL(urlSelected(const QString&)),
           this, SLOT(slotFixFileExtension()) );
  mainLayout->addWidget( mUrlRequester, row, 1 );
  row++;

  // TODO: Make this appear more dangerous!
  mDeleteCheckBox = new QCheckBox( i18n( "&Delete folders after completion" ), mainWidget );
  mainLayout->addMultiCellWidget( mDeleteCheckBox, row, row, 0, 1, Qt::AlignLeft );
  row++;

  // TODO: what's this, tooltips

  // TODO: Warn that user should do mail check for online IMAP and possibly cached IMAP as well

  mainLayout->setColStretch( 1, 1 );
  mainLayout->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding ), row, 0 );

  // Make it a bit bigger, else the folder requester cuts off the text too early
  resize( 500, minimumSize().height() );
}

void ArchiveFolderDialog::slotFolderChanged( KMFolder *folder )
{
  mDeleteCheckBox->setEnabled( folder->canDeleteMessages() );
}

void ArchiveFolderDialog::setFolder( KMFolder *defaultFolder )
{
  mFolderRequester->setFolder( defaultFolder );
  // TODO: what if the file already exists?
  mUrlRequester->setURL( standardArchivePath( defaultFolder->name() ) );
  mDeleteCheckBox->setEnabled( defaultFolder->canDeleteMessages() );
}

void ArchiveFolderDialog::slotOk()
{
  if ( !Util::checkOverwrite( mUrlRequester->url(), this ) ) {
    return;
  }

  if ( !mFolderRequester->folder() ) {
    KMessageBox::information( this, i18n( "Please select the folder that should be archived." ),
                              i18n( "No folder selected" ) );
    return;
  }

  // TODO: check if url is empty. or better yet, disable ok button until file is chosen
  KMail::BackupJob *backupJob = new KMail::BackupJob( mParentWidget );
  backupJob->setRootFolder( mFolderRequester->folder() );
  backupJob->setSaveLocation( mUrlRequester->url() );
  backupJob->setArchiveType( static_cast<BackupJob::ArchiveType>( mFormatComboBox->currentItem() ) );
  backupJob->setDeleteFoldersAfterCompletion( mDeleteCheckBox->isChecked() &&
                                              mFolderRequester->folder()->canDeleteMessages() );
  backupJob->start();
  accept();
}

void ArchiveFolderDialog::slotFixFileExtension()
{
  // KDE4: use KMimeType::extractKnownExtension() here
  const int numExtensions = 4;

  // These extensions are sorted differently, .tar has to come last, or it will match before giving
  // the more specific ones time to match.
  const char *sortedExtensions[numExtensions] = { ".zip", ".tar.bz2", ".tar.gz", ".tar" };

  // The extensions here are also sorted, like the enum order of BackupJob::ArchiveType
  const char *extensions[numExtensions] = { ".zip", ".tar", ".tar.bz2", ".tar.gz" };

  QString fileName = mUrlRequester->url();

  // First, try to find the extension of the file name and remove it
  for( int i = 0; i < numExtensions; i++ ) {
    int index = fileName.lower().findRev( sortedExtensions[i] );
    if ( index != -1 ) {
      fileName = fileName.left( fileName.length() - QString( sortedExtensions[i] ).length() );
      break;
    }
  }

  // Now, we've got a filename without an extension, simply append the correct one
  fileName += extensions[mFormatComboBox->currentItem()];
  mUrlRequester->setURL( fileName );
}

#include "archivefolderdialog.moc"
