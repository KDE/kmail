/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmundoredomanager.h"

using namespace KMail;
KMUndoRedoManager::KMUndoRedoManager(QObject *parent)
    : QObject{parent}
{
}

KMUndoRedoManager::~KMUndoRedoManager() = default;
