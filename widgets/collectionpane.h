/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2010 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef COLLECTIONPANE_H
#define COLLECTIONPANE_H

#include <messagelist/pane.h>
#include <messagelist/storagemodel.h>

class CollectionPane : public MessageList::Pane
{
    Q_OBJECT
public:
    explicit CollectionPane(bool restoreSession, QAbstractItemModel *model, QItemSelectionModel *selectionModel, QWidget *parent = 0);
    ~CollectionPane();

    MessageList::StorageModel *createStorageModel(QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent);
    void writeConfig(bool restoreSession);
};

class CollectionStorageModel : public MessageList::StorageModel
{
    Q_OBJECT
public:
    /**
    * Create a StorageModel wrapping the specified folder.
    */
    explicit CollectionStorageModel(QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent = 0);
    ~CollectionStorageModel();
    bool isOutBoundFolder(const Akonadi::Collection &c) const;
};

#endif /* COLLECTIONPANE_H */

