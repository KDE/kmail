/*
   Copyright (C) 2013-2020 Laurent Montel <montel@kde.org>

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

#ifndef SENDLATERJOB_H
#define SENDLATERJOB_H

#include <QObject>

#include "sendlatermanager.h"

#include <KMime/Message>
#include <ItemFetchScope>
#include <Item>

namespace SendLater {
class SendLaterInfo;
}
class KJob;
class SendLaterJob : public QObject
{
    Q_OBJECT
public:
    explicit SendLaterJob(SendLaterManager *manager, SendLater::SendLaterInfo *info, QObject *parent = nullptr);
    ~SendLaterJob();
    void start();

private:
    Q_DISABLE_COPY(SendLaterJob)
    void sendDone();
    void sendError(const QString &error, SendLaterManager::ErrorType type);
    void slotMessageTransfered(const Akonadi::Item::List &);
    void slotJobFinished(KJob *);
    void slotDeleteItem(KJob *);
    void updateAndCleanMessageBeforeSending(const KMime::Message::Ptr &msg);
    Akonadi::ItemFetchScope mFetchScope;
    SendLaterManager *mManager = nullptr;
    SendLater::SendLaterInfo *mInfo = nullptr;
    Akonadi::Item mItem;
};

#endif // SENDLATERJOB_H
