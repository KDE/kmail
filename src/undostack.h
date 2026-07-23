/*
    This file is part of KMail

    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 2003 Zack Rusin <zack@kde.org>
    SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

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
class UndoInfoBase
{
public:
    UndoInfoBase() = default;
    virtual ~UndoInfoBase() { };
    virtual void undo() = 0;
    [[nodiscard]] virtual QString undoInfo() const = 0;
    int id = -1;
};

/** A class for storing Undo information. */
class UndoInfoMoveItems : public QObject, public UndoInfoBase
{
    Q_OBJECT
public:
    UndoInfoMoveItems();
    ~UndoInfoMoveItems() override;
    [[nodiscard]] QString undoInfo() const override;

    void undo() override;
    Akonadi::Item::List items;
    Akonadi::Collection srcFolder;
    Akonadi::Collection destFolder;
    bool moveToTrash = false;

private:
    void slotMoveResult(KJob *);
};

class KMAILTESTS_TESTS_EXPORT UndoStack : public QObject
{
    Q_OBJECT

public:
    explicit UndoStack(int size);
    ~UndoStack() override;

    [[nodiscard]] int newUndoMoveAction(const Akonadi::Collection &srcFolder, const Akonadi::Collection &destFolder);
    void addMsgToMoveAction(int undoId, const Akonadi::Item &item);
    [[nodiscard]] bool isEmpty() const;
    void undo();

    [[nodiscard]] QString undoInfo() const;

Q_SIGNALS:
    void undoStackChanged();

private:
    KMAIL_NO_EXPORT void clear();
    QList<UndoInfoBase *> mStack;
    const int mSize = 0;
    int mLastId = 0;
    UndoInfoBase *mCachedInfo = nullptr;
};
}
