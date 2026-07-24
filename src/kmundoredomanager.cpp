/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmundoredomanager.h"
#include <QUndoStack>
using namespace KMail;
KMUndoRedoManager::KMUndoRedoManager(QObject *parent)
    : QObject{parent}
    , mUndoStack(new QUndoStack(this))
{
}

KMUndoRedoManager::~KMUndoRedoManager() = default;

void KMUndoInfoMoveItems::undo()
{
    // TODO
}

void KMUndoInfoMoveItems::redo()
{
    // TODO
}
