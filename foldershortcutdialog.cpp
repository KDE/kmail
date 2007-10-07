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
**   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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


#include <QLabel>
#include <QGroupBox>


#include <kkeysequencewidget.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kvbox.h>

#include "kmmainwidget.h"
#include "foldershortcutdialog.h"
#include "kmfolder.h"

#include <QFrame>

using namespace KMail;

FolderShortcutDialog::FolderShortcutDialog( KMFolder *folder,
                                            KMMainWidget *mainwidget,
                                            QWidget *parent )
:  KDialog( parent ),
   mFolder( folder ), mMainWidget( mainwidget )
{
  setCaption( i18n( "Shortcut for Folder %1", folder->label() ) );
  setButtons( Ok | Cancel );
  QFrame *box = new KVBox( this );
  setMainWidget( box );
  QGroupBox *gb = new QGroupBox(i18n("Select Shortcut for Folder"),box );
  QHBoxLayout *layout = new QHBoxLayout;
  gb->setWhatsThis( i18n( "<qt>To choose a key or a combination "
                             "of keys which select the current folder, "
                             "click the button below and then press the key(s) "
                             "you wish to associate with this folder.</qt>" ) );
  gb->setLayout( layout );
  KHBox *hb = new KHBox;
  layout->addWidget( hb );
  new QWidget(hb);
  mKeySeqWidget = new KKeySequenceWidget( hb );
  mKeySeqWidget->setObjectName( "FolderShortcutSelector" );
  new QWidget(hb);

  connect( this, SIGNAL( okClicked() ), SLOT( slotOk() ) );
  mKeySeqWidget->setKeySequence( folder->shortcut().primary(),
                                 KKeySequenceWidget::NoValidate );
  mKeySeqWidget->setCheckActionList(mMainWidget->actionList());
}

FolderShortcutDialog::~FolderShortcutDialog()
{
}

void FolderShortcutDialog::slotOk()
{
  mKeySeqWidget->applyStealShortcut();
  mFolder->setShortcut( KShortcut(mKeySeqWidget->keySequence(), QKeySequence()) );
}

#include "foldershortcutdialog.moc"


