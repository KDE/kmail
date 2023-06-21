/*
   SPDX-FileCopyrightText: 2021-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailmergemanager.h"

MailMergeManager::MailMergeManager(QObject *parent)
    : QObject(parent)
{
}

MailMergeManager::~MailMergeManager() = default;

QString MailMergeManager::printDebugInfo() const
{
    // TODO
    return {};
}

void MailMergeManager::load(bool state)
{
    // Load list of element
}

void MailMergeManager::stopAll()
{
    // TODO stop agent
}

bool MailMergeManager::itemRemoved(Akonadi::Item::Id id)
{
    // TODO remove id.
    // TODO
    //    if (mConfig->hasGroup(SendLaterUtil::sendLaterPattern().arg(id))) {
    //        removeInfo(id);
    //        mConfig->reparseConfiguration();
    //        Q_EMIT needUpdateConfigDialogBox();
    //        return true;
    //    }
    return false;
}

#include "moc_mailmergemanager.cpp"
