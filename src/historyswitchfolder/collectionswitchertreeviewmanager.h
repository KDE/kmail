/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021-2025 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: LGPL-2.0-or-later
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

    [[nodiscard]] QWidget *parentWidget() const;
    void setParentWidget(QWidget *newParentWidget);

    void selectForward();
    void selectBackward();
    void updateViewGeometry();

    void addHistory(const Akonadi::Collection &currentCol, const QString &fullPath);

    [[nodiscard]] CollectionSwitcherTreeView *collectionSwitcherTreeView() const;

Q_SIGNALS:
    void switchToFolder(const Akonadi::Collection &col);

private:
    KMAIL_NO_EXPORT void activateCollection(const QModelIndex &index);
    KMAIL_NO_EXPORT void switchToCollectionClicked(const QModelIndex &index);
    KMAIL_NO_EXPORT void selectCollection(const int from, const int to);
    QWidget *mParentWidget = nullptr;
    CollectionSwitcherTreeView *const mCollectionSwitcherTreeView;
    CollectionSwitcherModel *const mCollectionSwitcherModel;
};
