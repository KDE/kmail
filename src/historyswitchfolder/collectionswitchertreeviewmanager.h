/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: GPL-2.0-only
*/
#pragma once

#include "kmail_private_export.h"
#include <Akonadi/Collection>
#include <QList>
#include <QObject>
class CollectionSwitcherTreeView;
class CollectionSwitcherModel;
class QAction;

class KMAILTESTS_TESTS_EXPORT CollectionSwitcherTreeViewManager : public QObject
{
    Q_OBJECT
public:
    explicit CollectionSwitcherTreeViewManager(QObject *parent = nullptr);
    ~CollectionSwitcherTreeViewManager() override;

    void addActions(const QList<QAction *> &lst);

    Q_REQUIRED_RESULT QWidget *parentWidget() const;
    void setParentWidget(QWidget *newParentWidget);

    void selectForward();
    void selectBackward();
    void updateViewGeometry();

    void addHistory(const Akonadi::Collection &currentCol, const QString &fullPath);

    Q_REQUIRED_RESULT CollectionSwitcherTreeView *collectionSwitcherTreeView() const;

Q_SIGNALS:
    void switchToFolder(const Akonadi::Collection &col);

private:
    void activateCollection(const QModelIndex &index);
    void switchToCollectionClicked(const QModelIndex &index);
    void selectCollection(const int from, const int to);
    QWidget *mParentWidget = nullptr;
    CollectionSwitcherTreeView *const mCollectionSwitcherTreeView;
    CollectionSwitcherModel *const mCollectionSwitcherModel;
};
