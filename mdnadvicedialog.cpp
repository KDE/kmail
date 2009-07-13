/*
  Copyright (c) 2009 Michael Leupold <lemma@confuego.org>

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

#include "mdnadvicedialog.h"

#include <KMessageBox>
#include <KLocalizedString>
#include <QtCore/QPointer>

MDNAdviceDialog::MDNAdviceDialog( const QString &text, bool canDeny, QWidget *parent )
  : KDialog( parent ), m_result(MDNIgnore)
{
  setCaption( i18n( "Message Disposition Notification Request" ) );
  if ( canDeny ) {
    setButtons( KDialog::Yes | KDialog::User1 | KDialog::User2 );
    setButtonText( KDialog::User2, "Send \"&denied\"" );
  } else {
    setButtons( KDialog::Yes | KDialog::User1 );
  }
  setButtonText( KDialog::Yes, "&Ignore" );
  setButtonText( KDialog::User1, "&Send" );
  setEscapeButton( KDialog::Yes );
  KMessageBox::createKMessageBox( this, QMessageBox::Question, text,
                                  QStringList(), QString(), 0, KMessageBox::NoExec );
  connect( this, SIGNAL( buttonClicked( KDialog::ButtonCode ) ),
           this, SLOT( slotButtonClicked( KDialog::ButtonCode ) ) );
}

MDNAdviceDialog::~MDNAdviceDialog()
{
}

MDNAdviceDialog::MDNAdvice MDNAdviceDialog::questionIgnoreSend( const QString &text, bool canDeny )
{
  MDNAdvice rc = MDNIgnore;
  QPointer<MDNAdviceDialog> dlg( new MDNAdviceDialog( text, canDeny ) );
  dlg->exec();
  if ( dlg ) {
    rc = dlg->m_result;
  }
  delete dlg;
  return rc;
}

void MDNAdviceDialog::slotButtonClicked( int button )
{
  switch ( button ) {

    case KDialog::User1:
      m_result = MDNSend;
      accept();
      break;

    case KDialog::User2:
      m_result = MDNSendDenied;
      accept();
      break;

    case KDialog::Yes:
    default:
      m_result = MDNIgnore;
      break;

  }
  reject();
}

#include "mdnadvicedialog.moc"
