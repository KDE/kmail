/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021-2025 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class CollectionSwitcherTreeViewManagerTest : public QObject
{
    Q_OBJECT
public:
    explicit CollectionSwitcherTreeViewManagerTest(QObject *parent = nullptr);
    ~CollectionSwitcherTreeViewManagerTest() override = default;
private Q_SLOTS:
    void shouldHaveDefaultValues();
};
