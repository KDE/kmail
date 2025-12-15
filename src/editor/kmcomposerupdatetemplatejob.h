/*
   SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Collection>
#include <KIdentityManagementCore/Identity>
#include <KMime/Message>
#include <QObject>

class KMComposerUpdateTemplateJob : public QObject
{
    Q_OBJECT
public:
    explicit KMComposerUpdateTemplateJob(QObject *parent = nullptr);
    ~KMComposerUpdateTemplateJob() override;
    void start();
    void setMsg(const QSharedPointer<KMime::Message> &msg);

    void setCustomTemplate(const QString &customTemplate);

    void setTextSelection(const QString &textSelection);

    void setWasModified(bool wasModified);

    void setUoldId(uint uoldId);

    void setUoid(uint uoid);

    void setIdent(const KIdentityManagementCore::Identity &ident);

    void setCollection(const Akonadi::Collection &col);
Q_SIGNALS:
    void updateComposer(const KIdentityManagementCore::Identity &ident, const QSharedPointer<KMime::Message> &msg, uint uoid, uint uoldId, bool wasModified);

private:
    void slotFinished();

    QString mTextSelection;
    QString mCustomTemplate;
    QSharedPointer<KMime::Message> mMsg;
    Akonadi::Collection mCollectionForNewMessage;
    uint mUoldId = 0;
    uint mUoid = 0;
    KIdentityManagementCore::Identity mIdent;
    bool mWasModified = false;
};
