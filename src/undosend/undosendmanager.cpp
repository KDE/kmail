/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "undosendmanager.h"
#include "kmail_debug.h"
#include "undosendcreatejob.h"

UndoSendManager::UndoSendManager(QObject *parent)
    : QObject(parent)
{
}

UndoSendManager::~UndoSendManager() = default;

UndoSendManager *UndoSendManager::self()
{
    static UndoSendManager s_self;
    return &s_self;
}

void UndoSendManager::addItem(qint64 index, const QString &subject, int delay)
{
    auto job = new UndoSendCreateJob(this);
    job->setAkonadiIndex(index);
    job->setSubject(subject);
    job->setDelay(delay);
    if (!job->start()) {
        qCWarning(KMAIL_LOG) << " Impossible to create job";
    }
}
