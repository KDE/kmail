/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021-2023 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: GPL-2.0-only
*/

#include "collectionswitchertreeviewmanager.h"
#include "collectionswitchermodel.h"
#include "collectionswitchertreeview.h"
#include "kmail_debug.h"
#include <QScrollBar>

CollectionSwitcherTreeViewManager::CollectionSwitcherTreeViewManager(QObject *parent)
    : QObject{parent}
    , mCollectionSwitcherTreeView(new CollectionSwitcherTreeView(nullptr))
    , mCollectionSwitcherModel(new CollectionSwitcherModel(this))
{
    mCollectionSwitcherTreeView->setModel(mCollectionSwitcherModel);
    connect(mCollectionSwitcherTreeView, &CollectionSwitcherTreeView::pressed, this, &CollectionSwitcherTreeViewManager::switchToCollectionClicked);
    connect(mCollectionSwitcherTreeView, &CollectionSwitcherTreeView::collectionSelected, this, &CollectionSwitcherTreeViewManager::activateCollection);
}

CollectionSwitcherTreeViewManager::~CollectionSwitcherTreeViewManager()
{
    delete mCollectionSwitcherTreeView;
}

void CollectionSwitcherTreeViewManager::addActions(const QList<QAction *> &lst)
{
    // Make sure that actions works when mCollectionSwitcherTreeView is show.
    mCollectionSwitcherTreeView->addActions(lst);
}

void CollectionSwitcherTreeViewManager::activateCollection(const QModelIndex &index)
{
    Q_UNUSED(index)
    if (mCollectionSwitcherTreeView->selectionModel()->selectedRows().isEmpty()) {
        return;
    }

    const int row = mCollectionSwitcherTreeView->selectionModel()->selectedRows().first().row();
    const Akonadi::Collection col = mCollectionSwitcherModel->collection(row);
    Q_EMIT switchToFolder(col);

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

void CollectionSwitcherTreeViewManager::selectCollection(const int from, const int to)
{
    QModelIndex index;
    const int step = from < to ? 1 : -1;
    if (!mCollectionSwitcherTreeView->isVisible()) {
        updateViewGeometry();
        index = mCollectionSwitcherModel->index(from + step, 0);
        if (!index.isValid()) {
            index = mCollectionSwitcherModel->index(0, 0);
        }
        mCollectionSwitcherTreeView->show();
        mCollectionSwitcherTreeView->setFocus();
    } else {
        int newRow = mCollectionSwitcherTreeView->selectionModel()->currentIndex().row() + step;
        if (newRow == to + step) {
            newRow = from;
        }
        index = mCollectionSwitcherModel->index(newRow, 0);
    }

    mCollectionSwitcherTreeView->selectionModel()->select(index, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
    mCollectionSwitcherTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

CollectionSwitcherTreeView *CollectionSwitcherTreeViewManager::collectionSwitcherTreeView() const
{
    return mCollectionSwitcherTreeView;
}

void CollectionSwitcherTreeViewManager::selectForward()
{
    selectCollection(0, mCollectionSwitcherModel->rowCount() - 1);
}

void CollectionSwitcherTreeViewManager::selectBackward()
{
    selectCollection(mCollectionSwitcherModel->rowCount() - 1, 0);
}

void CollectionSwitcherTreeViewManager::updateViewGeometry()
{
    QWidget *window = mParentWidget ? mParentWidget->window() : nullptr;
    if (window) {
        const QSize centralSize = window->size();

        const QSize viewMaxSize(centralSize.width() * 3 / 4, centralSize.height() * 3 / 4);

        const int rowHeight = mCollectionSwitcherTreeView->sizeHintForRow(0);
        const int frameWidth = mCollectionSwitcherTreeView->frameWidth();
        const QSize viewSize(std::min(mCollectionSwitcherTreeView->sizeHintWidth() + 2 * frameWidth + mCollectionSwitcherTreeView->verticalScrollBar()->width(),
                                      viewMaxSize.width()),
                             std::min(std::max(rowHeight * mCollectionSwitcherModel->rowCount() + 2 * frameWidth, rowHeight * 6), viewMaxSize.height()));

        // Position should be central over the editor area, so map to global from
        // parent of central widget since the view is positioned in global coords
        const QPoint centralWidgetPos = window->parentWidget() ? window->mapToGlobal(window->pos()) : window->pos();
        const int xPos = std::max(0, centralWidgetPos.x() + (centralSize.width() - viewSize.width()) / 2);
        const int yPos = std::max(0, centralWidgetPos.y() + (centralSize.height() - viewSize.height()) / 2);

        mCollectionSwitcherTreeView->setFixedSize(viewSize);
        mCollectionSwitcherTreeView->move(xPos, yPos);
    } else {
        qCWarning(KMAIL_LOG) << "Problem mParentWidget is null! it's a bug";
    }
}

void CollectionSwitcherTreeViewManager::addHistory(const Akonadi::Collection &currentCol, const QString &fullPath)
{
    mCollectionSwitcherModel->addHistory(currentCol, fullPath);
}

#include "moc_collectionswitchertreeviewmanager.cpp"
