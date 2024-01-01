/*
   SPDX-FileCopyrightText: 2021-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "toomanyrecipientswarning.h"
#include <KLocalizedString>
#include <MessageComposer/MessageComposerSettings>
TooManyRecipientsWarning::TooManyRecipientsWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setPosition(KMessageWidget::Header);
    setVisible(false);
    setCloseButtonVisible(true);
    setMessageType(Warning);
    setWordWrap(true);
    setText(i18nc("@info:status",
                  "We have reached maximum recipients. Truncating recipients list to %1.",
                  MessageComposer::MessageComposerSettings::self()->maximumRecipients()));
}

TooManyRecipientsWarning::~TooManyRecipientsWarning() = default;

#include "moc_toomanyrecipientswarning.cpp"
