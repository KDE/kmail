/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <QObject>
class KNotification;
class QTimer;
class KMAILTESTS_TESTS_EXPORT UndoSendCreateJob : public QObject
{
    Q_OBJECT
public:
    explicit UndoSendCreateJob(QObject *parent = nullptr);
    ~UndoSendCreateJob() override;
    Q_REQUIRED_RESULT bool canStart() const;
    Q_REQUIRED_RESULT bool start();

    Q_REQUIRED_RESULT QString subject() const;
    void setSubject(const QString &subject);

    Q_REQUIRED_RESULT int delay() const;
    void setDelay(int delay);

    Q_REQUIRED_RESULT qint64 akonadiIndex() const;
    void setAkonadiIndex(qint64 akonadiIndex);

private:
    void slotActivateNotificationAction(unsigned int index);
    void slotNotificationClosed();
    void undoSendEmail();
    void slotTimeOut();
    QString mSubject;
    KNotification *mNotification = nullptr;
    QTimer *mTimer = nullptr;
    qint64 mAkonadiIndex = -1;
    int mDelay = -1;
};

