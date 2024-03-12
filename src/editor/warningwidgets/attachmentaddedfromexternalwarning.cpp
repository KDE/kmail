/*
   SPDX-FileCopyrightText: 2020-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "attachmentaddedfromexternalwarning.h"

#include <QUrl>

#include <KLocalizedString>

AttachmentAddedFromExternalWarning::AttachmentAddedFromExternalWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setPosition(KMessageWidget::Header);
    setVisible(false);
    setCloseButtonVisible(true);
    setMessageType(Information);
    setWordWrap(true);
}

AttachmentAddedFromExternalWarning::~AttachmentAddedFromExternalWarning() = default;

void AttachmentAddedFromExternalWarning::setAttachmentNames(const QStringList &lst)
{
    QStringList attachments;

    for (const QString &item : lst) {
        const QUrl url(item);

        if (url.isLocalFile()) {
            attachments << url.toLocalFile().toHtmlEscaped();
        } else {
            attachments << QString(item).toHtmlEscaped();
        }
    }

    if (attachments.count() == 1) {
        setText(i18n("This attachment: <ul><li>%1</li></ul> was added externally. Remove it if it's an error.", attachments.at(0)));
    } else {
        setText(
            i18n("These attachments: <ul><li>%1</li></ul> were added externally. Remove them if it's an error.", attachments.join(QLatin1String("</li><li>"))));
    }
}

#include "moc_attachmentaddedfromexternalwarning.cpp"
