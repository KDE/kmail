/*
  SPDX-FileCopyrightText: 2016-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "identityinvalidfolder.h"

using namespace KMail;

IdentityInvalidFolder::IdentityInvalidFolder(QWidget *parent)
    : KMessageWidget(parent)
{
    setMessageType(KMessageWidget::Warning);
    setCloseButtonVisible(false);
    hide();
}

IdentityInvalidFolder::~IdentityInvalidFolder() = default;

void IdentityInvalidFolder::setErrorMessage(const QString &msg)
{
    animatedShow();
    setText(msg);
}
