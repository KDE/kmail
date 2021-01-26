/*
   SPDX-FileCopyrightText: 2020-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
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

AttachmentAddedFromExternalWarning::~AttachmentAddedFromExternalWarning() = default;

void AttachmentAddedFromExternalWarning::setAttachmentNames(const QStringList &lst)
{
    if (lst.count() == 1) {
        setText(i18n("This attachment: <ul><li>%1</li></ul> was added externally. Remove it if it's an error.", lst.at(0)));
    } else {
        setText(i18n("These attachments: <ul><li>%1</li></ul> were added externally. Remove them if it's an error.", lst.join(QLatin1String("</li><li>"))));
    }
}
