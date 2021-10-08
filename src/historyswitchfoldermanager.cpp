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
    mUndoStack->push(new HistorySwitchFolderCommand(currentCol, col));
}

void HistorySwitchFolderManager::clear()
{
    mUndoStack->clear();
}

HistorySwitchFolderCommand::HistorySwitchFolderCommand(const Akonadi::Collection &currentCol, const Akonadi::Collection &col)
    : mCurrentCollection(currentCol)
    , mNewCollection(col)
{
}

void HistorySwitchFolderCommand::undo()
{
}

void HistorySwitchFolderCommand::redo()
{
}
