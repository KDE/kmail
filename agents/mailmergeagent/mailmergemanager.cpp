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

void MailMergeManager::load(bool state) {}

void MailMergeManager::stopAll() {}

bool MailMergeManager::itemRemoved(Akonadi::Item::Id id)
{
    // TODO
    //    if (mConfig->hasGroup(SendLaterUtil::sendLaterPattern().arg(id))) {
    //        removeInfo(id);
    //        mConfig->reparseConfiguration();
    //        Q_EMIT needUpdateConfigDialogBox();
    //        return true;
    //    }
    return false;
}
