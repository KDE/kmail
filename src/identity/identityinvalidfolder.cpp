/*
  SPDX-FileCopyrightText: 2016-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "identityinvalidfolder.h"

using namespace KMail;
using namespace Qt::Literals::StringLiterals;

IdentityInvalidFolder::IdentityInvalidFolder(QWidget *parent)
    : KMessageWidget(parent)
{
    setPosition(KMessageWidget::Header);
    setMessageType(KMessageWidget::Warning);
    setIcon(QIcon::fromTheme("dialog-warning"_L1));
    setFocusPolicy(Qt::NoFocus);
    setCloseButtonVisible(false);
    hide();
}

IdentityInvalidFolder::~IdentityInvalidFolder() = default;

void IdentityInvalidFolder::setErrorMessage(const QString &msg)
{
    animatedShow();
    setText(msg);
}

#include "moc_identityinvalidfolder.cpp"
