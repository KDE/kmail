/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

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

private:
    QUndoStack *const mUndoStack;
};

class KMUndoInfoMoveItems : public QUndoCommand
{
public:
    void undo() override;
    void redo() override;
};
}
