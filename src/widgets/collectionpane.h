/*
   Copyright (C) 2010-2016 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef COLLECTIONPANE_H
#define COLLECTIONPANE_H

#include <messagelist/pane.h>
#include <MessageList/StorageModel>

class CollectionPane : public MessageList::Pane
{
    Q_OBJECT
public:
    explicit CollectionPane(bool restoreSession, QAbstractItemModel *model, QItemSelectionModel *selectionModel, QWidget *parent = Q_NULLPTR);
    ~CollectionPane();

    MessageList::StorageModel *createStorageModel(QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent) Q_DECL_OVERRIDE;
    void writeConfig(bool restoreSession) Q_DECL_OVERRIDE;
};

class CollectionStorageModel : public MessageList::StorageModel
{
    Q_OBJECT
public:
    /**
    * Create a StorageModel wrapping the specified folder.
    */
    explicit CollectionStorageModel(QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent = Q_NULLPTR);
    ~CollectionStorageModel();
    bool isOutBoundFolder(const Akonadi::Collection &c) const Q_DECL_OVERRIDE;
};

#endif /* COLLECTIONPANE_H */

