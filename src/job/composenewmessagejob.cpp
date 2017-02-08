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
#include "config-kmail.h"
#include "kmkernel.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"

#include <KMime/Message>
#include <MessageComposer/MessageHelper>
#include <TemplateParser/TemplateParser>

ComposeNewMessageJob::ComposeNewMessageJob(QObject *parent)
    : QObject(parent)
{

}

ComposeNewMessageJob::~ComposeNewMessageJob()
{

}

void ComposeNewMessageJob::start()
{
    KMime::Message::Ptr msg(new KMime::Message());

    const uint identity = mFolder ? mFolder->identity() : 0;
    MessageHelper::initHeader(msg, KMKernel::self()->identityManager(), identity);
    TemplateParser::TemplateParser parser(msg, TemplateParser::TemplateParser::NewMessage);
    parser.setIdentityManager(KMKernel::self()->identityManager());
    if (mFolder) {
        parser.process(msg, mFolder->collection());
    } else {
        parser.process(KMime::Message::Ptr(), Akonadi::Collection());
    }
    KMail::Composer *win = KMail::makeComposer(msg, false, false, KMail::Composer::New, identity);
    if (mFolder) {
        win->setCollectionForNewMessage(mFolder->collection());
    }
    bool forceCursorPosition = parser.cursorPositionWasSet();
    if (forceCursorPosition) {
        win->setFocusToEditor();
    }
    win->show();
    deleteLater();
}

void ComposeNewMessageJob::setFolder(const QSharedPointer<MailCommon::FolderCollection> &folder)
{
    mFolder = folder;
}
