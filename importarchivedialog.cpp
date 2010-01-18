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
#include "importarchivedialog.h"

#include "folderrequester.h"
#include "kmmainwidget.h"
#include "importjob.h"

#include <Akonadi/Collection>

#include <kurlrequester.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <qlabel.h>

using namespace KMail;

ImportArchiveDialog::ImportArchiveDialog( QWidget *parent )
  : KDialog( parent ), mParentWidget( parent )
{
  setObjectName( "import_archive_dialog" );
  setCaption( i18n( "Import Archive" ) );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  setModal( true );
  QWidget *mainWidget = new QWidget( this );
  QGridLayout *mainLayout = new QGridLayout( mainWidget );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setMargin( KDialog::marginHint() );
  setMainWidget( mainWidget );

  int row = 0;

  // TODO: Explaination label
  // TODO: Use QFormLayout in KDE4
  // TODO: better label for "Ok" button

  QLabel *folderLabel = new QLabel( i18n( "&Folder:" ), mainWidget );
  mainLayout->addWidget( folderLabel, row, 0 );
  mFolderRequester = new FolderRequester( mainWidget );
  folderLabel->setBuddy( mFolderRequester );
  mainLayout->addWidget( mFolderRequester, row, 1 );
  row++;

  QLabel *fileNameLabel = new QLabel( i18n( "&Archive File:" ), mainWidget );
  mainLayout->addWidget( fileNameLabel, row, 0 );
  mUrlRequester = new KUrlRequester( mainWidget );
  mUrlRequester->setFilter( "*.tar *.zip *.tar.gz *.tar.bz2" );
  fileNameLabel->setBuddy( mUrlRequester );
  mainLayout->addWidget( mUrlRequester, row, 1 );
  row++;

  // TODO: what's this, tooltips

  mainLayout->setColumnStretch( 1, 1 );
  mainLayout->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding ), row, 0 );

  // Make it a bit bigger, else the folder requester cuts off the text too early
  resize( 500, minimumSize().height() );
}

void ImportArchiveDialog::setFolder( const Akonadi::Collection &defaultCollection )
{
  mFolderRequester->setFolder( defaultCollection );
}

void ImportArchiveDialog::slotButtonClicked( int button )
{
  if ( button == KDialog::Cancel ) {
    reject();
    return;
  }
  Q_ASSERT( button == KDialog::Ok );

  if ( !QFile::exists( mUrlRequester->url().path() ) ) {
    KMessageBox::information( this, i18n( "Please select an archive file that should be imported." ),
                              i18n( "No archive file selected" ) );
    return;
  }

  if ( !mFolderRequester->folderCollection().isValid() ) {
    KMessageBox::information( this, i18n( "Please select the folder where the archive should be imported to." ),
                              i18n( "No target folder selected" ) );
    return;
  }

  // TODO: check if url is empty. or better yet, disable ok button until file is chosen

  ImportJob *importJob = new KMail::ImportJob( mParentWidget );
  importJob->setFile( mUrlRequester->url() );
  importJob->setRootFolder( mFolderRequester->folderCollection() );
  importJob->start();
  accept();
}

#include "importarchivedialog.moc"
