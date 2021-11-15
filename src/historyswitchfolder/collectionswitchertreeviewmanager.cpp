/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: GPL-2.0-only
*/

#include "collectionswitchertreeviewmanager.h"
#include "collectionswitchermodel.h"
#include "collectionswitchertreeview.h"

CollectionSwitcherTreeViewManager::CollectionSwitcherTreeViewManager(QObject *parent)
    : QObject{parent}
    , mCollectionSwitcherTreeView(new CollectionSwitcherTreeView(nullptr)) // TODO use parent ???
    , mCollectionSwitcherModel(new CollectionSwitcherModel(this))
{
    mCollectionSwitcherTreeView->setModel(mCollectionSwitcherModel);
    connect(mCollectionSwitcherTreeView, &CollectionSwitcherTreeView::pressed, this, &CollectionSwitcherTreeViewManager::switchToCollectionClicked);
    connect(mCollectionSwitcherTreeView, &CollectionSwitcherTreeView::collectionSelected, this, &CollectionSwitcherTreeViewManager::activateCollection);
}

CollectionSwitcherTreeViewManager::~CollectionSwitcherTreeViewManager()
{
}

void CollectionSwitcherTreeViewManager::activateCollection(const QModelIndex &index)
{
    Q_UNUSED(index)
    if (mCollectionSwitcherTreeView->selectionModel()->selectedRows().isEmpty()) {
        return;
    }

    const int row = mCollectionSwitcherTreeView->selectionModel()->selectedRows().first().row();
    // TODO activate it. Q_EMIT switchToFolder(...)

    mCollectionSwitcherTreeView->hide();
}

void CollectionSwitcherTreeViewManager::switchToCollectionClicked(const QModelIndex &index)
{
    mCollectionSwitcherTreeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    activateCollection(index);
}

QWidget *CollectionSwitcherTreeViewManager::parentWidget() const
{
    return mParentWidget;
}

void CollectionSwitcherTreeViewManager::setParentWidget(QWidget *newParentWidget)
{
    mParentWidget = newParentWidget;
}
