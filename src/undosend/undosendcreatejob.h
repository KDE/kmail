/*
   SPDX-FileCopyrightText: 2019-2024 Laurent Montel <montel@kde.org>

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
    [[nodiscard]] bool canStart() const;
    [[nodiscard]] bool start();

    [[nodiscard]] QString subject() const;
    void setMessageInfoText(const QString &subject);

    [[nodiscard]] int delay() const;
    void setDelay(int delay);

    [[nodiscard]] qint64 akonadiIndex() const;
    void setAkonadiIndex(qint64 akonadiIndex);

private:
    KMAIL_NO_EXPORT void slotNotificationClosed();
    KMAIL_NO_EXPORT void undoSendEmail();
    KMAIL_NO_EXPORT void slotTimeOut();
    QString mSubject;
    KNotification *mNotification = nullptr;
    QTimer *mTimer = nullptr;
    qint64 mAkonadiIndex = -1;
    int mDelay = -1;
};
