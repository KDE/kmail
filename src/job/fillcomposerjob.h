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

#ifndef FILLCOMPOSERJOB_H
#define FILLCOMPOSERJOB_H

#include <QObject>
#include <KMime/Message>

struct FillComposerJobSettings
{
    FillComposerJobSettings()
        : mIdentity(0)
        , mForceShowWindow(false)
        , mHidden(false)
    {
    }

    FillComposerJobSettings(bool hidden, const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body, const QString &attachName, const QByteArray &attachCte,
                            const QByteArray &attachData, const QByteArray &attachType, const QByteArray &attachSubType, const QByteArray &attachParamAttr, const QString &attachParamValue,
                            const QByteArray &attachContDisp, const QByteArray &attachCharset, unsigned int identity, bool forceShowWindow)
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
    unsigned int mIdentity;
    bool mForceShowWindow;
    bool mHidden;
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
    void slotOpenComposer();
    FillComposerJobSettings mSettings;
    KMime::Message::Ptr mMsg;
};

#endif // FILLCOMPOSERJOB_H
