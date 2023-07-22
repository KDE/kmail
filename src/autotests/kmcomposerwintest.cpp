/*
  SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmcomposerwintest.h"
#include "editor/warningwidgets/nearexpirywarning.h"
#include "kmkernel.h"
#include <MessageComposer/Composer>

#include <gpgme.h>
#include <gpgme++/key.h>

#include <KEmailAddress>
#include <KToggleAction>

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
        createTestKey("encrypt <encrypt@test.example>",   "345678901", GpgME::OpenPGP, Kleo::KeyCache::KeyCache::KeyUsage::AnyUsage),
        createTestKey("wrongkey <wrongkey@test.example>",   "22222222", GpgME::OpenPGP, Kleo::KeyCache::KeyCache::KeyUsage::AnyUsage),
        createTestKey("friends@kde.example", "1", GpgME::OpenPGP, Kleo::KeyCache::KeyCache::KeyUsage::AnyUsage),
    });
}

void KMComposerWinTest::cleanupTestCase()
{
    QVERIFY(mKernel->identityManager()->removeIdentity(QStringLiteral("wrongkeysign")));
    QVERIFY(mKernel->identityManager()->removeIdentity(QStringLiteral("wrongkey")));
    QVERIFY(mKernel->identityManager()->removeIdentity(QStringLiteral("autocrypt")));
    QVERIFY(mKernel->identityManager()->removeIdentity(QStringLiteral("signonly")));
    QVERIFY(mKernel->identityManager()->removeIdentity(QStringLiteral("encryptonly")));
    QVERIFY(mKernel->identityManager()->removeIdentity(QStringLiteral("signandencrypt")));
    QVERIFY(mKernel->identityManager()->removeIdentity(QStringLiteral("nothing")));
    mKernel->identityManager()->commit();
    killAgent();
}

void KMComposerWinTest::resetIdentities()
{
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("nothing"));
        i.setPrimaryEmailAddress(QStringLiteral("nothing@test.example"));
        i.setPGPSigningKey("345678901");
        i.setPGPEncryptionKey("345678901");
        i.setPgpAutoSign(false);
        i.setPgpAutoEncrypt(false);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("signonly"));
        i.setPrimaryEmailAddress(QStringLiteral("signonly@test.example"));
        i.setPGPSigningKey("345678901");
        i.setPGPEncryptionKey("345678901");
        i.setPgpAutoSign(true);
        i.setPgpAutoEncrypt(false);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("encryptonly"));
        i.setPrimaryEmailAddress(QStringLiteral("encryptonly@test.example"));
        i.setPGPSigningKey("345678901");
        i.setPGPEncryptionKey("345678901");
        i.setPgpAutoSign(false);
        i.setPgpAutoEncrypt(true);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("signandencrypt"));
        i.setPrimaryEmailAddress(QStringLiteral("signandencrypt@test.example"));
        i.setPGPSigningKey("345678901");
        i.setPGPEncryptionKey("345678901");
        i.setPgpAutoSign(true);
        i.setPgpAutoEncrypt(true);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("autocrypt"));
        i.setPrimaryEmailAddress(QStringLiteral("autocrypt@test.example"));
        i.setPGPSigningKey("345678901");
        i.setPGPEncryptionKey("345678901");
        i.setPgpAutoSign(true);
        i.setPgpAutoEncrypt(true);
        i.setAutocryptEnabled(true);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("wrongkey"));
        i.setPrimaryEmailAddress(QStringLiteral("wrongkey@test.example"));
        i.setPGPSigningKey("1111111");
        i.setPGPEncryptionKey("1111111");
        i.setPgpAutoSign(true);
        i.setPgpAutoEncrypt(true);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(QStringLiteral("wrongkeysign"));
        i.setPrimaryEmailAddress(QStringLiteral("wrongkeysign@test.example"));
        i.setPGPSigningKey("1111111");
        i.setPGPEncryptionKey("22222222");
        i.setPgpAutoSign(true);
        i.setPgpAutoEncrypt(true);
    }
    mKernel->identityManager()->commit();
}

void KMComposerWinTest::testSigning_data()
{
    const auto im = mKernel->identityManager();

    QTest::addColumn<uint>("uoid");
    QTest::addColumn<bool>("sign");
    QTest::addColumn<bool>("sign_possible");

    QTest::newRow("nothing") << im->identityForAddress(QStringLiteral("nothing@test.example")).uoid() << false << true;
    QTest::newRow("signonly") << im->identityForAddress(QStringLiteral("signonly@test.example")).uoid() << true << true;
    QTest::newRow("encryptonly") << im->identityForAddress(QStringLiteral("encryptonly@test.example")).uoid() << false << true;
    QTest::newRow("signandencrypt") << im->identityForAddress(QStringLiteral("signandencrypt@test.example")).uoid() << true << true;
    QTest::newRow("autocrypt") << im->identityForAddress(QStringLiteral("autocrypt@test.example")).uoid() << true << true;
    QTest::newRow("wrongkey") << im->identityForAddress(QStringLiteral("wrongkey@test.example")).uoid() << false << false;
    QTest::newRow("wrongkeysign") << im->identityForAddress(QStringLiteral("wrongkeysign@test.example")).uoid() << false << false;
}

void KMComposerWinTest::testSigning()
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
    toggleSigning(composer);
}

void KMComposerWinTest::toggleSigning(KMail::Composer* composer)
{
    QFETCH(bool, sign);
    QFETCH(bool, sign_possible);

    auto signAction = composer->findChild<KToggleAction *>(QStringLiteral("sign_message"));
    auto signature = composer->findChild<QLabel *>(QStringLiteral("signatureindicator"));
    QVERIFY(signature);
    QVERIFY(signAction);

    QCOMPARE(signature->isVisible(), sign);
    QCOMPARE(signAction->isChecked(), sign);
    QCOMPARE(signAction->isEnabled(), sign_possible);

    if (!sign_possible) {
        return;
    }

    {
        signAction->trigger();
        QCOMPARE(signAction->isChecked(), !sign);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(signature->isVisible(), !sign);
    }
    {
        signAction->trigger();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(signature->isVisible(), sign);
        QCOMPARE(signAction->isChecked(), sign);
    }
    composer->setModified(false);
}

void KMComposerWinTest::testEncryption_data()
{
    const auto im = mKernel->identityManager();
    const QString recipient(QStringLiteral("Friends <friends@kde.example>"));

    QTest::addColumn<uint>("uoid");
    QTest::addColumn<QString>("recipient");
    QTest::addColumn<bool>("encrypt");
    QTest::addColumn<bool>("encrypt_possible");

    QTest::newRow("nothing") << im->identityForAddress(QStringLiteral("nothing@test.example")).uoid() << recipient << false << true;
    QTest::newRow("signonly") << im->identityForAddress(QStringLiteral("signonly@test.example")).uoid() << recipient << false << true;
    QTest::newRow("encryptonly") << im->identityForAddress(QStringLiteral("encryptonly@test.example")).uoid() << recipient << true << true;
    QTest::newRow("signandencrypt") << im->identityForAddress(QStringLiteral("signandencrypt@test.example")).uoid() << recipient << true << true;
    QTest::newRow("autocrypt") << im->identityForAddress(QStringLiteral("autocrypt@test.example")).uoid() << "Autocrypt <friends@autocrypt.example>" << true << true;
    QTest::newRow("wrongkey") << im->identityForAddress(QStringLiteral("wrongkey@test.example")).uoid() << recipient << false << false;
    QTest::newRow("wrongkeysign") << im->identityForAddress(QStringLiteral("wrongkeysign@test.example")).uoid() << recipient << true << true;
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
    toggleEncryption(composer);
}

void KMComposerWinTest::toggleEncryption(KMail::Composer* composer)
{
    QFETCH(bool, encrypt);
    QFETCH(bool, encrypt_possible);

    auto encryptAction = composer->findChild<KToggleAction *>(QStringLiteral("encrypt_message"));
    auto encryption = composer->findChild<QLabel *>(QStringLiteral("encryptionindicator"));
    QVERIFY(encryption);
    QVERIFY(encryptAction);

    QCOMPARE(encryption->isVisible(), encrypt);
    QCOMPARE(encryptAction->isChecked(), encrypt);
    QCOMPARE(encryptAction->isEnabled(), encrypt_possible);

    if (!encrypt_possible) {
        return;
    }

    {
        encryptAction->trigger();
        QCOMPARE(encryptAction->isChecked(), !encrypt);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(encryption->isVisible(), !encrypt);
    }
    {
        encryptAction->trigger();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(encryption->isVisible(), encrypt);
        QCOMPARE(encryptAction->isChecked(), encrypt);
    }
    composer->setModified(false);
}

void KMComposerWinTest::testNearExpiryWarningIdentity_data()
{
    const auto im = mKernel->identityManager();

    QTest::addColumn<uint>("uoid");
    QTest::addColumn<bool>("visible");
    QTest::addColumn<QString>("searchpattern");

    QTest::newRow("nothing") << im->identityForAddress(QStringLiteral("nothing@test.example")).uoid() << false << QString();
    QTest::newRow("signonly") << im->identityForAddress(QStringLiteral("signonly@test.example")).uoid() << false << QString();
    QTest::newRow("encryptonly") << im->identityForAddress(QStringLiteral("encryptonly@test.example")).uoid() << false << QString();
    QTest::newRow("signandencrypt") << im->identityForAddress(QStringLiteral("signandencrypt@test.example")).uoid() << false << QString();
    QTest::newRow("autocrypt") << im->identityForAddress(QStringLiteral("autocrypt@test.example")).uoid() << false << QString();

    auto ident = im->identityForAddress(QStringLiteral("wrongkey@test.example"));
    QTest::newRow("wrongkey") << ident.uoid() << true << QString::fromUtf8(ident.pgpEncryptionKey());

    ident = im->identityForAddress(QStringLiteral("wrongkeysign@test.example"));
    QTest::newRow("wrongkeysign") << ident.uoid() << true << QString::fromUtf8(ident.pgpSigningKey());
}

void KMComposerWinTest::testNearExpiryWarningIdentity()
{
    QFETCH(uint, uoid);
    QFETCH(bool, visible);
    QFETCH(QString, searchpattern);

    const auto ident = mKernel->identityManager()->identityForUoid(uoid);
    const auto msg(createItem(ident));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    auto nearExpiryWarning = composer->findChild<NearExpiryWarning *>();
    QVERIFY(nearExpiryWarning);
    QCOMPARE(nearExpiryWarning->isVisible(), visible);

    if (visible) {
        QCOMPARE(nearExpiryWarning->text().count(QStringLiteral("<p>")), 1);
        QVERIFY(nearExpiryWarning->text().contains(searchpattern));
    }
}

void KMComposerWinTest::testChangeIdentity()
{
    const auto im = mKernel->identityManager();

    auto ident = im->identityForAddress(QStringLiteral("signonly@test.example"));
    const auto msg(createItem(ident));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    auto encryption = composer->findChild<QLabel *>(QStringLiteral("encryptionindicator"));
    auto signature = composer->findChild<QLabel *>(QStringLiteral("signatureindicator"));
    auto encryptAction = composer->findChild<KToggleAction *>(QStringLiteral("encrypt_message"));
    auto signAction = composer->findChild<KToggleAction *>(QStringLiteral("sign_message"));
    auto identCombo = composer->findChild<KIdentityManagement::IdentityCombo *>(QStringLiteral("identitycombo"));
    QVERIFY(encryption);
    QVERIFY(signature);
    QVERIFY(identCombo);
    QVERIFY(encryptAction);
    QVERIFY(signAction);
    QCOMPARE(encryption->isVisible(), false);
    QCOMPARE(signature->isVisible(), true);
    QCOMPARE(signAction->isEnabled(), true);
    QCOMPARE(encryptAction->isEnabled(), true);

    {
        ident = im->identityForAddress(QStringLiteral("signandencrypt@test.example"));
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(encryption->isVisible(), true);
        QCOMPARE(signature->isVisible(), true);
        QCOMPARE(signAction->isEnabled(), true);
        QCOMPARE(encryptAction->isEnabled(), true);
    }

    {
        ident = im->identityForAddress(QStringLiteral("nothing@test.example"));
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(encryption->isVisible(), false);
        QCOMPARE(signature->isVisible(), false);
        QCOMPARE(signAction->isEnabled(), true);
        QCOMPARE(encryptAction->isEnabled(), true);
    }

    {
        ident = im->identityForAddress(QStringLiteral("wrongkey@test.example"));
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(encryption->isVisible(), false);
        QCOMPARE(signature->isVisible(), false);
        QCOMPARE(signAction->isEnabled(), false);
        QCOMPARE(encryptAction->isEnabled(), false);
    }


    {
        ident = im->identityForAddress(QStringLiteral("signonly@test.example"));
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(encryption->isVisible(), false);
        QCOMPARE(signature->isVisible(), true);
        QCOMPARE(signAction->isEnabled(), true);
        QCOMPARE(encryptAction->isEnabled(), true);
    }
}


void KMComposerWinTest::testChangeIdentityNearExpiryWarning()
{
    const auto im = mKernel->identityManager();
    auto ident = im->identityForAddress(QStringLiteral("signonly@test.example"));
    const auto msg(createItem(ident));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    auto identCombo = composer->findChild<KIdentityManagement::IdentityCombo *>(QStringLiteral("identitycombo"));
    auto nearExpiryWarning = composer->findChild<NearExpiryWarning *>();
    QVERIFY(nearExpiryWarning);
    QVERIFY(identCombo);
    QCOMPARE(nearExpiryWarning->isVisible(), false);

    {
        ident = im->identityForAddress(QStringLiteral("wrongkey@test.example"));
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(nearExpiryWarning->isVisible(), true);
        QCOMPARE(nearExpiryWarning->text().count(QStringLiteral("<p>")), 1);
        QVERIFY(nearExpiryWarning->text().contains(QString::fromUtf8(ident.pgpEncryptionKey())));
    }
    {
        ident = im->identityForAddress(QStringLiteral("wrongkeysign@test.example"));
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(nearExpiryWarning->isVisible(), true);
        QCOMPARE(nearExpiryWarning->text().count(QStringLiteral("<p>")), 1);
        QVERIFY(!nearExpiryWarning->text().contains(QString::fromUtf8(ident.pgpEncryptionKey())));
        QVERIFY(nearExpiryWarning->text().contains(QString::fromUtf8(ident.pgpSigningKey())));
    }
    {
        ident = im->identityForAddress(QStringLiteral("signandencrypt@test.example"));
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(nearExpiryWarning->isVisible(), false);
    }
}
