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

#include "kmcomposercreatenewcomposerjob.h"
#include "kmkernel.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"

#include <KMime/Message>
#include <MessageComposer/MessageHelper>
#include <TemplateParser/TemplateParserJob>

KMComposerCreateNewComposerJob::KMComposerCreateNewComposerJob(QObject *parent)
    : QObject(parent)
{
}

KMComposerCreateNewComposerJob::~KMComposerCreateNewComposerJob()
{
}

void KMComposerCreateNewComposerJob::start()
{
    mMsg = KMime::Message::Ptr(new KMime::Message());

    MessageHelper::initHeader(mMsg, KMKernel::self()->identityManager(), mCurrentIdentity);
    TemplateParser::TemplateParserJob *parser = new TemplateParser::TemplateParserJob(mMsg, TemplateParser::TemplateParserJob::NewMessage, this);
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
