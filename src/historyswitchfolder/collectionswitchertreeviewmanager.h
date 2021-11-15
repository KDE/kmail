/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: GPL-2.0-only
*/
#pragma once

#include <QObject>

class CollectionSwitcherTreeViewManager : public QObject
{
    Q_OBJECT
public:
    explicit CollectionSwitcherTreeViewManager(QObject *parent = nullptr);
    ~CollectionSwitcherTreeViewManager() override;
};
