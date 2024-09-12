/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <KIdentityManagementWidgets/IdentityTreeView>
#include <QWidget>

class IdentityTreeNgWidget : public KIdentityManagementWidgets::IdentityTreeView
{
    Q_OBJECT
public:
    explicit IdentityTreeNgWidget(QWidget *parent = nullptr);
    ~IdentityTreeNgWidget() override;

Q_SIGNALS:
    void contextMenuRequested(const QPoint &);

private:
    void slotCustomContextMenuRequested(const QPoint &pos);
};
