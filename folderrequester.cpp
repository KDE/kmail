/**
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#include "kmfoldertree.h"
#include "kmfolderseldlg.h"

#include <kdebug.h>
#include <klineedit.h>
#include <kdialog.h>

#include <qlayout.h>
#include <qtoolbutton.h>

namespace KMail {

FolderRequester::FolderRequester( QWidget *parent, KMFolderTree *tree )
  : QWidget( parent ), mFolder( 0 ), mFolderTree( tree ), 
    mMustBeReadWrite( true ), mShowOutbox( true ), mShowImapFolders( true )
{
  QHBoxLayout * hlay = new QHBoxLayout( this, 0, KDialog::spacingHint() );
  hlay->setAutoAdd( true );

  edit = new KLineEdit( this );
  edit->setReadOnly( true );

  QToolButton* button = new QToolButton( this );
  button->setAutoRaise(true);
  button->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  button->setFixedSize( 16, 16 );
  button->setIconSet( KGlobal::iconLoader()->loadIconSet( "up", KIcon::Small, 0 ) );
  connect( button, SIGNAL(clicked()), this, SLOT(slotOpenDialog()) );

  setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
        QSizePolicy::Fixed ) );
}

//-----------------------------------------------------------------------------
void FolderRequester::slotOpenDialog()
{
  KMFolderSelDlg dlg( this, mFolderTree, i18n("Select Folder"), 
      mMustBeReadWrite, false );
  dlg.setFlags( mMustBeReadWrite, mShowOutbox, mShowImapFolders );
  dlg.setFolder( mFolder );
  KMFolder* dest;

  if (!dlg.exec()) return;
  if (!(dest = dlg.folder())) return;
  setFolder( dest );
}

//-----------------------------------------------------------------------------
FolderRequester::~FolderRequester()
{
}

//-----------------------------------------------------------------------------
KMFolder * FolderRequester::folder( void )
{
  return mFolder;
}

//-----------------------------------------------------------------------------
void FolderRequester::setFolder( KMFolder *folder )
{
  mFolder = folder;
  if ( mFolder )
    edit->setText( mFolder->label() );
  emit folderChanged( folder );
}

//-----------------------------------------------------------------------------
void FolderRequester::setFolder( const QString &idString )
{
  setFolder( kmkernel->findFolderById( idString ) );
}

} // namespace KMail

#include "folderrequester.moc"
