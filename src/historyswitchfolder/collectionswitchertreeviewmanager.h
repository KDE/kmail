/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: GPL-2.0-only
*/
#pragma once

#include <Akonadi/Collection>
#include <QObject>
class CollectionSwitcherTreeView;
class CollectionSwitcherModel;
class CollectionSwitcherTreeViewManager : public QObject
{
    Q_OBJECT
public:
    explicit CollectionSwitcherTreeViewManager(QObject *parent = nullptr);
    ~CollectionSwitcherTreeViewManager() override;

    Q_REQUIRED_RESULT QWidget *parentWidget() const;
    void setParentWidget(QWidget *newParentWidget);

Q_SIGNALS:
    void switchToFolder(const Akonadi::Collection &col);

private:
    void activateCollection(const QModelIndex &index);
    void switchToCollectionClicked(const QModelIndex &index);
    QWidget *mParentWidget = nullptr;
    CollectionSwitcherTreeView *const mCollectionSwitcherTreeView;
    CollectionSwitcherModel *const mCollectionSwitcherModel;
};
