/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailmergemanager.h"

MailMergeManager::MailMergeManager(QObject *parent)
    : QObject(parent)
{
}

MailMergeManager::~MailMergeManager()
{
}

QString MailMergeManager::printDebugInfo() const
{
    // TODO
    return {};
}
