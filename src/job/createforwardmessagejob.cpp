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

#include "createforwardmessagejob.h"
#include "kmkernel.h"
#include "../util.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"
#include <KMime/Message>
#include <KEmailAddress>
#include <MailCommon/MailUtil>

CreateForwardMessageJob::CreateForwardMessageJob(QObject *parent)
    : QObject(parent)
{
}

CreateForwardMessageJob::~CreateForwardMessageJob()
{
    delete mMessageFactory;
}

void CreateForwardMessageJob::setSettings(const CreateForwardMessageJobSettings &value)
{
    mSettings = value;
}

void CreateForwardMessageJob::start()
{
    const auto col = CommonKernel->collectionFromId(mSettings.item.parentCollection().id());
    mMessageFactory = new MessageComposer::MessageFactoryNG(mSettings.msg, mSettings.item.id(),
                                                            col);
    connect(mMessageFactory, &MessageComposer::MessageFactoryNG::createForwardDone, this, &CreateForwardMessageJob::slotCreateForwardDone);
    mMessageFactory->setIdentityManager(KMKernel::self()->identityManager());
    mMessageFactory->setFolderIdentity(MailCommon::Util::folderIdentity(mSettings.item));
    mMessageFactory->setSelection(mSettings.selection);
    mMessageFactory->setTemplate(mSettings.templateStr);
    mMessageFactory->createForwardAsync();
}

void CreateForwardMessageJob::slotCreateForwardDone(const KMime::Message::Ptr &fmsg)
{
    if (mSettings.url.isValid()) {
        fmsg->to()->fromUnicodeString(KEmailAddress::decodeMailtoUrl(mSettings.url).toLower(), "utf-8");
    }
    bool lastEncrypt = false;
    bool lastSign = false;
    KMail::Util::lastEncryptAndSignState(lastEncrypt, lastSign, mSettings.msg);

    if (mSettings.url.isValid()) {
        KMail::Composer *win = KMail::makeComposer(fmsg, lastSign, lastEncrypt, KMail::Composer::Forward);
        win->show();
    } else {
        uint id = 0;
        if (auto hrd = mSettings.msg->headerByType("X-KMail-Identity")) {
            id = hrd->asUnicodeString().trimmed().toUInt();
        }
        if (id == 0) {
            id = mSettings.identity;
        }
        KMail::Composer *win = KMail::makeComposer(fmsg, lastSign, lastEncrypt, KMail::Composer::Forward, id, QString(), mSettings.templateStr);
        win->show();
    }
    deleteLater();
}
