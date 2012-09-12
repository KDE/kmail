/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>
  
  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "attachmentmissingwarning.h"
#include <KLocale>
#include <KAction>
#include <KIcon>

AttachmentMissingWarning::AttachmentMissingWarning(QWidget *parent)
    :KMessageWidget(parent)
{
  setVisible(false);
  setCloseButtonVisible(true);
  setMessageType(Information);
  setText( i18n( "The message you have composed seems to refer to an attached file but you have not attached anything. Do you want to attach a file to your message?" ) );
  setWordWrap(true);

  KAction *action = this->findChild<KAction *>(); // should give us the close action...
  if ( action ) {
    connect( action, SIGNAL(triggered(bool)), SLOT(explicitlyClosed()) );
  }

  action = new KAction( KIcon(QLatin1String( "mail-attachment" )), i18n( "&Attach file" ), this );
  connect( action, SIGNAL(triggered(bool)), SLOT(slotAttachFile()) );
  addAction( action );

}

AttachmentMissingWarning::~AttachmentMissingWarning()
{

}

void AttachmentMissingWarning::slotAttachFile()
{
  Q_EMIT attachMissingFile();
  setVisible(false);
}

void AttachmentMissingWarning::explicitlyClosed()
{
  Q_EMIT closeAttachMissingFile();
}

#include "attachmentmissingwarning.moc"
