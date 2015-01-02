/*
  Copyright (c) 2012-2015 Montel Laurent <montel@kde.org>

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
#include <KLocalizedString>
#include <QAction>
#include <QIcon>

AttachmentMissingWarning::AttachmentMissingWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setVisible(false);
    setCloseButtonVisible(false);
    setMessageType(Information);
    setText(i18n("The message you have composed seems to refer to an attached file but you have not attached anything. Do you want to attach a file to your message?"));
    setWordWrap(true);

    QAction *action = new QAction(QIcon::fromTheme(QLatin1String("mail-attachment")), i18n("&Attach file"), this);
    connect(action, &QAction::triggered, this, &AttachmentMissingWarning::slotAttachFile);
    addAction(action);

    action = new QAction(QIcon::fromTheme(QLatin1String("window-close")), i18n("&Remind me later"), this);
    connect(action, &QAction::triggered, this, &AttachmentMissingWarning::explicitlyClosed);
    addAction(action);

}

AttachmentMissingWarning::~AttachmentMissingWarning()
{
}

void AttachmentMissingWarning::slotAttachFile()
{
    Q_EMIT attachMissingFile();
}

void AttachmentMissingWarning::slotFileAttached()
{
    animatedHide();
    Q_EMIT closeAttachMissingFile();
}

void AttachmentMissingWarning::explicitlyClosed()
{
    animatedHide();
    Q_EMIT explicitClosedMissingAttachment();
}

