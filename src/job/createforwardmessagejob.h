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


#ifndef CREATEFORWARDMESSAGEJOB_H
#define CREATEFORWARDMESSAGEJOB_H

#include <QObject>
#include <QUrl>
#include <KMime/Message>
#include <AkonadiCore/Item>

struct CreateForwardMessageJobSettings
{
    CreateForwardMessageJobSettings()
        : mIdentity(0)
    {

    }

    QUrl mUrl;
    Akonadi::Item mItem;
    KMime::Message::Ptr mMsg;
    QString mTemplate;
    QString mSelection;
    uint mIdentity;
};

class CreateForwardMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit CreateForwardMessageJob(QObject *parent = nullptr);
    ~CreateForwardMessageJob();
    void start();

    void setSettings(const CreateForwardMessageJobSettings &value);

private:
    CreateForwardMessageJobSettings mSettings;
};

#endif // CREATEFORWARDMESSAGEJOB_H
