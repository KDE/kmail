/*
  Copyright (c) 2016 Sandro Knauß <sknauss@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "kmcommandstest.h"
#include "kmcommands.h"
#include "kmkernel.h"
#include <KIdentityManagement/IdentityManager>
#include <KIdentityManagement/Identity>

#include <AkonadiCore/Item>
#include <AkonadiCore/Collection>

#include <KMime/Message>

#include <QDebug>
#include <QEventLoop>
#include <QLabel>
#include <QStandardPaths>
#include <QTest>
#include <QTemporaryDir>

Akonadi::Item createItem(const KIdentityManagement::Identity &ident)
{
    QByteArray data =
    "From: Konqui <konqui@kde.org>\n"
    "To: Friends <friends@kde.org>\n"
    "Date: Sun, 21 Mar 1993 23:56:48 -0800 (PST)\n"
    "Subject: Sample message\n"
    "MIME-Version: 1.0\n"
    "X-KMail-Identity: "+ QByteArray::number(ident.uoid()) +"\n"
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
    const KMainWindow *w =  mKernel->mainWin();
    QLabel *encryption = w->findChild<QLabel *>(QStringLiteral("encryptionindicator"));
    QVERIFY(encryption);
    QCOMPARE(encryption->isVisible(), encrypt);
}

void KMCommandsTest::verifySignature(bool sign)
{
    const KMainWindow *w =  mKernel->mainWin();
    QLabel *signature = w->findChild<QLabel *>(QStringLiteral("signatureindicator"));
    QVERIFY(signature);
    QCOMPARE(signature->isVisible(), sign);
}

void KMCommandsTest::testMailtoReply()
{
    resetIdentities();
    {   // default has auto sign set -> verifySignature = true
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->defaultIdentity();
        Akonadi::Item item(createItem(ident));

        KMMailtoReplyCommand *cmd(new KMMailtoReplyCommand(Q_NULLPTR, QStringLiteral("mailto:test@example.com") , item, QString()));
        cmd->start();
        verifySignature(true);
        waitForMainWindowToClose();
    }
    {   // secundus has no auto sign set -> verifySignature = false
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("secundus@example.com"));
        Akonadi::Item item(createItem(ident));

        KMMailtoReplyCommand *cmd(new KMMailtoReplyCommand(Q_NULLPTR, QStringLiteral("mailto:test@example.com") , item, QString()));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {   // drei has auto sign set -> verifySignature = true
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("drei@example.com"));
        Akonadi::Item item(createItem(ident));

        KMMailtoReplyCommand *cmd(new KMMailtoReplyCommand(Q_NULLPTR, QStringLiteral("mailto:test@example.com") , item, QString()));
        cmd->start();
        verifySignature(true);
        waitForMainWindowToClose();
    }
}

void KMCommandsTest::testReply()
{
    resetIdentities();
    {   // default has auto sign set -> verifySignature = true
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->defaultIdentity();
        Akonadi::Item item(createItem(ident));

        KMReplyCommand *cmd(new KMReplyCommand(Q_NULLPTR, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(true);
        waitForMainWindowToClose();
    }
    {   // secundus has no auto sign set -> verifySignature = false
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("secundus@example.com"));
        Akonadi::Item item(createItem(ident));

        KMReplyCommand *cmd(new KMReplyCommand(Q_NULLPTR, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {   // drei has auto sign set -> verifySignature = true
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("drei@example.com"));
        Akonadi::Item item(createItem(ident));

        KMReplyCommand *cmd(new KMReplyCommand(Q_NULLPTR, item, MessageComposer::ReplyAll));
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

    {   // default has auto sign set -> verifySignature = true
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->defaultIdentity();
        Akonadi::Item item(createItem(ident));

        KMReplyCommand *cmd(new KMReplyCommand(Q_NULLPTR, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {   // secundus has no auto sign set -> verifySignature = false
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("secundus@example.com"));
        Akonadi::Item item(createItem(ident));

        KMReplyCommand *cmd(new KMReplyCommand(Q_NULLPTR, item, MessageComposer::ReplyAll));
        cmd->start();
        verifySignature(false);
        waitForMainWindowToClose();
    }
    {   // drei has auto sign set -> verifySignature = true
        const KIdentityManagement::Identity &ident = mKernel->identityManager()->identityForAddress(QStringLiteral("drei@example.com"));
        Akonadi::Item item(createItem(ident));

        KMReplyCommand *cmd(new KMReplyCommand(Q_NULLPTR, item, MessageComposer::ReplyAll));
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
    setenv("LC_ALL", "C", 1);
    setenv("XDG_CONFIG_HOME", config.path().toUtf8(), 1);
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    QTEST_DISABLE_KEYPAD_NAVIGATION
    KMCommandsTest tc;
    return QTest::qExec(&tc, argc, argv);
}
