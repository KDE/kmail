/*******************************************************************************
**
** Filename   : foldershortcutdialog.cpp
** Created on : 09 October, 2004
** Copyright  : (c) 2004 Till Adam
** Email      : adam@kde.org
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   This program is distributed in the hope that it will be useful,
**   but WITHOUT ANY WARRANTY; without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**   GNU General Public License for more details.
**
**   You should have received a copy of the GNU General Public License
**   along with this program; if not, write to the Free Software
**   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
**
*******************************************************************************/

#include <qlabel.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>

#include <kkeybutton.h>
#include <klocale.h>

#include "foldershortcutdialog.h"
#include "kmfolder.h"

using namespace KMail;

FolderShortcutDialog::FolderShortcutDialog( KMFolder *folder,
                                            QWidget *parent,
                                            const char *name )
:  KDialogBase( parent, name, true,
               i18n( "Shortcut for Folder %1" ).arg( folder->label() ),
               KDialogBase::Ok | KDialogBase::Cancel ),
   mFolder( folder )
{
  QVBox *box = makeVBoxMainWidget();
  QVGroupBox *gb = new QVGroupBox( i18n("Select shortcut for folder"), box );
  QWhatsThis::add( gb, i18n( "<qt>To choose a key or a combination "
                             "of keys which select the current folder, "
                             "click the button below and then press the key(s) "
                             "you wish to associate with this folder.</qt>" ) );
  QHBox *hb = new QHBox( gb );
  new QWidget(hb);
  mKeyButton = new KKeyButton( hb, "FolderShortcutSelector" );
  new QWidget(hb);

  connect( mKeyButton, SIGNAL( capturedShortcut( const KShortcut& ) ),
           this, SLOT( slotCapturedShortcut( const KShortcut& ) ) );
  mKeyButton->setShortcut( folder->shortcut(), false );
}

FolderShortcutDialog::~FolderShortcutDialog()
{
}

void FolderShortcutDialog::slotCapturedShortcut( const KShortcut& sc )
{
  // hm, check if it's valid?
  mKeyButton->setShortcut( sc, false );
}

void FolderShortcutDialog::slotOk()
{
  mFolder->setShortcut( mKeyButton->shortcut() );
  KDialogBase::slotOk();
}

#include "foldershortcutdialog.moc"


