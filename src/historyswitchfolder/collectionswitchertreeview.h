/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021-2025 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "kmail_private_export.h"
#include <QTreeView>

class KMAILTESTS_TESTS_EXPORT CollectionSwitcherTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit CollectionSwitcherTreeView(QWidget *parent = nullptr);
    ~CollectionSwitcherTreeView() override;

    [[nodiscard]] int sizeHintWidth() const;
    void resizeColumnsToContents();

Q_SIGNALS:
    void collectionSelected(const QModelIndex &index);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
};
