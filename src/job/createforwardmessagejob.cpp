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


#include "createforwardmessagejob.h"
#include "config-kmail.h"
#include "kmkernel.h"
#include "../util.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"
#include <KMime/Message>
#include <KEmailAddress>
#include <MailCommon/MailUtil>
#include <MessageComposer/MessageFactory>
#include <QUrl>

CreateForwardMessageJob::CreateForwardMessageJob(QObject *parent)
    : QObject(parent)
{

}

CreateForwardMessageJob::~CreateForwardMessageJob()
{

}

void CreateForwardMessageJob::setSettings(const CreateForwardMessageJobSettings &value)
{
    mSettings = value;
}

void CreateForwardMessageJob::start()
{
    MessageComposer::MessageFactory factory(mSettings.mMsg, mSettings.mItem.id(), MailCommon::Util::updatedCollection(mSettings.mItem.parentCollection()));
    factory.setIdentityManager(KMKernel::self()->identityManager());
    factory.setFolderIdentity(MailCommon::Util::folderIdentity(mSettings.mItem));
    KMime::Message::Ptr fmsg = factory.createForward();
    fmsg->to()->fromUnicodeString(KEmailAddress::decodeMailtoUrl(mSettings.mUrl).toLower(), "utf-8");
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, mSettings.mMsg);

    KMail::Composer *win = KMail::makeComposer(fmsg, lastSign, lastEncrypt, KMail::Composer::Forward);
    win->show();
    deleteLater();
}

