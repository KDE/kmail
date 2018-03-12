/*
   Copyright (C) 2017-2018 Laurent Montel <montel@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef COMPOSENEWMESSAGEJOB_H
#define COMPOSENEWMESSAGEJOB_H

#include <QObject>
#include <QSharedPointer>
#include <MailCommon/FolderSettings>

class ComposeNewMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit ComposeNewMessageJob(QObject *parent = nullptr);
    ~ComposeNewMessageJob();
    void start();
    void setFolderSettings(const QSharedPointer<MailCommon::FolderSettings> &folder);

    void setCurrentCollection(const Akonadi::Collection &col);
private:
    Q_DISABLE_COPY(ComposeNewMessageJob)
    void slotOpenComposer(bool forceCursorPosition);
    QSharedPointer<MailCommon::FolderSettings> mFolder;
    Akonadi::Collection mCurrentCollection;
    uint mIdentity = 0;
    KMime::Message::Ptr mMsg = nullptr;
};

#endif // COMPOSENEWMESSAGEJOB_H
