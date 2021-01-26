/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "undosendcreatejob.h"
#include "kmail_debug.h"

#include <MessageComposer/SendLaterRemoveJob>

#include <KLocalizedString>
#include <KNotification>
#include <QTimer>

UndoSendCreateJob::UndoSendCreateJob(QObject *parent)
    : QObject(parent)
{
}

UndoSendCreateJob::~UndoSendCreateJob() = default;

bool UndoSendCreateJob::canStart() const
{
    if (mAkonadiIndex < 0 || mDelay <= 0) {
        return false;
    }
    return true;
}

bool UndoSendCreateJob::start()
{
    if (!canStart()) {
        qCWarning(KMAIL_LOG) << "Impossible to start undosendcreatejob";
        deleteLater();
        return false;
    }
    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, &UndoSendCreateJob::slotTimeOut);
    mTimer->setSingleShot(true);
    mTimer->start(mDelay * 1000);
    mNotification = new KNotification(QStringLiteral("undosend"), KNotification::Persistent);
    mNotification->setText(mSubject);
    mNotification->setActions(QStringList() << i18n("Undo send"));

    connect(mNotification, QOverload<unsigned int>::of(&KNotification::activated), this, &UndoSendCreateJob::slotActivateNotificationAction);
    connect(mNotification, &KNotification::closed, this, &UndoSendCreateJob::slotNotificationClosed);
    mNotification->sendEvent();

    return true;
}

void UndoSendCreateJob::slotTimeOut()
{
    mNotification->close();
    deleteLater();
}

void UndoSendCreateJob::slotNotificationClosed()
{
    mTimer->stop();
    deleteLater();
}

void UndoSendCreateJob::slotActivateNotificationAction(unsigned int index)
{
    // Index == 0 => is the default action. We don't have it.
    switch (index) {
    case 1:
        undoSendEmail();
        return;
    }
    qCWarning(KMAIL_LOG) << " SpecialNotifierJob::slotActivateNotificationAction unknown index " << index;
}

void UndoSendCreateJob::undoSendEmail()
{
    mTimer->stop();
    auto job = new MessageComposer::SendLaterRemoveJob(mAkonadiIndex, this);
    job->start();
}

QString UndoSendCreateJob::subject() const
{
    return mSubject;
}

void UndoSendCreateJob::setSubject(const QString &subject)
{
    mSubject = subject;
}

int UndoSendCreateJob::delay() const
{
    return mDelay;
}

void UndoSendCreateJob::setDelay(int delay)
{
    mDelay = delay;
}

qint64 UndoSendCreateJob::akonadiIndex() const
{
    return mAkonadiIndex;
}

void UndoSendCreateJob::setAkonadiIndex(qint64 akonadiIndex)
{
    mAkonadiIndex = akonadiIndex;
}
