/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "historyswitchfoldermanager.h"

#include <QUndoStack>

HistorySwitchFolderManager::HistorySwitchFolderManager(QObject *parent)
    : QObject{parent}
    , mUndoStack(new QUndoStack(this))
{
}

HistorySwitchFolderManager::~HistorySwitchFolderManager()
{
}

void HistorySwitchFolderManager::addHistory(const Akonadi::Collection &currentCol, const Akonadi::Collection &col)
{
    mUndoStack->push(new HistorySwitchFolderCommand(this, currentCol, col));
}

void HistorySwitchFolderManager::clear()
{
    mUndoStack->clear();
}

void HistorySwitchFolderManager::changeCollection(const Akonadi::Collection &currentCol)
{
    Q_EMIT switchToFolder(currentCol);
}

void HistorySwitchFolderManager::undo()
{
    mUndoStack->undo();
}

void HistorySwitchFolderManager::redo()
{
    mUndoStack->redo();
}

HistorySwitchFolderCommand::HistorySwitchFolderCommand(HistorySwitchFolderManager *manager,
                                                       const Akonadi::Collection &currentCol,
                                                       const Akonadi::Collection &col)
    : mCurrentCollection(currentCol)
    , mNewCollection(col)
    , mManager(manager)
{
}

void HistorySwitchFolderCommand::undo()
{
    mManager->changeCollection(mCurrentCollection);
}

void HistorySwitchFolderCommand::redo()
{
    mManager->changeCollection(mNewCollection);
}
