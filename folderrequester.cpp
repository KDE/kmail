/*
 * Copyright (c) 2004 Carsten Burghardt <burghardt@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */


#include "folderrequester.h"

#include "messageviewer/autoqpointer.h"

#include <kdebug.h>
#include <klineedit.h>
#include <kiconloader.h>
#include <KLocale>
#include <kdialog.h>
#include "folderselectiontreeviewdialog.h"
#include <QLayout>
#include <QToolButton>
#include <QHBoxLayout>
#include <QKeyEvent>

namespace KMail {

FolderRequester::FolderRequester( QWidget *parent )
  : QWidget( parent ),
    mMustBeReadWrite( true ), mShowOutbox( true ), mShowImapFolders( true )
{
  QHBoxLayout * hlay = new QHBoxLayout( this );
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  edit = new KLineEdit( this );
  edit->setReadOnly( true );
  hlay->addWidget( edit );

  QToolButton* button = new QToolButton( this );
  button->setIcon( KIcon( "folder" ) );
  button->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  hlay->addWidget( button );
  connect( button, SIGNAL(clicked()), this, SLOT(slotOpenDialog()) );

  setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
        QSizePolicy::Fixed ) );
  setFocusPolicy( Qt::StrongFocus );
}

//-----------------------------------------------------------------------------
void FolderRequester::slotOpenDialog()
{
  AutoQPointer<FolderSelectionTreeViewDialog> dlg( new FolderSelectionTreeViewDialog( this ) );
  dlg->setCaption( i18n("Select Folder") );
  dlg->setModal( false );

  if ( dlg->exec() && dlg ) {
    setFolder( dlg->selectedCollection() );
  }
#if 0
  AutoQPointer<FolderSelectionDialog> dlg( new FolderSelectionDialog( this, mFolderTree,
                                                                      i18n("Select Folder"),
                                                                      mMustBeReadWrite, false ) );
  dlg->setFlags( mMustBeReadWrite, mShowOutbox, mShowImapFolders );
  dlg->setFolder( mFolder );

  if ( dlg->exec() && dlg ) {
    setFolder( dlg->folder() );
  }
#endif
}

//-----------------------------------------------------------------------------
FolderRequester::~FolderRequester()
{
}

Akonadi::Collection FolderRequester::folderCollection() const
{
  return mCollection;
}

//-----------------------------------------------------------------------------
void FolderRequester::setFolder( const Akonadi::Collection&col )
{
  mCollection = col;
  if ( mCollection.isValid() ) {
#if 0 //TODO port it
    edit->setText( mFolder->prettyUrl() );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
    mFolderId = QString::number( mCollection.id() );
  }
  else if ( !mMustBeReadWrite ) // the Local Folders root node was selected
    edit->setText( i18n("Local Folders") );
  emit folderChanged( mCollection );
}

//-----------------------------------------------------------------------------
void FolderRequester::setFolder( const QString &idString )
{
#if 0
  KMFolder *folder = kmkernel->findFolderById( idString );
  if ( folder ) {
    setFolder( folder );
  } else {
    if ( !idString.isEmpty() ) {
      edit->setText( i18n( "Unknown folder '%1'", idString ) );
    } else {
      edit->setText( i18n( "Please select a folder" ) );
    }
    mFolder = 0;
  }
  mFolderId = idString;
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

//-----------------------------------------------------------------------------
void FolderRequester::keyPressEvent( QKeyEvent * e )
{
  if ( e->key() == Qt::Key_Space )
    slotOpenDialog();
  else
    e->ignore();
}
} // namespace KMail

#include "folderrequester.moc"
