/*
   Copyright (C) 2017 Laurent Montel <montel@kde.org>

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

#ifndef HANDLECLICKEDURLJOB_H
#define HANDLECLICKEDURLJOB_H

#include <QObject>
#include <QSharedPointer>
#include <MailCommon/FolderSettings>

class HandleClickedUrlJob : public QObject
{
    Q_OBJECT
public:
    explicit HandleClickedUrlJob(QObject *parent = nullptr);
    ~HandleClickedUrlJob();

    void start();

    void setUrl(const QUrl &url);
    void setFolder(const QSharedPointer<MailCommon::FolderSettings> &folder);
    void setCurrentCollection(const Akonadi::Collection &currentCollection);
private:
    void slotOpenComposer();
    QUrl mUrl;
    Akonadi::Collection mCurrentCollection;
    QSharedPointer<MailCommon::FolderSettings> mFolder;
    uint mIdentity = 0;
    KMime::Message::Ptr mMsg = nullptr;
};

#endif // HANDLECLICKEDURLJOB_H
