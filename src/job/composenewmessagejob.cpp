/*
   SPDX-FileCopyrightText: 2017-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "composenewmessagejob.h"
#include "kmkernel.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"

#include <KMime/Message>
#include <MessageComposer/MessageHelper>
#include <TemplateParser/TemplateParserJob>

ComposeNewMessageJob::ComposeNewMessageJob(QObject *parent)
    : QObject(parent)
{
}

ComposeNewMessageJob::~ComposeNewMessageJob()
= default;

void ComposeNewMessageJob::setCurrentCollection(const Akonadi::Collection &col)
{
    mCurrentCollection = col;
}

void ComposeNewMessageJob::start()
{
    mMsg = KMime::Message::Ptr(new KMime::Message());

    mIdentity = mFolder ? mFolder->identity() : 0;
    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager(), mIdentity);
    auto parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
    connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &ComposeNewMessageJob::slotOpenComposer);
    parser->setIdentityManager(KMKernel::self()->identityManager());
    if (mFolder) {
        parser->process(mMsg, mCurrentCollection.id());
    } else {
        parser->process(KMime::Message::Ptr());
    }
}

void ComposeNewMessageJob::slotOpenComposer(bool forceCursorPosition)
{
    KMail::Composer *win = KMail::makeComposer(mMsg, false, false, KMail::Composer::New, mIdentity);
    win->setCollectionForNewMessage(mCurrentCollection);

    if (forceCursorPosition) {
        win->setFocusToEditor();
    }
    win->show();
    deleteLater();
}

void ComposeNewMessageJob::setFolderSettings(const QSharedPointer<MailCommon::FolderSettings> &folder)
{
    mFolder = folder;
}
