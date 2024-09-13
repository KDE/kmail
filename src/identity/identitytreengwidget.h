/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <KIdentityManagementWidgets/IdentityTreeView>
#include <QWidget>
namespace KIdentityManagementCore
{
class Identity;
class IdentityManager;
}

namespace KMail
{
class IdentityTreeNgWidget : public KIdentityManagementWidgets::IdentityTreeView
{
    Q_OBJECT
public:
    explicit IdentityTreeNgWidget(QWidget *parent = nullptr);
    ~IdentityTreeNgWidget() override;

    KIdentityManagementCore::IdentityManager *identityManager() const;
    void setIdentityManager(KIdentityManagementCore::IdentityManager *im);

Q_SIGNALS:
    void contextMenuRequested(const QPoint &);

private:
    void slotCustomContextMenuRequested(const QPoint &pos);
    KIdentityManagementCore::IdentityManager *mIdentityManager = nullptr;
};
}
