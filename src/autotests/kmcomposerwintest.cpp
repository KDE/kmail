/*
  SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmcomposerwintest.h"
#include "kmkernel.h"
#include <MessageComposer/Composer>

#include <gpgme.h>
#include <gpgme++/key.h>

#include <KEmailAddress>

#include <KIdentityManagement/Identity>
#include <KIdentityManagement/IdentityCombo>
#include <KIdentityManagement/IdentityManager>

#include <Libkleo/KeyCache>

#include <KMime/Message>

#include <QEventLoop>
#include <QLabel>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QTimer>

#include <chrono>

using namespace std::chrono_literals;

QTEST_MAIN(KMComposerWinTest)

auto mapValidity(GpgME::UserID::Validity validity)
{
    switch (validity) {
    default:
    case GpgME::UserID::Unknown:
        return GPGME_VALIDITY_UNKNOWN;
    case GpgME::UserID::Undefined:
        return GPGME_VALIDITY_UNDEFINED;
    case GpgME::UserID::Never:
        return GPGME_VALIDITY_NEVER;
    case GpgME::UserID::Marginal:
        return GPGME_VALIDITY_MARGINAL;
    case GpgME::UserID::Full:
        return GPGME_VALIDITY_FULL;
    case GpgME::UserID::Ultimate:
        return GPGME_VALIDITY_ULTIMATE;
    }
}

GpgME::Key createTestKey(QByteArray uid, QByteArray fingerprint,
                         GpgME::Protocol protocol = GpgME::UnknownProtocol,
                         Kleo::KeyCache::KeyUsage usage = Kleo::KeyCache::KeyUsage::AnyUsage,
                         GpgME::UserID::Validity validity = GpgME::UserID::Full)
{
    static int count = 0;
    count++;

    gpgme_key_t key;
    gpgme_key_from_uid(&key, uid.constData());
    Q_ASSERT(key);
    Q_ASSERT(key->uids);
    if (protocol != GpgME::UnknownProtocol) {
        key->protocol = protocol == GpgME::OpenPGP ? GPGME_PROTOCOL_OpenPGP : GPGME_PROTOCOL_CMS;
    }
    key->fpr = strdup(fingerprint.rightJustified(40, '0').constData());
    key->revoked = 0;
    key->expired = 0;
    key->disabled = 0;
    key->can_encrypt = int(usage == Kleo::KeyCache::KeyUsage::AnyUsage || usage == Kleo::KeyCache::KeyUsage::Encrypt);
    key->can_sign = int(usage == Kleo::KeyCache::KeyUsage::AnyUsage || usage == Kleo::KeyCache::KeyUsage::Sign);
    key->secret = 1;
    key->uids->validity = mapValidity(validity);
    gpgme_subkey_t subkey;
    subkey = (gpgme_subkey_t) calloc (1, sizeof *subkey);
    key->subkeys = subkey;
    key->_last_subkey = subkey;
    subkey->timestamp = 123456789;
    subkey->revoked = 0;
    subkey->expired = 0;
    subkey->disabled = 0;
    subkey->keyid = strdup(fingerprint.constData());
    subkey->can_encrypt = int(usage == Kleo::KeyCache::KeyUsage::AnyUsage || usage == Kleo::KeyCache::KeyUsage::Encrypt);
    subkey->can_sign = int(usage == Kleo::KeyCache::KeyUsage::AnyUsage || usage == Kleo::KeyCache::KeyUsage::Sign);

    return GpgME::Key(key, false);
}

void killAgent()
{
    QProcess proc;
    proc.setProgram(QStringLiteral("gpg-connect-agent"));
    proc.start();
    proc.waitForStarted();
    proc.write("KILLAGENT\n");
    proc.write("BYE\n");
    proc.closeWriteChannel();
    proc.waitForFinished();
}

KMime::Message::Ptr createItem(const KIdentityManagement::Identity &ident, QByteArray recipient="Friends <friends@kde.example>")
{
    QByteArray data
        = "From: Konqui <konqui@kde.example>\n"
          "To: " + recipient +"\n"
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
{
    QStandardPaths::setTestModeEnabled(true);
    qputenv("KDEHOME", QFile::encodeName(QDir::homePath() + QLatin1String("/.qttest")).constData());

    const QDir genericDataLocation(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    auto gnupgDir = QDir(genericDataLocation.filePath(QStringLiteral("gnupg")));
    qputenv("GNUPGHOME", gnupgDir.absolutePath().toUtf8());
    gnupgDir.mkpath(QStringLiteral("."));

    // We need to initalize KMKernel after modifing the envionment variables, otherwise we would read the wrong configs.
    mKernel = new KMKernel(parent);
}

KMComposerWinTest::~KMComposerWinTest()
{
    delete mKernel;
}

void KMComposerWinTest::init()
{
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
}

void KMComposerWinTest::initTestCase()
{
    const QDir genericDataLocation(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    autocryptDir = QDir(genericDataLocation.filePath(QStringLiteral("autocrypt")));

    resetIdentities();

    // Keep a KeyCache reference. Make sure that the cache stays hot between the tests.
    mKernel->init();

    Kleo::KeyCache::mutableInstance()->setKeys({
        createTestKey("test <drei@test.example>",   "345678901", GpgME::OpenPGP, Kleo::KeyCache::KeyCache::KeyUsage::AnyUsage),
        createTestKey("friends@kde.example", "1", GpgME::OpenPGP, Kleo::KeyCache::KeyCache::KeyUsage::AnyUsage),
    });
}

void KMComposerWinTest::cleanupTestCase()
{
    QVERIFY(mKernel->identityManager()->removeIdentity(QStringLiteral("testautocrypt")));
    QVERIFY(mKernel->identityManager()->removeIdentity(QStringLiteral("test3")));
    QVERIFY(mKernel->identityManager()->removeIdentity(QStringLiteral("test2")));
    QVERIFY(mKernel->identityManager()->removeIdentity(QStringLiteral("default")));
    mKernel->identityManager()->commit();
    killAgent();
}

void KMComposerWinTest::resetIdentities()
{
    KIdentityManagement::Identity &i1 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("default"));
    i1.setPrimaryEmailAddress(QStringLiteral("firstname.lastname@test.example"));
    i1.setPGPSigningKey("0x123456789");
    i1.setPGPEncryptionKey("0x123456789");
    i1.setPgpAutoSign(true);
    i1.setPgpAutoEncrypt(false);
    KIdentityManagement::Identity &i2 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("test2"));
    i2.setPrimaryEmailAddress(QStringLiteral("secundus@test.example"));
    i2.setPGPSigningKey("0x234567890");
    i2.setPGPEncryptionKey("0x234567890");
    i2.setPgpAutoSign(false);
    i2.setPgpAutoEncrypt(false);
    KIdentityManagement::Identity &i3 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("test3"));
    i3.setPrimaryEmailAddress(QStringLiteral("drei@test.example"));
    i3.setPGPSigningKey("0x345678901");
    i3.setPGPEncryptionKey("0x345678901");
    i3.setPgpAutoSign(true);
    i3.setPgpAutoEncrypt(true);
    KIdentityManagement::Identity &i4 = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("testautocrypt"));
    i4.setPrimaryEmailAddress(QStringLiteral("autocrypt@test.example"));
    i4.setPGPSigningKey("345678901");
    i4.setPGPEncryptionKey("345678901");
    i4.setPgpAutoSign(true);
    i4.setPgpAutoEncrypt(true);
    i4.setAutocryptEnabled(true);
    mKernel->identityManager()->commit();
}

void KMComposerWinTest::testSignature_data()
{
    const auto im = mKernel->identityManager();

    QTest::addColumn<uint>("uoid");
    QTest::addColumn<bool>("sign");

    QTest::newRow("default") << im->identityForAddress(QStringLiteral("firstname.lastname@test.example")).uoid() << true;
    QTest::newRow("secondus@test.example") << im->identityForAddress(QStringLiteral("secundus@test.example")).uoid() << false;
    QTest::newRow("drei@test.example") << im->identityForAddress(QStringLiteral("drei@test.example")).uoid() << true;
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
    auto signature = composer->findChild<QLabel *>(QStringLiteral("signatureindicator"));
    QVERIFY(signature);
    QCOMPARE(signature->isVisible(), sign);
    composer->close();
}

void KMComposerWinTest::testEncryption_data()
{
    const auto im = mKernel->identityManager();
    const QString recipient(QStringLiteral("Friends <friends@kde.example>"));

    QTest::addColumn<uint>("uoid");
    QTest::addColumn<QString>("recipient");
    QTest::addColumn<bool>("encrypt");

    QTest::newRow("default") << im->identityForAddress(QStringLiteral("firstname.lastname@test.example")).uoid() << recipient << false;
    QTest::newRow("secondus@test.example") << im->identityForAddress(QStringLiteral("secundus@test.example")).uoid() << recipient << false;
    QTest::newRow("drei@test.example") << im->identityForAddress(QStringLiteral("drei@test.example")).uoid() << recipient << true;
    QTest::newRow("autocrypt@test.example") << im->identityForAddress(QStringLiteral("autocrypt@test.example")).uoid() << "Autocrypt <friends@autocrypt.example>" << true;
}

void KMComposerWinTest::testEncryption()
{
    QFETCH(uint, uoid);
    QFETCH(QString, recipient);
    QFETCH(bool, encrypt);

    QString dummy;
    QString addrSpec;
    if (KEmailAddress::splitAddress(recipient, dummy, addrSpec, dummy) != KEmailAddress::AddressOk) {
        addrSpec = recipient;
    }

    QFile file1(QLatin1String(TEST_DATA_DIR) + QStringLiteral("/autocrypt/friends%40kde.org.json"));
    QVERIFY(file1.copy(autocryptDir.filePath(addrSpec.replace(QStringLiteral("@"),QStringLiteral("%40")) + QStringLiteral(".json"))));

    const auto ident = mKernel->identityManager()->identityForUoid(uoid);
    const auto msg(createItem(ident, recipient.toUtf8()));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));

    // We need to wait till KeyCache is populated, otherwise we will get wrong results
    const auto instance = Kleo::KeyCache::instance();
    if (!instance->initialized()) {
        QEventLoop loop;
        connect(instance.get(), &Kleo::KeyCache::keyListingDone, this, [&loop](){
            loop.quit();
        });
        loop.exec();
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    auto encryption = composer->findChild<QLabel *>(QStringLiteral("encryptionindicator"));
    QVERIFY(encryption);
    QCOMPARE(encryption->isVisible(), encrypt);
}

void KMComposerWinTest::testChangeIdentity()
{
    QFile file1(QLatin1String(TEST_DATA_DIR) + QStringLiteral("/autocrypt/friends%40kde.org.json"));
    QVERIFY(file1.copy(autocryptDir.filePath(QStringLiteral("friends%40kde.example.json"))));

    const auto im = mKernel->identityManager();

    auto ident = im->identityForAddress(QStringLiteral("firstname.lastname@test.example"));
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
        ident = im->identityForAddress(QStringLiteral("autocrypt@test.example"));
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
        ident = im->identityForAddress(QStringLiteral("secundus@test.example"));
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
