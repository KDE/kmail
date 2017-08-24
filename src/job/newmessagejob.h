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

    NewMessageJobSettings(const QString &to, const QString &cc, const QString &bcc, bool hidden, const QString &attachURL, const QSharedPointer<MailCommon::FolderSettings> &folder, uint identity,
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
    ~NewMessageJob();
    void start();

    void setNewMessageJobSettings(const NewMessageJobSettings &newMessageJobSettings);

private:
    void slotOpenComposer();
    NewMessageJobSettings mNewMessageJobSettings;
    Akonadi::Collection mCollection;
    QUrl mAttachURL;
    KMime::Message::Ptr mMsg;
};

#endif // NEWMESSAGEJOB_H
