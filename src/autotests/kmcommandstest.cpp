/*
  SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmcommandstest.h"
#include "kmcommands.h"
#include "kmkernel.h"
#include <KIdentityManagement/Identity>
#include <KIdentityManagement/IdentityManager>

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>

#include <KMime/Message>

#include <QEventLoop>
#include <QLabel>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

Akonadi::Item createItem(const KIdentityManagement::Identity &ident)
{
    QByteArray data
        = "From: Konqui <konqui@kde.org>\n"
          "To: Friends <friends@kde.org>\n"
          "Date: Sun, 21 Mar 1993 23:56:48 -0800 (PST)\n"
          "Subject: Sample message\n"
          "MIME-Version: 1.0\n"
          "X-KMail-Identity: " + QByteArray::number(ident.uoid()) + "\n"
                                                                    "Content-type: text/plain; charset=us-ascii\n"
                                                                    "\n"
                                                                    "\n"
                                                                    "This is explicitly typed plain US-ASCII text.\n"
                                                                    "It DOES end with a linebreak.\n"
                                                                    "\n";

    KMime::Message::Ptr msgPtr = KMime::Message::Ptr(new KMime::Message());
    Akonadi::Item item;
    Akonadi::Collection col(0);
    msgPtr->setContent(data);
    msgPtr->parse();
    item.setPayload<KMime::Message::Ptr>(msgPtr);
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
    const KIdentityManagement::Identity &def = mKernel->identityManager()->defaultIdentity();
    KIdentityManagement::Identity &i1 = mKernel->identityManager()->modifyIdentityForUoid(def.uoid());
    i1.setIdentityName(QStringLiteral("default"));
    mKernel->identityManager()->newFromScratch(QStringLiteral("test2"));
    mKernel->identityManager()->newFromScratch(QStringLiteral("test3"));
    mKernel->identityManager()->commit();
}

void KMCommandsTest::resetIdentities()
{
    KIdentityManagement::Identity &i1 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("default"));
    i1.setFullName(QStringLiteral("default"));
    i1.setPrimaryEmailAddress(QStringLiteral("firstname.lastname@example.com"));
    i1.setPGPSigningKey("0x123456789");
    i1.setPgpAutoSign(true);
    KIdentityManagement::Identity &i2 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("test2"));
    i2.setFullName(QStringLiteral("second"));
    i2.setPrimaryEmailAddress(QStringLiteral("secundus@example.com"));
    i2.setPGPSigningKey("0x234567890");
    i2.setPgpAutoSign(false);
    KIdentityManagement::Identity &i3 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("test3"));
    i3.setFullName(QStringLiteral("third"));
    i3.setPrimaryEmailAddress(QStringLiteral("drei@example.com"));
    i3.setPGPSigningKey("0x345678901");
    i3.setPgpAutoSign(true);
    mKernel->identityManager()->commit();
}

void KMCommandsTest::verifyEncryption(bool encrypt)
{
    const KMainWindow *w = mKernel->mainWin();
    auto *encryption = w->findChild<QLabel *>(QStringLiteral("encryptionindicator"));
    QVERIFY(encryption);
    QCOMPARE(encryption->isVisible(), encrypt);
}

void KMCommandsTest::verifySignature(bool sign)
{
    const KMainWindow *w = mKernel->mainWin();
    auto *signature = w->findChild<QLabel *>(QStringLiteral("signatureindicator"));
    QVERIFY(signature);
    QCOMPARE(signature->isVisible(), sign);
}

void KMCommandsTest::testMailtoReply()
{
    resetIdentities();
    {
        // default has auto sign set -> verifySignature = true
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->defaultIdentity();
        Akonadi::Item item(createItem(ident));

        auto *cmd(new KMMailtoReplyCommand(nullptr, QUrl(QStringLiteral("mailto:test@example.com")), item, QString()));
        cmd->start();
        verifySignature(true);
        waitForMainWindowToClose();
    }
    {
        // secundus has no auto sign set -> verifySignature = false
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("secundus@example.com"));
        Akonadi::Item item(createItem(ident));

        auto *cmd(new KMMailtoReplyCommand(nullptr, QUrl(QStringLiteral("mailto:test@example.com")), item, QString()));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {
        // drei has auto sign set -> verifySignature = true
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("drei@example.com"));
        Akonadi::Item item(createItem(ident));

        auto *cmd(new KMMailtoReplyCommand(nullptr, QUrl(QStringLiteral("mailto:test@example.com")), item, QString()));
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
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->defaultIdentity();
        Akonadi::Item item(createItem(ident));

        auto *cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(true);
        waitForMainWindowToClose();
    }
    {
        // secundus has no auto sign set -> verifySignature = false
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("secundus@example.com"));
        Akonadi::Item item(createItem(ident));

        auto *cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {
        // drei has auto sign set -> verifySignature = true
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("drei@example.com"));
        Akonadi::Item item(createItem(ident));

        auto *cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(true);
        waitForMainWindowToClose();
    }
}

void KMCommandsTest::testReplyWithoutDefaultGPGSign()
{
    resetIdentities();
    KIdentityManagement::Identity &i1 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("default"));
    i1.setPgpAutoSign(false);
    mKernel->identityManager()->commit();

    {
        // default has auto sign set -> verifySignature = true
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->defaultIdentity();
        Akonadi::Item item(createItem(ident));

        auto *cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {
        // secundus has no auto sign set -> verifySignature = false
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("secundus@example.com"));
        Akonadi::Item item(createItem(ident));

        auto *cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {
        // drei has auto sign set -> verifySignature = true
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("drei@example.com"));
        Akonadi::Item item(createItem(ident));

        auto *cmd(new KMReplyCommand(nullptr, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(true);
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
    QTemporaryDir config;
    qputenv("LC_ALL", "C");
    qputenv("XDG_CONFIG_HOME", config.path().toUtf8());
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    QTEST_DISABLE_KEYPAD_NAVIGATION
    KMCommandsTest tc;
    return QTest::qExec(&tc, argc, argv);
}
