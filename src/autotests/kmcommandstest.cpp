/*
  SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmcommandstest.h"
#include "kmcommands.h"
#include "kmkernel.h"
#include <KIdentityManagementCore/Identity>
#include <KIdentityManagementCore/IdentityManager>

#include <Akonadi/Collection>
#include <Akonadi/Item>

#include <KMime/Message>

#include <QEventLoop>
#include <QLabel>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::Literals::StringLiterals;

Akonadi::Item createItem(const KIdentityManagementCore::Identity &ident)
{
    QByteArray data
        = "From: Konqui <konqui@kde.org>\n"
          "To: Friends <friends@kde.org>\n"
          "Date: Sun, 21 Mar 1993 23:56:48 -0800 (PST)\n"
          "Subject: Sample message\n"
          "MIME-Version: 1.0\n"
          "X-KMail-Identity: "_ba + QByteArray::number(ident.uoid()) + "\n"
                                                                    "Content-type: text/plain; charset=us-ascii\n"
                                                                    "\n"
                                                                    "\n"
                                                                    "This is explicitly typed plain US-ASCII text.\n"
                                                                    "It DOES end with a linebreak.\n"
                                                                    "\n";

    std::shared_ptr<KMime::Message> msgPtr = std::shared_ptr<KMime::Message>(new KMime::Message());
    Akonadi::Item item;
    Akonadi::Collection col(0);
    msgPtr->setContent(data);
    msgPtr->parse();
    item.setPayload<std::shared_ptr<KMime::Message>>(msgPtr);
    item.setParentCollection(col);

    return item;
}

KMCommandsTest::KMCommandsTest(QObject *parent)
    : QObject(parent)
    , mKernel(new KMKernel(parent))
{
}

KMCommandsTest::~KMCommandsTest()
{
    delete mKernel;
}

void KMCommandsTest::initTestCase()
{
    const KIdentityManagementCore::Identity &def = mKernel->identityManager()->defaultIdentity();
    KIdentityManagementCore::Identity &i1 = mKernel->identityManager()->modifyIdentityForUoid(def.uoid());
    i1.setIdentityName(u"default"_s);
    mKernel->identityManager()->newFromScratch(u"test2"_s);
    mKernel->identityManager()->newFromScratch(u"test3"_s);
    mKernel->identityManager()->commit();
}

void KMCommandsTest::resetIdentities()
{
    KIdentityManagementCore::Identity &i1 = mKernel->identityManager()->modifyIdentityForName(u"default"_s);
    i1.setFullName(u"default"_s);
    i1.setPrimaryEmailAddress(u"firstname.lastname@example.com"_s);
    i1.setPGPSigningKey("0x123456789");
    i1.setPgpAutoSign(true);
    KIdentityManagementCore::Identity &i2 = mKernel->identityManager()->modifyIdentityForName(u"test2"_s);
    i2.setFullName(u"second"_s);
    i2.setPrimaryEmailAddress(u"secundus@example.com"_s);
    i2.setPGPSigningKey("0x234567890");
    i2.setPgpAutoSign(false);
    KIdentityManagementCore::Identity &i3 = mKernel->identityManager()->modifyIdentityForName(u"test3"_s);
    i3.setFullName(u"third"_s);
    i3.setPrimaryEmailAddress(u"drei@example.com"_s);
    i3.setPGPSigningKey("0x345678901");
    i3.setPgpAutoSign(true);
    mKernel->identityManager()->commit();
}

void KMCommandsTest::verifyEncryption(bool encrypt)
{
    const KMainWindow *w = mKernel->mainWin();
    auto encryption = w->findChild<QLabel *>(u"encryptionindicator"_s);
    QVERIFY(encryption);
    QCOMPARE(encryption->isVisible(), encrypt);
}

void KMCommandsTest::verifySignature(bool sign)
{
    const KMainWindow *w = mKernel->mainWin();
    auto signature = w->findChild<QLabel *>(u"signatureindicator"_s);
    QVERIFY(signature);
    QCOMPARE(signature->isVisible(), sign);
}

void KMCommandsTest::testMailtoReply()
{
    resetIdentities();
    {
        // default has auto sign set -> verifySignature = true
        const KIdentityManagementCore::Identity &ident = mKernel->identityManager()->defaultIdentity();
        Akonadi::Item item(createItem(ident));

        auto cmd(new KMMailtoReplyCommand(nullptr, QUrl(u"mailto:test@example.com"_s), item, QString()));
        cmd->start();
        verifySignature(true);
        waitForMainWindowToClose();
    }
    {
        // secundus has no auto sign set -> verifySignature = false
        const KIdentityManagementCore::Identity &ident = mKernel->identityManager()->identityForAddress(u"secundus@example.com"_s);
        Akonadi::Item item(createItem(ident));

        auto cmd(new KMMailtoReplyCommand(nullptr, QUrl(u"mailto:test@example.com"_s), item, QString()));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {
        // drei has auto sign set -> verifySignature = true
        const KIdentityManagementCore::Identity &ident = mKernel->identityManager()->identityForAddress(u"drei@example.com"_s);
        Akonadi::Item item(createItem(ident));

        auto cmd(new KMMailtoReplyCommand(nullptr, QUrl(u"mailto:test@example.com"_s), item, QString()));
        cmd->start();
        verifySignature(true);
        waitForMainWindowToClose();
    }
}

void KMCommandsTest::testReply()
{
    resetIdentities();
    {
        // default has auto sign set -> verifySignature = true
        const KIdentityManagementCore::Identity &ident = mKernel->identityManager()->defaultIdentity();
        Akonadi::Item item(createItem(ident));

        auto cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(true);
        waitForMainWindowToClose();
    }
    {
        // secundus has no auto sign set -> verifySignature = false
        const KIdentityManagementCore::Identity &ident = mKernel->identityManager()->identityForAddress(u"secundus@example.com"_s);
        Akonadi::Item item(createItem(ident));

        auto cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {
        // drei has auto sign set -> verifySignature = true
        const KIdentityManagementCore::Identity &ident = mKernel->identityManager()->identityForAddress(u"drei@example.com"_s);
        Akonadi::Item item(createItem(ident));

        auto cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(true);
        waitForMainWindowToClose();
    }
}

void KMCommandsTest::testReplyWithoutDefaultGPGSign()
{
    resetIdentities();
    KIdentityManagementCore::Identity &i1 = mKernel->identityManager()->modifyIdentityForName(u"default"_s);
    i1.setPgpAutoSign(false);
    mKernel->identityManager()->commit();

    {
        // default has auto sign set -> verifySignature = true
        const KIdentityManagementCore::Identity &ident = mKernel->identityManager()->defaultIdentity();
        Akonadi::Item item(createItem(ident));

        auto cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {
        // secundus has no auto sign set -> verifySignature = false
        const KIdentityManagementCore::Identity &ident = mKernel->identityManager()->identityForAddress(u"secundus@example.com"_s);
        Akonadi::Item item(createItem(ident));

        auto cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {
        // drei has auto sign set -> verifySignature = true
        const KIdentityManagementCore::Identity &ident = mKernel->identityManager()->identityForAddress(u"drei@example.com"_s);
        Akonadi::Item item(createItem(ident));

        auto cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(true);
        waitForMainWindowToClose();
    }
}

void KMCommandsTest::testSendAgain()
{
    resetIdentities();
    {
        const KIdentityManagementCore::Identity &ident = mKernel->identityManager()->defaultIdentity();
        Akonadi::Item item(createItem(ident));

        auto cmd(new KMResendMessageCommand(nullptr, item));
        cmd->start();
        QVERIFY(!cmd->retrievedMsgs().isEmpty());
        waitForMainWindowToClose();
    }
}

void KMCommandsTest::waitForMainWindowToClose()
{
    KMainWindow *w = mKernel->mainWin();
    QEventLoop loop;
    loop.connect(w, &QMainWindow::destroyed, &loop, &QEventLoop::quit);
    w->close();
    loop.exec();
}

int main(int argc, char *argv[])
{
    QStandardPaths::setTestModeEnabled(true);
    QTemporaryDir config;
    qputenv("LC_ALL", "C");
    qputenv("XDG_CONFIG_HOME", config.path().toUtf8());
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    KMCommandsTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "moc_kmcommandstest.cpp"
