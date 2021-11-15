/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: GPL-2.0-only
*/

#include "collectionswitchertreeviewmanager.h"

CollectionSwitcherTreeViewManager::CollectionSwitcherTreeViewManager(QObject *parent)
    : QObject{parent}
{
}

CollectionSwitcherTreeViewManager::~CollectionSwitcherTreeViewManager()
{
}

QWidget *CollectionSwitcherTreeViewManager::parentWidget() const
{
    return mParentWidget;
}

void CollectionSwitcherTreeViewManager::setParentWidget(QWidget *newParentWidget)
{
    mParentWidget = newParentWidget;
}
