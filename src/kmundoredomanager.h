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

    [[nodiscard]] int newUndoMoveAction(const Akonadi::Collection &srcFolder, const Akonadi::Collection &destFolder);
    void addMsgToMoveAction(int undoId, const Akonadi::Item &item);

private:
    QUndoStack *const mUndoStack;
};

class KMUndoInfoMoveItems : public QUndoCommand
{
public:
    void undo() override;
    void redo() override;

    [[nodiscard]] Akonadi::Item::List items() const;
    void setItems(const Akonadi::Item::List &newItems);

    [[nodiscard]] Akonadi::Collection srcFolder() const;
    void setSrcFolder(const Akonadi::Collection &newSrcFolder);

    [[nodiscard]] Akonadi::Collection destFolder() const;
    void setDestFolder(const Akonadi::Collection &newDestFolder);

    [[nodiscard]] bool moveToTrash() const;
    void setMoveToTrash(bool newMoveToTrash);

private:
    Akonadi::Item::List mItems;
    Akonadi::Collection mSrcFolder;
    Akonadi::Collection mDestFolder;
    bool mMoveToTrash = false;
};
}
