/*
   SPDX-FileCopyrightText: 2010-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <MessageList/Pane>
#include <MessageList/StorageModel>

class CollectionPane : public MessageList::Pane
{
    Q_OBJECT
public:
    explicit CollectionPane(bool restoreSession, QAbstractItemModel *model, QItemSelectionModel *selectionModel, QWidget *parent = nullptr);
    ~CollectionPane() override;

    MessageList::StorageModel *createStorageModel(QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent) override;
    void writeConfig(bool restoreSession) override;
};

class CollectionStorageModel : public MessageList::StorageModel
{
    Q_OBJECT
public:
    /**
     * Create a StorageModel wrapping the specified folder.
     */
    explicit CollectionStorageModel(QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent = nullptr);
    ~CollectionStorageModel() override;
    Q_REQUIRED_RESULT bool isOutBoundFolder(const Akonadi::Collection &c) const override;
};

