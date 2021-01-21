/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
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
    ~ComposeNewMessageJob() override;
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
