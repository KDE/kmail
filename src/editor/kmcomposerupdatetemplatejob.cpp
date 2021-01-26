/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kmcomposerupdatetemplatejob.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"
#include "kmkernel.h"

#include <MessageComposer/MessageHelper>
#include <TemplateParser/TemplateParserJob>

KMComposerUpdateTemplateJob::KMComposerUpdateTemplateJob(QObject *parent)
    : QObject(parent)
{
}

KMComposerUpdateTemplateJob::~KMComposerUpdateTemplateJob() = default;

void KMComposerUpdateTemplateJob::start()
{
    auto parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
    connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &KMComposerUpdateTemplateJob::slotFinished);
    parser->setSelection(mTextSelection);
    parser->setAllowDecryption(true);
    parser->setIdentityManager(KMKernel::self()->identityManager());
    if (!mCustomTemplate.isEmpty()) {
        parser->process(mCustomTemplate, mMsg, mCollectionForNewMessage.id());
    } else {
        parser->processWithIdentity(mUoid, mMsg, mCollectionForNewMessage.id());
    }
}

void KMComposerUpdateTemplateJob::slotFinished()
{
    Q_EMIT updateComposer(mIdent, mMsg, mUoid, mUoldId, mWasModified);
    deleteLater();
}

void KMComposerUpdateTemplateJob::setMsg(const KMime::Message::Ptr &msg)
{
    mMsg = msg;
}

void KMComposerUpdateTemplateJob::setCustomTemplate(const QString &customTemplate)
{
    mCustomTemplate = customTemplate;
}

void KMComposerUpdateTemplateJob::setTextSelection(const QString &textSelection)
{
    mTextSelection = textSelection;
}

void KMComposerUpdateTemplateJob::setWasModified(bool wasModified)
{
    mWasModified = wasModified;
}

void KMComposerUpdateTemplateJob::setUoldId(uint uoldId)
{
    mUoldId = uoldId;
}

void KMComposerUpdateTemplateJob::setUoid(uint uoid)
{
    mUoid = uoid;
}

void KMComposerUpdateTemplateJob::setIdent(const KIdentityManagement::Identity &ident)
{
    mIdent = ident;
}

void KMComposerUpdateTemplateJob::setCollection(const Akonadi::Collection &col)
{
    mCollectionForNewMessage = col;
}
