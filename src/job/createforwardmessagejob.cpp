/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "createforwardmessagejob.h"
#include "../util.h"
#include "composer.h"
#include "editor/kmcomposerwin.h"
#include "kmkernel.h"
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
    mMessageFactory = new MessageComposer::MessageFactoryNG(mSettings.msg, mSettings.item.id(), col);
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
