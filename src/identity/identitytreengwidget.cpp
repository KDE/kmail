/*
  SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "identitytreengwidget.h"
#include <KIdentityManagementCore/IdentityManager>
using namespace KMail;
IdentityTreeNgWidget::IdentityTreeNgWidget(QWidget *parent)
    : KIdentityManagementWidgets::IdentityTreeView(parent)
{
    connect(this, &QTreeView::customContextMenuRequested, this, &IdentityTreeNgWidget::slotCustomContextMenuRequested);
}

IdentityTreeNgWidget::~IdentityTreeNgWidget() = default;

void IdentityTreeNgWidget::slotCustomContextMenuRequested(const QPoint &pos)
{
    Q_EMIT contextMenuRequested(pos);
}

KIdentityManagementCore::IdentityManager *IdentityTreeNgWidget::identityManager() const
{
    Q_ASSERT(mIdentityManager);
    return mIdentityManager;
}

void IdentityTreeNgWidget::setIdentityManager(KIdentityManagementCore::IdentityManager *im)
{
    mIdentityManager = im;
}

#include "moc_identitytreengwidget.cpp"
