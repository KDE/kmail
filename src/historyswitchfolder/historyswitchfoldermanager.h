/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <Akonadi/Collection>
#include <QObject>
#include <QUndoStack>
class QUndoStack;

class HistorySwitchFolderManager : public QObject
{
    Q_OBJECT
public:
    explicit HistorySwitchFolderManager(QObject *parent = nullptr);
    ~HistorySwitchFolderManager() override;
    // Add static method
    void clear();
    void addHistory(const Akonadi::Collection &currentCol, const Akonadi::Collection &col, const QString &fullPath = {});

    void changeCollection(const Akonadi::Collection &currentCol);

    void undo();
    void redo();

Q_SIGNALS:
    void switchToFolder(const Akonadi::Collection &col);

private:
    QUndoStack *const mUndoStack;
};

class HistorySwitchFolderCommand : public QUndoCommand
{
public:
    explicit HistorySwitchFolderCommand(HistorySwitchFolderManager *manager,
                                        const Akonadi::Collection &currentCol,
                                        const Akonadi::Collection &col,
                                        const QString &realPath);

    void undo() override;
    void redo() override;

private:
    const Akonadi::Collection mCurrentCollection;
    const Akonadi::Collection mNewCollection;
    QString mFullPath;
    HistorySwitchFolderManager *const mManager;
};
