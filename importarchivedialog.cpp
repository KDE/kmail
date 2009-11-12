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

#include "kmfolder.h"
#include "folderrequester.h"
#include "kmmainwidget.h"
#include "importjob.h"

#include <kurlrequester.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <qlabel.h>

using namespace KMail;

ImportArchiveDialog::ImportArchiveDialog( QWidget *parent )
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

  // TODO: Explaination label
  // TODO: Use QFormLayout in KDE4
  // TODO: sensible stretch factors
  // TODO: sensible minimum horizontal size

  QLabel *folderLabel = new QLabel( i18n( "Folder:" ), mainWidget );
  mainLayout->addWidget( folderLabel, row, 0 );
  mFolderRequester = new FolderRequester( mainWidget, kmkernel->getKMMainWidget()->folderTree() );
  mainLayout->addWidget( mFolderRequester, row, 1 );
  row++;

  QLabel *fileNameLabel = new QLabel( i18n( "Archive File:" ), mainWidget );
  mainLayout->addWidget( fileNameLabel, row, 0 );
  mUrlRequester = new KURLRequester( mainWidget );
  mUrlRequester->setMode( KFile::LocalOnly );
  mUrlRequester->setFilter( "*.tar *.zip *.tar.gz *.tar.bz2" );
  mainLayout->addWidget( mUrlRequester, row, 1 );
  row++;

  // TODO: what's this, tooltips

  mainLayout->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding ), row, 0 );
}

void ImportArchiveDialog::setFolder( KMFolder *defaultFolder )
{
  mFolderRequester->setFolder( defaultFolder );
}

void ImportArchiveDialog::slotOk()
{
  if ( !QFile::exists( mUrlRequester->url() ) ) {
    KMessageBox::information( this, i18n( "Please select an archive file that should be imported." ),
                              i18n( "No archive file selected" ) );
    return;
  }

  // TODO: check if url is empty. or better yet, disable ok button until file is chosen

  ImportJob *importJob = new KMail::ImportJob( mParentWidget );
  importJob->setFile( mUrlRequester->url() );
  importJob->setRootFolder( mFolderRequester->folder() );
  importJob->start();
  accept();
}

#include "importarchivedialog.moc"
