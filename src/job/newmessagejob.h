/*
   SPDX-FileCopyrightText: 2017-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef NEWMESSAGEJOB_H
#define NEWMESSAGEJOB_H

#include <QObject>
#include <QSharedPointer>
#include <MailCommon/FolderSettings>

struct NewMessageJobSettings
{
    NewMessageJobSettings()
    {
    }

    NewMessageJobSettings(const QString &to, const QString &cc, const QString &bcc, bool hidden, const QString &attachURL, const QSharedPointer<MailCommon::FolderSettings> &folder, uint identity, const Akonadi::Collection &currentCollection)
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
    ~NewMessageJob();
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

#endif // NEWMESSAGEJOB_H
