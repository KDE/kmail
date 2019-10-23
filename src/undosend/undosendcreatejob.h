/*
   Copyright (C) 2019 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef UNDOSENDCREATEJOB_H
#define UNDOSENDCREATEJOB_H

#include <QObject>
#include "kmail_private_export.h"
class KNotification;
class QTimer;
class KMAILTESTS_TESTS_EXPORT UndoSendCreateJob : public QObject
{
    Q_OBJECT
public:
    explicit UndoSendCreateJob(QObject *parent = nullptr);
    ~UndoSendCreateJob();
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

#endif // UNDOSENDCREATEJOB_H
