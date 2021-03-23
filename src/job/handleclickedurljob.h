/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <MailCommon/FolderSettings>
#include <QObject>
#include <QSharedPointer>

class KMAILTESTS_TESTS_EXPORT HandleClickedUrlJob : public QObject
{
    Q_OBJECT
public:
    explicit HandleClickedUrlJob(QObject *parent = nullptr);
    ~HandleClickedUrlJob() override;

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

