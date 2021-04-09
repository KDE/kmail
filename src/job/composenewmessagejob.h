/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <MailCommon/FolderSettings>
#include <QObject>
#include <QSharedPointer>

class ComposeNewMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit ComposeNewMessageJob(QObject *parent = nullptr);
    ~ComposeNewMessageJob() override;

    void start();
    void setFolderSettings(const QSharedPointer<MailCommon::FolderSettings> &folder);

    void setCurrentCollection(const Akonadi::Collection &col);
    void setRecipientsFromMessage(const Akonadi::Item &from);

private:
    Q_DISABLE_COPY(ComposeNewMessageJob)
    void slotOpenComposer(bool forceCursorPosition);
    QSharedPointer<MailCommon::FolderSettings> mFolder;
    Akonadi::Collection mCurrentCollection;
    Akonadi::Item mRecipientsFrom;
    uint mIdentity = 0;
    KMime::Message::Ptr mMsg = nullptr;
};

