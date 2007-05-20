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

#include "config.h"

#include "folderrequester.h"
#include "kmfolder.h"
#include "kmfoldertree.h"
#include "folderselectiondialog.h"

#include <kdebug.h>
#include <klineedit.h>
#include <kiconloader.h>
#include <kdialog.h>

#include <QLayout>
#include <QToolButton>
//Added by qt3to4:
#include <QHBoxLayout>
#include <QKeyEvent>

namespace KMail {

FolderRequester::FolderRequester( QWidget *parent, KMFolderTree *tree )
  : QWidget( parent ), mFolder( 0 ), mFolderTree( tree ),
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
  button->setIconSize( QSize( K3Icon::SizeSmall, K3Icon::SizeSmall ) );
  hlay->addWidget( button );
  connect( button, SIGNAL(clicked()), this, SLOT(slotOpenDialog()) );

  setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
        QSizePolicy::Fixed ) );
  setFocusPolicy( Qt::StrongFocus );
}

//-----------------------------------------------------------------------------
void FolderRequester::slotOpenDialog()
{
  FolderSelectionDialog dlg( this, mFolderTree, i18n("Select Folder"),
      mMustBeReadWrite, false );
  dlg.setFlags( mMustBeReadWrite, mShowOutbox, mShowImapFolders );
  dlg.setFolder( mFolder );

  if (!dlg.exec()) return;
  setFolder( dlg.folder() );
}

//-----------------------------------------------------------------------------
FolderRequester::~FolderRequester()
{
}

//-----------------------------------------------------------------------------
KMFolder * FolderRequester::folder( void ) const
{
  return mFolder;
}

//-----------------------------------------------------------------------------
void FolderRequester::setFolder( KMFolder *folder )
{
  mFolder = folder;
  if ( mFolder ) {
    edit->setText( mFolder->prettyUrl() );
    mFolderId = mFolder->idString();
  }
  else if ( !mMustBeReadWrite ) // the Local Folders root node was selected
    edit->setText( i18n("Local Folders") );
  emit folderChanged( folder );
}

//-----------------------------------------------------------------------------
void FolderRequester::setFolder( const QString &idString )
{
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
