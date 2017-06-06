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

#include "composenewmessagejob.h"
#include "kmkernel.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"

#include <KMime/Message>
#include <MessageComposer/MessageHelper>
#include <TemplateParser/TemplateParserJob>

ComposeNewMessageJob::ComposeNewMessageJob(QObject *parent)
    : QObject(parent)
    , mIdentity(0)
    , mMsg(nullptr)
{
}

ComposeNewMessageJob::~ComposeNewMessageJob()
{
}

void ComposeNewMessageJob::setCurrentCollection(const Akonadi::Collection &col)
{
    mCurrentCollection = col;
}

void ComposeNewMessageJob::start()
{
    mMsg = KMime::Message::Ptr(new KMime::Message());

    mIdentity = mFolder ? mFolder->identity() : 0;
    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager(), mIdentity);
    TemplateParser::TemplateParserJob *parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage);
    connect(parser, &TemplateParser::TemplateParserJob::parsingDone, this, &ComposeNewMessageJob::slotOpenComposer);
    parser->setIdentityManager(KMKernel::self()->identityManager());
    if (mFolder) {
        parser->process(mMsg, mCurrentCollection);
    } else {
        parser->process(KMime::Message::Ptr(), Akonadi::Collection());
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
