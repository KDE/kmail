/*
   SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "undosendmanager.h"
#include "undosendcreatejob.h"
#include "kmail_debug.h"

UndoSendManager::UndoSendManager(QObject *parent)
    : QObject(parent)
{
}

UndoSendManager::~UndoSendManager()
{
}

UndoSendManager *UndoSendManager::self()
{
    static UndoSendManager s_self;
    return &s_self;
}

void UndoSendManager::addItem(qint64 index, const QString &subject, int delay)
{
    UndoSendCreateJob *job = new UndoSendCreateJob(this);
    job->setAkonadiIndex(index);
    job->setSubject(subject);
    job->setDelay(delay);
    if (!job->start()) {
        qCWarning(KMAIL_LOG) << " Impossible to create job";
    }
}
