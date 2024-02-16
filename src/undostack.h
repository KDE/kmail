/*
    This file is part of KMail

    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 2003 Zack Rusin <zack@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include "kmail_private_export.h"
#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <QList>
#include <QObject>

class KJob;

namespace KMail
{
/** A class for storing Undo information. */
class UndoInfo
{
public:
    UndoInfo() = default;

    int id = -1;
    Akonadi::Item::List items;
    Akonadi::Collection srcFolder;
    Akonadi::Collection destFolder;
    bool moveToTrash = false;
};

class KMAILTESTS_TESTS_EXPORT UndoStack : public QObject
{
    Q_OBJECT

public:
    explicit UndoStack(int size);
    ~UndoStack() override;

    void clear();
    [[nodiscard]] int size() const;
    [[nodiscard]] int newUndoAction(const Akonadi::Collection &srcFolder, const Akonadi::Collection &destFolder);
    void addMsgToAction(int undoId, const Akonadi::Item &item);
    [[nodiscard]] bool isEmpty() const;
    void undo();

    void pushSingleAction(const Akonadi::Item &item, const Akonadi::Collection &, const Akonadi::Collection &destFolder);
    void folderDestroyed(const Akonadi::Collection &folder);

    [[nodiscard]] QString undoInfo() const;

Q_SIGNALS:
    void undoStackChanged();

private:
    KMAIL_NO_EXPORT void slotMoveResult(KJob *);
    QList<UndoInfo *> mStack;
    const int mSize = 0;
    int mLastId = 0;
    UndoInfo *mCachedInfo = nullptr;
};
}
