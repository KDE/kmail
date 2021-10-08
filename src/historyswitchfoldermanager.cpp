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

void HistorySwitchFolderManager::addHistory()
{
    // TODO mUndoStack->push(new QUndoCommand(this));
}

void HistorySwitchFolderManager::clear()
{
    mUndoStack->clear();
}

void HistorySwitchFolderManager::addCollection(const Akonadi::Collection &col)
{
    // TODO
}
