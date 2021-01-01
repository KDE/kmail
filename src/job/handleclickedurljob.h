/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef HANDLECLICKEDURLJOB_H
#define HANDLECLICKEDURLJOB_H

#include <QObject>
#include <QSharedPointer>
#include <MailCommon/FolderSettings>
#include "kmail_private_export.h"

class KMAILTESTS_TESTS_EXPORT HandleClickedUrlJob : public QObject
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
    Q_DISABLE_COPY(HandleClickedUrlJob)
    void slotOpenComposer();
    QUrl mUrl;
    Akonadi::Collection mCurrentCollection;
    QSharedPointer<MailCommon::FolderSettings> mFolder;
    uint mIdentity = 0;
    KMime::Message::Ptr mMsg = nullptr;
};

#endif // HANDLECLICKEDURLJOB_H
