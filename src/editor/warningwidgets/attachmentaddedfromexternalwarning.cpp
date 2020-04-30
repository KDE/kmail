/*
   Copyright (C) 2020 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "attachmentaddedfromexternalwarning.h"
#include <KLocalizedString>

AttachmentAddedFromExternalWarning::AttachmentAddedFromExternalWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setVisible(false);
    setCloseButtonVisible(true);
    setMessageType(Information);
    setWordWrap(true);
}

AttachmentAddedFromExternalWarning::~AttachmentAddedFromExternalWarning()
{
}

void AttachmentAddedFromExternalWarning::setAttachmentNames(const QStringList &lst)
{
    if (lst.count() == 1) {
        setText(i18n("This attachment: <ul><li>%1</li></ul> was added externally. Remove it if it's an error.", lst.at(0)));
    } else {
        setText(i18n("These attachments: <ul><li>%1</li></ul> were added externally. Remove them if it's an error.", lst.join(QLatin1String("</li><li>"))));
    }
}
