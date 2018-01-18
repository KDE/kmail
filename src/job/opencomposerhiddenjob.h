/*
   Copyright (C) 2017-2018 Laurent Montel <montel@kde.org>

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

#ifndef OPENCOMPOSERHIDDENJOB_H
#define OPENCOMPOSERHIDDENJOB_H

#include <QObject>
#include <KMime/Message>

struct OpenComposerHiddenJobSettings
{
    OpenComposerHiddenJobSettings()
    {
    }

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
    ~OpenComposerHiddenJob();
    void start();
    void setSettings(const OpenComposerHiddenJobSettings &settings);

private:
    Q_DISABLE_COPY(OpenComposerHiddenJob)
    void slotOpenComposer();
    OpenComposerHiddenJobSettings mSettings;
    KMime::Message::Ptr mMsg;
};

#endif // OPENCOMPOSERHIDDENJOB_H
