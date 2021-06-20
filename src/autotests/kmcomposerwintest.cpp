/*
  SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmcomposerwintest.h"
#include "composer.h"
#include "kmkernel.h"

#include <KIdentityManagement/Identity>
#include <KIdentityManagement/IdentityCombo>
#include <KIdentityManagement/IdentityManager>

#include <KMime/Message>

#include <QEventLoop>
#include <QLabel>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <chrono>

using namespace std::chrono_literals;

QTEST_MAIN(KMComposerWinTest)

KMime::Message::Ptr createItem(const KIdentityManagement::Identity &ident)
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

    auto msgPtr = KMime::Message::Ptr(new KMime::Message());
    msgPtr->setContent(data);
    msgPtr->parse();
    return msgPtr;
}

KMComposerWinTest::KMComposerWinTest(QObject *parent)
    : QObject(parent)
    , mKernel(new KMKernel(parent))
{
}

KMComposerWinTest::~KMComposerWinTest()
{
    delete mKernel;
}

void KMComposerWinTest::init()
{
    autocryptDir.removeRecursively();
    autocryptDir.mkpath(QStringLiteral("."));
}

void KMComposerWinTest::cleanup()
{
    autocryptDir.removeRecursively();
    QEventLoop loop;
    auto w = mKernel->mainWin();
    loop.connect(w, &QMainWindow::destroyed, &loop, &QEventLoop::quit);
    w->close();
    loop.exec();

    resetIdentities();
}

void KMComposerWinTest::initTestCase()
{
    qputenv("LC_ALL", "C");
    qputenv("KDEHOME", QFile::encodeName(QDir::homePath() + QLatin1String("/.qttest")).constData());

    const QDir genericDataLocation(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    autocryptDir = QDir(genericDataLocation.filePath(QStringLiteral("autocrypt")));

    const KIdentityManagement::Identity &def = mKernel->identityManager()->defaultIdentity();
    KIdentityManagement::Identity &i1 = mKernel->identityManager()->modifyIdentityForUoid(def.uoid());
    i1.setIdentityName(QStringLiteral("default"));
    mKernel->identityManager()->newFromScratch(QStringLiteral("test2"));
    mKernel->identityManager()->newFromScratch(QStringLiteral("test3"));
    mKernel->identityManager()->newFromScratch(QStringLiteral("autocrypt"));
    mKernel->identityManager()->commit();
    resetIdentities();
}

void KMComposerWinTest::resetIdentities()
{
    KIdentityManagement::Identity &i1 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("default"));
    i1.setFullName(QStringLiteral("default"));
    i1.setPrimaryEmailAddress(QStringLiteral("firstname.lastname@example.com"));
    i1.setPGPSigningKey("0x123456789");
    i1.setPGPEncryptionKey("0x123456789");
    i1.setPgpAutoSign(true);
    i1.setPgpAutoEncrypt(false);
    KIdentityManagement::Identity &i2 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("test2"));
    i2.setFullName(QStringLiteral("second"));
    i2.setPrimaryEmailAddress(QStringLiteral("secundus@example.com"));
    i2.setPGPSigningKey("0x234567890");
    i2.setPGPEncryptionKey("0x234567890");
    i2.setPgpAutoSign(false);
    i2.setPgpAutoEncrypt(false);
    KIdentityManagement::Identity &i3 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("test3"));
    i3.setFullName(QStringLiteral("third"));
    i3.setPrimaryEmailAddress(QStringLiteral("drei@example.com"));
    i3.setPGPSigningKey("0x345678901");
    i3.setPGPEncryptionKey("0x345678901");
    i3.setPgpAutoSign(true);
    i3.setPgpAutoEncrypt(true);
    KIdentityManagement::Identity &i4 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("autocrypt"));
    i4.setFullName(QStringLiteral("autocrypt"));
    i4.setPrimaryEmailAddress(QStringLiteral("autocrypt@example.com"));
    i4.setPGPSigningKey("0x456789012");
    i4.setPGPEncryptionKey("0x456789012");
    i4.setPgpAutoSign(true);
    i4.setPgpAutoEncrypt(true);
    i4.setAutocryptEnabled(true);
    mKernel->identityManager()->commit();
}

void KMComposerWinTest::testSignature_data()
{
    const auto im =  mKernel->identityManager();

    QTest::addColumn<uint>("uoid");
    QTest::addColumn<bool>("sign");

    QTest::newRow("default") << im->defaultIdentity().uoid() << true;
    QTest::newRow("secondus@example.com") << im->identityForAddress(QStringLiteral("secundus@example.com")).uoid() << false;
    QTest::newRow("drei@example.com") << im->identityForAddress(QStringLiteral("drei@example.com")).uoid() << true;
}

void KMComposerWinTest::testSignature()
{
    QFETCH(uint, uoid);
    QFETCH(bool, sign);

    const auto ident = mKernel->identityManager()->identityForUoid(uoid);
    const auto msg(createItem(ident));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    auto *signature = composer->findChild<QLabel *>(QStringLiteral("signatureindicator"));
    QVERIFY(signature);
    QCOMPARE(signature->isVisible(), sign);
    composer->close();
}

void KMComposerWinTest::testEncryption_data()
{
    const auto im =  mKernel->identityManager();

    QTest::addColumn<uint>("uoid");
    QTest::addColumn<bool>("encrypt");

    QTest::newRow("default") << im->defaultIdentity().uoid() << false;
    QTest::newRow("secondus@example.com") << im->identityForAddress(QStringLiteral("secundus@example.com")).uoid() << false;
    QTest::newRow("drei@example.com") << im->identityForAddress(QStringLiteral("drei@example.com")).uoid() << false;
    QTest::newRow("autocrypt@example.com") << im->identityForAddress(QStringLiteral("autocrypt@example.com")).uoid() << true;
}

void KMComposerWinTest::testEncryption()
{
    QFETCH(uint, uoid);
    QFETCH(bool, encrypt);

    QFile file1(QLatin1String(TEST_DATA_DIR) + QStringLiteral("/autocrypt/friends%40kde.org.json"));
    QVERIFY(file1.copy(autocryptDir.filePath(QStringLiteral("friends%40kde.org.json"))));

    const auto ident = mKernel->identityManager()->identityForUoid(uoid);
    const auto msg(createItem(ident));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    auto *encryption = composer->findChild<QLabel *>(QStringLiteral("encryptionindicator"));
    QVERIFY(encryption);
    QCOMPARE(encryption->isVisible(), encrypt);
    composer->close();
}

void KMComposerWinTest::testChangeIdentity()
{
    QFile file1(QLatin1String(TEST_DATA_DIR) + QStringLiteral("/autocrypt/friends%40kde.org.json"));
    QVERIFY(file1.copy(autocryptDir.filePath(QStringLiteral("friends%40kde.org.json"))));

    const auto im =  mKernel->identityManager();

    auto ident = im->defaultIdentity();
    const auto msg(createItem(ident));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    auto encryption = composer->findChild<QLabel *>(QStringLiteral("encryptionindicator"));
    auto signature = composer->findChild<QLabel *>(QStringLiteral("signatureindicator"));
    QVERIFY(encryption);
    QVERIFY(signature);
    QCOMPARE(encryption->isVisible(), false);
    QCOMPARE(signature->isVisible(), true);

    {
        ident = im->identityForAddress(QStringLiteral("autocrypt@example.com"));
        auto identCombo = composer->findChild<KIdentityManagement::IdentityCombo *>(QStringLiteral("identitycombo"));
        QVERIFY(identCombo);
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(encryption->isVisible(), true);
        QCOMPARE(signature->isVisible(), true);
    }

    {
        ident = im->identityForAddress(QStringLiteral("secundus@example.com"));
        auto identCombo = composer->findChild<KIdentityManagement::IdentityCombo *>(QStringLiteral("identitycombo"));
        QVERIFY(identCombo);
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(encryption->isVisible(), false);
        QCOMPARE(signature->isVisible(), false);
    }

}
