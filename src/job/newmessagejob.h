/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <MailCommon/FolderSettings>
#include <QObject>
#include <QSharedPointer>

struct NewMessageJobSettings {
    NewMessageJobSettings() = default;

    NewMessageJobSettings(const QString &to,
                          const QString &cc,
                          const QString &bcc,
                          bool hidden,
                          const QString &attachURL,
                          const QSharedPointer<MailCommon::FolderSettings> &folder,
                          uint identity,
                          const Akonadi::Collection &currentCollection)
        : mTo(to)
        , mCc(cc)
        , mBcc(bcc)
        , mAttachURL(attachURL)
        , mFolder(folder)
        , mCurrentCollection(currentCollection)
        , mHidden(hidden)
        , mIdentity(identity)
    {
    }

    QString mTo;
    QString mCc;
    QString mBcc;
    QString mAttachURL;
    QSharedPointer<MailCommon::FolderSettings> mFolder = nullptr;
    Akonadi::Collection mCurrentCollection;
    bool mHidden = false;
    uint mIdentity = 0;
};

class NewMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit NewMessageJob(QObject *parent = nullptr);
    ~NewMessageJob() override;
    void start();

    void setNewMessageJobSettings(const NewMessageJobSettings &newMessageJobSettings);

private:
    Q_DISABLE_COPY(NewMessageJob)
    void slotOpenComposer();
    NewMessageJobSettings mNewMessageJobSettings;
    Akonadi::Collection mCollection;
    QUrl mAttachURL;
    KMime::Message::Ptr mMsg = nullptr;
};

