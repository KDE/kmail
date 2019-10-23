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


#include "undosendcreatejob.h"
#include "kmail_debug.h"

UndoSendCreateJob::UndoSendCreateJob(QObject *parent)
    : QObject(parent)
{

}

UndoSendCreateJob::~UndoSendCreateJob()
{

}

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
    //TODO
    return true;
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

void UndoSendCreateJob::setAkonadiIndex(const qint64 &akonadiIndex)
{
    mAkonadiIndex = akonadiIndex;
}
