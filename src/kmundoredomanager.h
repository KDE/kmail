/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <QObject>
#include <QUndoCommand>
class QUndoStack;
namespace KMail
{
class KMUndoRedoManager : public QObject
{
    Q_OBJECT
public:
    explicit KMUndoRedoManager(QObject *parent = nullptr);
    ~KMUndoRedoManager() override;

    [[nodiscard]] QUndoCommand *newUndoMoveAction(const Akonadi::Collection &srcFolder, const Akonadi::Collection &destFolder);
    void addMsgToMoveAction(QUndoCommand *command, const Akonadi::Item &item);

    void moveItems(const Akonadi::Item::List &list, const Akonadi::Collection &collection);
    void slotMoveResult(KJob *job);

    [[nodiscard]] QUndoStack *undoStack() const;

private:
    QUndoStack *const mUndoStack;
};

class KMUndoBase : public QUndoCommand
{
public:
    explicit KMUndoBase(KMUndoRedoManager *manager, QUndoCommand *parent = nullptr);

protected:
    KMUndoRedoManager *const mManager;
};

class KMUndoInfoChangeStatusItems : public KMUndoBase
{
public:
    explicit KMUndoInfoChangeStatusItems(KMUndoRedoManager *manager, QUndoCommand *parent = nullptr);

    [[nodiscard]] Akonadi::Item::List items() const;
    void setItems(const Akonadi::Item::List &newItems);

    void undo() override;
    void redo() override;
    [[nodiscard]] QString actionText() const;

private:
    Akonadi::Item::List mItems;
};

class KMUndoInfoMoveItems : public KMUndoBase
{
public:
    explicit KMUndoInfoMoveItems(KMUndoRedoManager *manager, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;
    [[nodiscard]] QString actionText() const;

    [[nodiscard]] Akonadi::Item::List items() const;
    void setItems(const Akonadi::Item::List &newItems);
    void addItem(const Akonadi::Item &item);

    [[nodiscard]] Akonadi::Collection srcFolder() const;
    void setSrcFolder(const Akonadi::Collection &newSrcFolder);

    [[nodiscard]] Akonadi::Collection destFolder() const;
    void setDestFolder(const Akonadi::Collection &newDestFolder);

    [[nodiscard]] bool moveToTrash() const;
    void setMoveToTrash(bool newMoveToTrash);

    // QUndoStack::push() calls redo(). The move is already performed by the command
    // which created this undo action, so the first redo() must not move the items again.
    [[nodiscard]] bool skipFirstRedo() const;
    void setSkipFirstRedo(bool newSkipFirstRedo);

private:
    Akonadi::Item::List mItems;
    Akonadi::Collection mSrcFolder;
    Akonadi::Collection mDestFolder;
    bool mMoveToTrash = false;
    bool mSkipFirstRedo = true;
};
}
