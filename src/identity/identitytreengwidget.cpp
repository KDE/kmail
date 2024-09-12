/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "identitytreengwidget.h"

IdentityTreeNgWidget::IdentityTreeNgWidget(QWidget *parent)
    : KIdentityManagementWidgets::IdentityTreeView(parent)
{
}

IdentityTreeNgWidget::~IdentityTreeNgWidget() = default;

#include "moc_identitytreengwidget.cpp"
