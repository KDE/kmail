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

#ifndef KMCOMPOSERUPDATETEMPLATEJOB_H
#define KMCOMPOSERUPDATETEMPLATEJOB_H

#include <QObject>
#include <KMime/Message>
#include <AkonadiCore/Collection>
#include <KIdentityManagement/Identity>

class KMComposerUpdateTemplateJob : public QObject
{
    Q_OBJECT
public:
    explicit KMComposerUpdateTemplateJob(QObject *parent = nullptr);
    ~KMComposerUpdateTemplateJob();
    void start();
    void setMsg(const KMime::Message::Ptr &msg);

    void setCustomTemplate(const QString &customTemplate);

    void setTextSelection(const QString &textSelection);

    void setWasModified(bool wasModified);

    void setUoldId(uint uoldId);

    void setUoid(uint uoid);

    void setIdent(const KIdentityManagement::Identity &ident);

Q_SIGNALS:
    void updateComposer(const KIdentityManagement::Identity &ident, const KMime::Message::Ptr &msg, uint uoid, uint uoldId, bool wasModified);

private:
    void slotFinished();

    QString mTextSelection;
    QString mCustomTemplate;
    KMime::Message::Ptr mMsg;
    Akonadi::Collection mCollectionForNewMessage;
    uint mUoldId;
    uint mUoid;
    KIdentityManagement::Identity mIdent;
    bool mWasModified;
};

#endif // KMCOMPOSERUPDATETEMPLATEJOB_H
