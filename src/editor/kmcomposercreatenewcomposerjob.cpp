/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kmcomposercreatenewcomposerjob.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"
#include "kmkernel.h"

#include <MessageComposer/MessageHelper>
#include <TemplateParser/TemplateParserJob>

KMComposerCreateNewComposerJob::KMComposerCreateNewComposerJob(QObject *parent)
    : QObject(parent)
{
}

KMComposerCreateNewComposerJob::~KMComposerCreateNewComposerJob() = default;

void KMComposerCreateNewComposerJob::start()
{
    mMsg = KMime::Message::Ptr(new KMime::Message());

    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager(), mCurrentIdentity);
    auto parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
    connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &KMComposerCreateNewComposerJob::slotCreateNewComposer);
    parser->setIdentityManager(KMKernel::self()->identityManager());
    parser->process(mMsg, mCollectionForNewMessage.id());
}

void KMComposerCreateNewComposerJob::slotCreateNewComposer(bool forceCursorPosition)
{
    KMail::Composer *win = KMComposerWin::create(mMsg, false, false, KMail::Composer::New, mCurrentIdentity);
    win->setCollectionForNewMessage(mCollectionForNewMessage);
    if (forceCursorPosition) {
        win->setFocusToEditor();
    }
    win->show();
    deleteLater();
}

void KMComposerCreateNewComposerJob::setCurrentIdentity(uint currentIdentity)
{
    mCurrentIdentity = currentIdentity;
}

void KMComposerCreateNewComposerJob::setCollectionForNewMessage(const Akonadi::Collection &collectionForNewMessage)
{
    mCollectionForNewMessage = collectionForNewMessage;
}
