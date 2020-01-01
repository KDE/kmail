/*
   Copyright (C) 2017-2020 Laurent Montel <montel@kde.org>

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

#include "kmcomposerupdatetemplatejob.h"
#include "kmkernel.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"

#include <KMime/Message>
#include <MessageComposer/MessageHelper>
#include <TemplateParser/TemplateParserJob>

KMComposerUpdateTemplateJob::KMComposerUpdateTemplateJob(QObject *parent)
    : QObject(parent)
{
}

KMComposerUpdateTemplateJob::~KMComposerUpdateTemplateJob()
{
}

void KMComposerUpdateTemplateJob::start()
{
    TemplateParser::TemplateParserJob *parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
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
