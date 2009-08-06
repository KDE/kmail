/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Martin Koller <kollix@aon.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <attachmentdialog.h>

#include <kdialog.h>
#include <kmessagebox.h>
#include <klocale.h>

namespace KMail
{

//---------------------------------------------------------------------

AttachmentDialog::AttachmentDialog( QWidget *parent, const QString &filenameText,
                                    const QString &application, const QString &dontAskAgainName )
  : dontAskName( dontAskAgainName )
{
  text = i18n( "Open attachment '%1'?\n"
               "Note that opening an attachment may compromise "
               "your system's security.",
               filenameText );

  dialog = new KDialog( parent );
  dialog->setCaption( i18n("Open Attachment?") );
  dialog->setObjectName( "attachmentSaveOpen" );

  if ( application.isEmpty() )
    dialog->setButtons( KDialog::User3 | KDialog::User1 | KDialog::Cancel );
  else {
    dialog->setButtons( KDialog::User3 | KDialog::User2 | KDialog::User1 | KDialog::Cancel );
    dialog->setButtonText( KDialog::User2, i18n("&Open with '%1'", application ) );
  }

  dialog->setButtonGuiItem( KDialog::User3, KStandardGuiItem::saveAs() );
  dialog->setButtonText( KDialog::User1, i18n("&Open With...") );
  dialog->setDefaultButton( KDialog::User3 );

  connect( dialog, SIGNAL( user3Clicked() ), this, SLOT( saveClicked() ) );
  connect( dialog, SIGNAL( user2Clicked() ), this, SLOT( openClicked() ) );
  connect( dialog, SIGNAL( user1Clicked() ), this, SLOT( openWithClicked() ) );
}

//---------------------------------------------------------------------

int AttachmentDialog::exec()
{
  KConfigGroup cg( KGlobal::config().data(), "Notification Messages" );
  if ( cg.hasKey( dontAskName ) )
    return cg.readEntry( dontAskName, int(0) );

  bool again = false;
  const int ret =
    KMessageBox::createKMessageBox( dialog, QMessageBox::Question, text, QStringList(),
                                    i18n( "Do not ask again" ), &again, 0 );

  if ( ret == QDialog::Rejected )
    return Cancel;
  else {
    if ( again ) {
      KConfigGroup::WriteConfigFlags flags = KConfig::Persistent;
      KConfigGroup cg( KGlobal::config().data(), "Notification Messages" );
      cg.writeEntry( dontAskName, ret, flags );
      cg.sync();
    }

    return ret;
  }
}

//---------------------------------------------------------------------

void AttachmentDialog::saveClicked()
{
  dialog->done( Save );
}

//---------------------------------------------------------------------

void AttachmentDialog::openClicked()
{
  dialog->done( Open );
}

//---------------------------------------------------------------------

void AttachmentDialog::openWithClicked()
{
  dialog->done( OpenWith );
}

//---------------------------------------------------------------------

}  // namespace

#include "attachmentdialog.moc"
