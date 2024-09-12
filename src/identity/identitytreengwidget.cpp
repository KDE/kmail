/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "identitytreengwidget.h"

IdentityTreeNgWidget::IdentityTreeNgWidget(QWidget *parent)
    : KIdentityManagementWidgets::IdentityTreeView(parent)
{
    connect(this, &QTreeView::customContextMenuRequested, this, &IdentityTreeNgWidget::slotCustomContextMenuRequested);
}

IdentityTreeNgWidget::~IdentityTreeNgWidget() = default;

void IdentityTreeNgWidget::slotCustomContextMenuRequested(const QPoint &pos)
{
    // TODO
}

#include "moc_identitytreengwidget.cpp"
