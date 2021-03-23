/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KMime/Message>
#include <QObject>

struct OpenComposerHiddenJobSettings {
    OpenComposerHiddenJobSettings() = default;

    OpenComposerHiddenJobSettings(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, bool hidden)
        : mTo(to)
        , mCc(cc)
        , mBcc(bcc)
        , mSubject(subject)
        , mBody(body)
        , mHidden(hidden)
    {
    }

    QString mTo;
    QString mCc;
    QString mBcc;
    QString mSubject;
    QString mBody;
    bool mHidden = false;
};

class OpenComposerHiddenJob : public QObject
{
    Q_OBJECT
public:
    explicit OpenComposerHiddenJob(QObject *parent = nullptr);
    ~OpenComposerHiddenJob() override;
    void start();
    void setSettings(const OpenComposerHiddenJobSettings &settings);

private:
    Q_DISABLE_COPY(OpenComposerHiddenJob)
    void slotOpenComposer();
    OpenComposerHiddenJobSettings mSettings;
    KMime::Message::Ptr mMsg = nullptr;
};

