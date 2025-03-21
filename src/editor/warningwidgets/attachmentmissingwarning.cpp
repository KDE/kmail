/*
   SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "attachmentmissingwarning.h"

#include <KLocalizedString>
#include <QAction>
#include <QIcon>

using namespace Qt::Literals::StringLiterals;
AttachmentMissingWarning::AttachmentMissingWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setVisible(false);
    setCloseButtonVisible(false);
    setMessageType(Information);
    setPosition(KMessageWidget::Header);
    setText(i18n(
        "The message you have composed seems to refer to an attached file but you have not attached anything. Do you want to attach a file to your message?"));
    setWordWrap(true);

    auto action = new QAction(QIcon::fromTheme(QStringLiteral("mail-attachment")), i18n("&Attach file"), this);
    action->setObjectName("attachfileaction"_L1);
    connect(action, &QAction::triggered, this, &AttachmentMissingWarning::slotAttachFile);
    addAction(action);

    action = new QAction(QIcon::fromTheme(QStringLiteral("window-close")), i18n("&Remind me later"), this);
    action->setObjectName("remindmelater"_L1);
    connect(action, &QAction::triggered, this, &AttachmentMissingWarning::explicitlyClosed);
    addAction(action);
}

AttachmentMissingWarning::~AttachmentMissingWarning() = default;

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

#include "moc_attachmentmissingwarning.cpp"
