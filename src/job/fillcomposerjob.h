/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KMime/Message>
#include <QObject>

struct FillComposerJobSettings {
    FillComposerJobSettings() = default;

    FillComposerJobSettings(bool hidden,
                            const QString &to,
                            const QString &cc,
                            const QString &bcc,
                            const QString &subject,
                            const QString &body,
                            const QString &attachName,
                            const QByteArray &attachCte,
                            const QByteArray &attachData,
                            const QByteArray &attachType,
                            const QByteArray &attachSubType,
                            const QByteArray &attachParamAttr,
                            const QString &attachParamValue,
                            const QByteArray &attachContDisp,
                            const QByteArray &attachCharset,
                            unsigned int identity,
                            bool forceShowWindow)
        : mTo(to)
        , mCc(cc)
        , mBcc(bcc)
        , mSubject(subject)
        , mBody(body)
        , mAttachName(attachName)
        , mAttachCte(attachCte)
        , mAttachData(attachData)
        , mAttachType(attachType)
        , mAttachSubType(attachSubType)
        , mAttachParamAttr(attachParamAttr)
        , mAttachParamValue(attachParamValue)
        , mAttachContDisp(attachContDisp)
        , mAttachCharset(attachCharset)
        , mIdentity(identity)
        , mForceShowWindow(forceShowWindow)
        , mHidden(hidden)
    {
    }

    QString mTo;
    QString mCc;
    QString mBcc;
    QString mSubject;
    QString mBody;
    QString mAttachName;
    QByteArray mAttachCte;
    QByteArray mAttachData;
    QByteArray mAttachType;
    QByteArray mAttachSubType;
    QByteArray mAttachParamAttr;
    QString mAttachParamValue;
    QByteArray mAttachContDisp;
    QByteArray mAttachCharset;
    unsigned int mIdentity = 0;
    bool mForceShowWindow = false;
    bool mHidden = false;
};

class FillComposerJob : public QObject
{
    Q_OBJECT
public:
    explicit FillComposerJob(QObject *parent = nullptr);
    ~FillComposerJob();
    void start();
    void setSettings(const FillComposerJobSettings &settings);

private:
    Q_DISABLE_COPY(FillComposerJob)
    void slotOpenComposer();
    FillComposerJobSettings mSettings;
    KMime::Message::Ptr mMsg = nullptr;
};

