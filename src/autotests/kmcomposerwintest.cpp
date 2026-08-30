/*
  SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmcomposerwintest.h"

#include "editor/warningwidgets/nearexpirywarning.h"
#include "kmkernel.h"

#include <MessageComposer/ComposerJob>
#include <MessageComposer/RecipientsEditor>

#include <gpgme++/key.h>
#include <gpgme++/tofuinfo.h>
#include <gpgme.h>

#include <KEmailAddress>
#include <KToggleAction>

#include <KIdentityManagementCore/Identity>
#include <KIdentityManagementCore/IdentityManager>
#include <KIdentityManagementWidgets/IdentityCombo>

#include <Libkleo/Enum>
#include <Libkleo/KeyCache>

#include <KMime/Message>

#include <QEventLoop>
#include <QLabel>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QTimer>
#include <QToolButton>

#include <chrono>

using namespace Qt::Literals::StringLiterals;
using namespace std::chrono_literals;

Q_DECLARE_METATYPE(Kleo::TrustLevel)

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

GpgME::Key createTestKey(QByteArray uid,
                         QByteArray fingerprint,
                         time_t expires = 0,
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
    key->expired = expires ? expires < std::time(nullptr) : 0;
    key->disabled = 0;
    key->can_encrypt = int(usage == Kleo::KeyCache::KeyUsage::AnyUsage || usage == Kleo::KeyCache::KeyUsage::Encrypt);
    key->can_sign = int(usage == Kleo::KeyCache::KeyUsage::AnyUsage || usage == Kleo::KeyCache::KeyUsage::Sign);
    key->secret = 1;
    key->uids->validity = mapValidity(validity);
    gpgme_subkey_t subkey;
    subkey = (gpgme_subkey_t)calloc(1, sizeof *subkey);
    key->subkeys = subkey;
    key->_last_subkey = subkey;
    subkey->timestamp = 123456789;
    subkey->revoked = 0;
    subkey->expired = 0;
    subkey->expires = expires, subkey->disabled = 0;
    subkey->keyid = strdup(fingerprint.constData());
    subkey->can_encrypt = int(usage == Kleo::KeyCache::KeyUsage::AnyUsage || usage == Kleo::KeyCache::KeyUsage::Encrypt);
    subkey->can_sign = int(usage == Kleo::KeyCache::KeyUsage::AnyUsage || usage == Kleo::KeyCache::KeyUsage::Sign);

    return GpgME::Key(key, false);
}

void killAgent()
{
    QProcess proc;
    proc.setProgram(u"gpg-connect-agent"_s);
    proc.start();
    proc.waitForStarted();
    proc.write("KILLAGENT\n");
    proc.write("BYE\n");
    proc.closeWriteChannel();
    proc.waitForFinished();
}

std::shared_ptr<KMime::Message> createItem(const KIdentityManagementCore::Identity &ident, const QByteArray &recipient = "Friends <friends@kde.example>"_ba)
{
    QByteArray data
        = "From: Konqui <konqui@kde.example>\n"
          "To: "_ba + recipient +"\n"
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

    auto msgPtr = std::shared_ptr<KMime::Message>(new KMime::Message());
    msgPtr->setContent(data);
    msgPtr->parse();
    return msgPtr;
}

KMComposerWinTest::KMComposerWinTest(QObject *parent)
    : QObject(parent)
{
    QStandardPaths::setTestModeEnabled(true);
    qputenv("KDEHOME", QFile::encodeName(QDir::homePath() + "/.qttest"_L1).constData());

    const QDir genericDataLocation(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    QDir gnupgHomeData(QLatin1StringView(TEST_DATA_DIR) + u"/gnupg"_s);
    for (const auto &fileInfo : gnupgHomeData.entryInfoList(QDir::Files)) {
        QVERIFY(QFile(fileInfo.filePath()).copy(gnupgDir.path() + u"/"_s + fileInfo.fileName()));
    }
    qputenv("GNUPGHOME", gnupgDir.path().toUtf8());

    // We need to initialize KMKernel after modifying the environment variables, otherwise we would read the wrong configs
    mKernel = new KMKernel(parent);
}

KMComposerWinTest::~KMComposerWinTest()
{
    delete mKernel;
}

void KMComposerWinTest::init()
{
    autocryptDir.mkpath(u"."_s);
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
    autocryptDir = QDir(genericDataLocation.filePath(u"autocrypt"_s));

    resetIdentities();

    // Keep a KeyCache reference. Make sure that the cache stays hot between the tests.
    mKernel->init();

    Kleo::KeyCache::mutableInstance()->setKeys(
        {createTestKey("encrypt <encrypt@test.example>", "345678901"),
         createTestKey("wrongkey <wrongkey@test.example>", "22222222"),
         createTestKey("friends@kde.example", "1"),
         createTestKey("nearexpiry@test.example", "2", std::time(nullptr) + 2 * 86300),
         createTestKey("expired@test.example", "3", std::time(nullptr) - 2 * 86300),
         createTestKey("level0 <level0@tofu.example>",
                       "023DFCA4424EA644B174AD14C6F20F3A31F563CF",
                       0,
                       GpgME::UnknownProtocol,
                       Kleo::KeyCache::KeyUsage::AnyUsage,
                       GpgME::UserID::Marginal),
         createTestKey("level1 <level1@tofu.example>",
                       "B571A07AF0CE7BC645C98DDA3D081FB0E3ADDA15",
                       0,
                       GpgME::UnknownProtocol,
                       Kleo::KeyCache::KeyUsage::AnyUsage,
                       GpgME::UserID::Marginal),
         createTestKey("level2 <level2@tofu.example>",
                       "C6359BFCBC418D756D52B9D095FC1B2659BD3F38",
                       0,
                       GpgME::UnknownProtocol,
                       Kleo::KeyCache::KeyUsage::AnyUsage,
                       GpgME::UserID::Marginal),
         createTestKey("level3 <level3@tofu.example>",
                       "5FB1048166DF03F6AA15D57762EF1B9108E04EC9",
                       0,
                       GpgME::UnknownProtocol,
                       Kleo::KeyCache::KeyUsage::AnyUsage,
                       GpgME::UserID::Marginal),
         createTestKey("bad <bad@tofu.example>",
                       "EF86281B34F05348610D5A6F3579DB31A5384419",
                       0,
                       GpgME::UnknownProtocol,
                       Kleo::KeyCache::KeyUsage::AnyUsage,
                       GpgME::UserID::Marginal),
         createTestKey("validity0@kde.example", "20", 0, GpgME::UnknownProtocol, Kleo::KeyCache::KeyUsage::AnyUsage, GpgME::UserID::Unknown),
         createTestKey("validity1@kde.example", "21", 0, GpgME::UnknownProtocol, Kleo::KeyCache::KeyUsage::AnyUsage, GpgME::UserID::Undefined),
         createTestKey("validity2@kde.example", "22", 0, GpgME::UnknownProtocol, Kleo::KeyCache::KeyUsage::AnyUsage, GpgME::UserID::Never),
         createTestKey("validity3@kde.example", "23", 0, GpgME::UnknownProtocol, Kleo::KeyCache::KeyUsage::AnyUsage, GpgME::UserID::Marginal),
         createTestKey("validity4@kde.example", "24", 0, GpgME::UnknownProtocol, Kleo::KeyCache::KeyUsage::AnyUsage, GpgME::UserID::Full),
         createTestKey("validity5@kde.example", "25", 0, GpgME::UnknownProtocol, Kleo::KeyCache::KeyUsage::AnyUsage, GpgME::UserID::Ultimate)});
}

void KMComposerWinTest::cleanupTestCase()
{
    QVERIFY(mKernel->identityManager()->removeIdentity(u"nearexpiry"_s));
    QVERIFY(mKernel->identityManager()->removeIdentity(u"expired"_s));
    QVERIFY(mKernel->identityManager()->removeIdentity(u"wrongkeysign"_s));
    QVERIFY(mKernel->identityManager()->removeIdentity(u"wrongkey"_s));
    QVERIFY(mKernel->identityManager()->removeIdentity(u"autocrypt"_s));
    QVERIFY(mKernel->identityManager()->removeIdentity(u"signonly"_s));
    QVERIFY(mKernel->identityManager()->removeIdentity(u"encryptonly"_s));
    QVERIFY(mKernel->identityManager()->removeIdentity(u"signandencrypt"_s));
    QVERIFY(mKernel->identityManager()->removeIdentity(u"nothing"_s));
    mKernel->identityManager()->commit();
    killAgent();
}

void KMComposerWinTest::resetIdentities()
{
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(u"nothing"_s);
        i.setPrimaryEmailAddress(u"nothing@test.example"_s);
        i.setPGPSigningKey("345678901");
        i.setPGPEncryptionKey("345678901");
        i.setPgpAutoSign(false);
        i.setPgpAutoEncrypt(false);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(u"signonly"_s);
        i.setPrimaryEmailAddress(u"signonly@test.example"_s);
        i.setPGPSigningKey("345678901");
        i.setPGPEncryptionKey("345678901");
        i.setPgpAutoSign(true);
        i.setPgpAutoEncrypt(false);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(u"encryptonly"_s);
        i.setPrimaryEmailAddress(u"encryptonly@test.example"_s);
        i.setPGPSigningKey("345678901");
        i.setPGPEncryptionKey("345678901");
        i.setPgpAutoSign(false);
        i.setPgpAutoEncrypt(true);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(u"signandencrypt"_s);
        i.setPrimaryEmailAddress(u"signandencrypt@test.example"_s);
        i.setPGPSigningKey("345678901");
        i.setPGPEncryptionKey("345678901");
        i.setPgpAutoSign(true);
        i.setPgpAutoEncrypt(true);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(u"autocrypt"_s);
        i.setPrimaryEmailAddress(u"autocrypt@test.example"_s);
        i.setPGPSigningKey("345678901");
        i.setPGPEncryptionKey("345678901");
        i.setPgpAutoSign(true);
        i.setPgpAutoEncrypt(true);
        i.setAutocryptEnabled(true);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(u"wrongkey"_s);
        i.setPrimaryEmailAddress(u"wrongkey@test.example"_s);
        i.setPGPSigningKey("1111111");
        i.setPGPEncryptionKey("1111111");
        i.setPgpAutoSign(true);
        i.setPgpAutoEncrypt(true);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(u"wrongkeysign"_s);
        i.setPrimaryEmailAddress(u"wrongkeysign@test.example"_s);
        i.setPGPSigningKey("1111111");
        i.setPGPEncryptionKey("22222222");
        i.setPgpAutoSign(true);
        i.setPgpAutoEncrypt(true);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(u"nearexpiry"_s);
        i.setPrimaryEmailAddress(u"nearexpiry@test.example"_s);
        i.setPGPSigningKey("2");
        i.setPGPEncryptionKey("2");
        i.setPgpAutoSign(true);
        i.setPgpAutoEncrypt(true);
    }
    {
        auto &i = mKernel->identityManager()->modifyIdentityForName(u"expired"_s);
        i.setPrimaryEmailAddress(u"expired@test.example"_s);
        i.setPGPSigningKey("3");
        i.setPGPEncryptionKey("3");
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

    QTest::newRow("nothing") << im->identityForAddress(u"nothing@test.example"_s).uoid() << false << true;
    QTest::newRow("signonly") << im->identityForAddress(u"signonly@test.example"_s).uoid() << true << true;
    QTest::newRow("encryptonly") << im->identityForAddress(u"encryptonly@test.example"_s).uoid() << false << true;
    QTest::newRow("signandencrypt") << im->identityForAddress(u"signandencrypt@test.example"_s).uoid() << true << true;
    QTest::newRow("autocrypt") << im->identityForAddress(u"autocrypt@test.example"_s).uoid() << true << true;
    QTest::newRow("wrongkey") << im->identityForAddress(u"wrongkey@test.example"_s).uoid() << false << false;
    QTest::newRow("wrongkeysign") << im->identityForAddress(u"wrongkeysign@test.example"_s).uoid() << false << false;
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
    auto signature = composer->findChild<QLabel *>(u"signatureindicator"_s);
    QVERIFY(signature);
    QCOMPARE(signature->isVisible(), sign);
    toggleSigning(composer);
}

void KMComposerWinTest::toggleSigning(KMail::Composer *composer)
{
    QFETCH(bool, sign);
    QFETCH(bool, sign_possible);

    auto signAction = composer->findChild<KToggleAction *>(u"sign_message"_s);
    auto signature = composer->findChild<QLabel *>(u"signatureindicator"_s);
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
    const QString recipient(u"Friends <friends@kde.example>"_s);

    QTest::addColumn<uint>("uoid");
    QTest::addColumn<QString>("recipient");
    QTest::addColumn<bool>("encrypt");
    QTest::addColumn<bool>("encrypt_possible");

    QTest::newRow("nothing") << im->identityForAddress(u"nothing@test.example"_s).uoid() << recipient << false << true;
    QTest::newRow("signonly") << im->identityForAddress(u"signonly@test.example"_s).uoid() << recipient << false << true;
    QTest::newRow("encryptonly") << im->identityForAddress(u"encryptonly@test.example"_s).uoid() << recipient << true << true;
    QTest::newRow("signandencrypt") << im->identityForAddress(u"signandencrypt@test.example"_s).uoid() << recipient << true << true;
    QTest::newRow("autocrypt") << im->identityForAddress(u"autocrypt@test.example"_s).uoid() << "Autocrypt <friends@autocrypt.example>" << true << true;
    QTest::newRow("wrongkey") << im->identityForAddress(u"wrongkey@test.example"_s).uoid() << recipient << false << false;
    QTest::newRow("wrongkeysign") << im->identityForAddress(u"wrongkeysign@test.example"_s).uoid() << recipient << true << true;
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

    QFile file1(QLatin1StringView(TEST_DATA_DIR) + u"/autocrypt/friends%40kde.org.json"_s);
    QVERIFY(file1.copy(autocryptDir.filePath(addrSpec.replace(u"@"_s, u"%40"_s) + u".json"_s)));

    const auto ident = mKernel->identityManager()->identityForUoid(uoid);
    const auto msg(createItem(ident, recipient.toUtf8()));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));

    // We need to wait till KeyCache is populated, otherwise we will get wrong results
    if (const auto instance = Kleo::KeyCache::instance(); !instance->initialized()) {
        QEventLoop loop;
        connect(instance.get(), &Kleo::KeyCache::keyListingDone, this, [&loop]() {
            loop.quit();
        });
        loop.exec();
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    auto encryption = composer->findChild<QLabel *>(u"encryptionindicator"_s);
    QVERIFY(encryption);
    QCOMPARE(encryption->isVisible(), encrypt);
    toggleEncryption(composer);
}

void KMComposerWinTest::toggleEncryption(KMail::Composer *composer)
{
    QFETCH(bool, encrypt);
    QFETCH(bool, encrypt_possible);

    auto encryptAction = composer->findChild<KToggleAction *>(u"encrypt_message"_s);
    auto encryption = composer->findChild<QLabel *>(u"encryptionindicator"_s);
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

    QTest::newRow("nothing") << im->identityForAddress(u"nothing@test.example"_s).uoid() << false << QString();
    QTest::newRow("signonly") << im->identityForAddress(u"signonly@test.example"_s).uoid() << false << QString();
    QTest::newRow("encryptonly") << im->identityForAddress(u"encryptonly@test.example"_s).uoid() << false << QString();
    QTest::newRow("signandencrypt") << im->identityForAddress(u"signandencrypt@test.example"_s).uoid() << false << QString();
    QTest::newRow("autocrypt") << im->identityForAddress(u"autocrypt@test.example"_s).uoid() << false << QString();

    auto ident = im->identityForAddress(u"wrongkey@test.example"_s);
    QTest::newRow("wrongkey") << ident.uoid() << true << QString::fromUtf8(ident.pgpEncryptionKey());

    ident = im->identityForAddress(u"wrongkeysign@test.example"_s);
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
        QCOMPARE(nearExpiryWarning->text().count(u"<p>"_s), 1);
        QVERIFY(nearExpiryWarning->text().contains(searchpattern));
    }
}

void KMComposerWinTest::testChangeIdentity()
{
    const auto im = mKernel->identityManager();

    auto ident = im->identityForAddress(u"signonly@test.example"_s);
    const auto msg(createItem(ident));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    auto encryption = composer->findChild<QLabel *>(u"encryptionindicator"_s);
    auto signature = composer->findChild<QLabel *>(u"signatureindicator"_s);
    auto encryptAction = composer->findChild<KToggleAction *>(u"encrypt_message"_s);
    auto signAction = composer->findChild<KToggleAction *>(u"sign_message"_s);
    auto identCombo = composer->findChild<KIdentityManagementWidgets::IdentityCombo *>(u"identitycombo"_s);
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
        ident = im->identityForAddress(u"signandencrypt@test.example"_s);
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
        ident = im->identityForAddress(u"nothing@test.example"_s);
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
        ident = im->identityForAddress(u"wrongkey@test.example"_s);
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
        ident = im->identityForAddress(u"signonly@test.example"_s);
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
    auto ident = im->identityForAddress(u"signonly@test.example"_s);
    const auto msg(createItem(ident));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    auto identCombo = composer->findChild<KIdentityManagementWidgets::IdentityCombo *>(u"identitycombo"_s);
    auto nearExpiryWarning = composer->findChild<NearExpiryWarning *>();
    QVERIFY(nearExpiryWarning);
    QVERIFY(identCombo);
    QCOMPARE(nearExpiryWarning->isVisible(), false);

    {
        ident = im->identityForAddress(u"wrongkey@test.example"_s);
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(nearExpiryWarning->isVisible(), true);
        QCOMPARE(nearExpiryWarning->text().count(u"<p>"_s), 1);
        QVERIFY(nearExpiryWarning->text().contains(QString::fromUtf8(ident.pgpEncryptionKey())));
    }
    {
        ident = im->identityForAddress(u"wrongkeysign@test.example"_s);
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(nearExpiryWarning->isVisible(), true);
        QCOMPARE(nearExpiryWarning->text().count(u"<p>"_s), 1);
        QVERIFY(!nearExpiryWarning->text().contains(QString::fromUtf8(ident.pgpEncryptionKey())));
        QVERIFY(nearExpiryWarning->text().contains(QString::fromUtf8(ident.pgpSigningKey())));
    }
    {
        ident = im->identityForAddress(u"signandencrypt@test.example"_s);
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(nearExpiryWarning->isVisible(), false);
    }
}

void KMComposerWinTest::testOwnExpiry()
{
    const auto im = mKernel->identityManager();
    auto ident = im->identityForAddress(u"nearexpiry@test.example"_s);
    const auto msg(createItem(ident));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    auto identCombo = composer->findChild<KIdentityManagementWidgets::IdentityCombo *>(u"identitycombo"_s);
    auto nearExpiryWarning = composer->findChild<NearExpiryWarning *>();
    QVERIFY(nearExpiryWarning);
    QVERIFY(identCombo);
    QCOMPARE(nearExpiryWarning->isVisible(), true);
    QCOMPARE(nearExpiryWarning->text().count(u"<p>"_s), 1);
    QVERIFY(nearExpiryWarning->text().contains(QString::fromUtf8("0x" + ident.pgpEncryptionKey())));
    QVERIFY(nearExpiryWarning->text().contains(QString::fromUtf8("expires in 2 days.")));

    {
        ident = im->identityForAddress(u"signandencrypt@test.example"_s);
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(nearExpiryWarning->isVisible(), false);
    }
    {
        ident = im->identityForAddress(u"expired@test.example"_s);
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(nearExpiryWarning->isVisible(), true);
        QCOMPARE(nearExpiryWarning->text().count(u"<p>"_s), 1);
        QVERIFY(nearExpiryWarning->text().contains(QString::fromUtf8("0x" + ident.pgpEncryptionKey())));
        QVERIFY(nearExpiryWarning->text().contains(QString::fromUtf8("expired 2 days ago.")));
    }
    {
        ident = im->identityForAddress(u"nearexpiry@test.example"_s);
        identCombo->setCurrentIdentity(ident);
        // We need a small sleep so that identity change can take place
        QEventLoop loop;
        QTimer::singleShot(50ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(nearExpiryWarning->isVisible(), true);
        QCOMPARE(nearExpiryWarning->text().count(u"<p>"_s), 1);
        QVERIFY(nearExpiryWarning->text().contains(QString::fromUtf8("0x" + ident.pgpEncryptionKey())));
        QVERIFY(nearExpiryWarning->text().contains(QString::fromUtf8("expires in 2 days.")));
    }
}

void KMComposerWinTest::testRecipientExpiry()
{
    const auto im = mKernel->identityManager();
    auto ident = im->identityForAddress(u"signandencrypt@test.example"_s);
    const auto msg(createItem(ident, "nearexpiry@test.example"));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    // We need to wait till KeyCache is populated, otherwise we will get wrong results
    if (const auto instance = Kleo::KeyCache::instance(); !instance->initialized()) {
        QEventLoop loop;
        connect(instance.get(), &Kleo::KeyCache::keyListingDone, this, [&loop]() {
            loop.quit();
        });
        loop.exec();
    }

    QVERIFY(QTest::qWaitForWindowExposed(composer));
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    auto encryption = composer->findChild<QLabel *>(u"encryptionindicator"_s);
    auto nearExpiryWarning = composer->findChild<NearExpiryWarning *>();
    auto recipientsEditor = composer->findChild<MessageComposer::RecipientsEditor *>();
    QVERIFY(encryption);
    QVERIFY(nearExpiryWarning);
    QVERIFY(recipientsEditor);

    const auto lines = recipientsEditor->lines();
    const auto line = qobject_cast<MessageComposer::RecipientLineNG *>(lines.constFirst());
    const auto lineEdit = line->findChild<MessageComposer::RecipientLineEdit *>();
    QVERIFY(lineEdit);
    const auto toolButton = lineEdit->findChild<QToolButton *>();
    QVERIFY(toolButton);
    auto recipient = line->recipient();
    qDebug() << toolButton->toolTip();

    {
        const auto key = Kleo::KeyCache::instance()->findByEMailAddress("nearexpiry@test.example").at(0);
        QCOMPARE(recipient->key().primaryFingerprint(), key.primaryFingerprint());
        QVERIFY(toolButton->toolTip().contains(QString::fromUtf8("expires in 2 days.")));
        QCOMPARE(toolButton->icon().name(), u"emblem-warning"_s);
        QCOMPARE(encryption->isVisible(), true);
        QCOMPARE(nearExpiryWarning->isVisible(), false);
    }
    {
        lineEdit->clear();
        lineEdit->setFocus();
        // Simulate typing email address
        QTest::keyClicks(lineEdit, u"Friends <friends@kde.example>"_s, Qt::NoModifier, 10);

        // We need a small sleep, we need to  wait till the KeyResolver is run.
        QEventLoop loop;
        QTimer::singleShot(550ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        composer->setModified(false);
        const auto key = Kleo::KeyCache::instance()->findByEMailAddress("friends@kde.example").at(0);
        QVERIFY(toolButton->toolTip().contains(u"The encryption key is fully trusted."_s));
        QCOMPARE(toolButton->icon().name(), u"emblem-success"_s);
        QCOMPARE(recipient->key().primaryFingerprint(), key.primaryFingerprint());
        QCOMPARE(nearExpiryWarning->isVisible(), false);
        QCOMPARE(encryption->isVisible(), true);
    }
    {
        lineEdit->clear();
        lineEdit->setFocus();
        // Simulate typing email address
        QTest::keyClicks(lineEdit, u"\"Expired Key\" <expired@test.example>"_s, Qt::NoModifier, 10);

        // We need a small sleep, we need to  wait till the KeyResolver is run.
        QEventLoop loop;
        QTimer::singleShot(550ms, &loop, SLOT(quit()));
        loop.exec();
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        composer->setModified(false);
        const auto key = Kleo::KeyCache::instance()->findByEMailAddress("expired@test.example").at(0);
        qDebug() << toolButton->toolTip();
        QCOMPARE(toolButton->isVisible(), false);
        QCOMPARE(recipient->key().primaryFingerprint(), key.primaryFingerprint());
        QCOMPARE(nearExpiryWarning->isVisible(), false);
        QCOMPARE(encryption->isVisible(), false);
    }

    {
        auto encryptAction = composer->findChild<KToggleAction *>(u"encrypt_message"_s);
        QVERIFY(encryptAction);

        QCOMPARE(encryptAction->isChecked(), false);

        encryptAction->trigger();
        QCOMPARE(encryptAction->isChecked(), true);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(encryption->isVisible(), true);
        QCOMPARE(toolButton->isVisible(), true);
        QCOMPARE(toolButton->icon().name(), u"emblem-error"_s);
        QVERIFY(toolButton->toolTip().contains(QString::fromUtf8("expired 2 days ago.")));
        QEventLoop loop;
        QTimer::singleShot(1550ms, &loop, SLOT(quit()));
        loop.exec();
    }
}

void KMComposerWinTest::testRecipientAnnotation_data()
{
    QTest::addColumn<QString>("mailaddress");
    QTest::addColumn<Kleo::TrustLevel>("trustlevel");
    QTest::addColumn<QString>("searchpattern");
    QTest::addColumn<QString>("iconname");
    QTest::addColumn<bool>("encrypt");

    QTest::newRow("friends@kde.example") << u"friends@kde.example"_s << Kleo::Level3 << QString() << u"emblem-success"_s << true;
    QTest::newRow("validity0@kde.example") << u"validity0@kde.example"_s << Kleo::Level0 << u"It hasn't enough validity."_s << u"emblem-error"_s << false;
    QTest::newRow("validity1@kde.example") << u"validity1@kde.example"_s << Kleo::Level0 << u"It hasn't enough validity."_s << u"emblem-error"_s << false;
    QTest::newRow("validity2@kde.example") << u"validity2@kde.example"_s << Kleo::Level0 << u"It hasn't enough validity."_s << u"emblem-error"_s << false;
    QTest::newRow("validity3@kde.example") << u"validity3@kde.example"_s << Kleo::Level2 << u"You can sign the key, if you communicated"_s
                                           << u"emblem-success"_s << true;
    QTest::newRow("validity4@kde.example") << u"validity4@kde.example"_s << Kleo::Level3 << QString() << u"emblem-success"_s << true;
    QTest::newRow("validity5@kde.example") << u"validity5@kde.example"_s << Kleo::Level4 << QString() << u"emblem-success"_s << true;
    QTest::newRow("level0@tofu.example") << u"level0@tofu.example"_s << Kleo::Level0 << u"By using the key will be trusted more."_s << u"emblem-warning"_s
                                         << true;
    QTest::newRow("level1@tofu.example") << u"level1@tofu.example"_s << Kleo::Level1 << u"By using the key will be trusted more."_s << u"emblem-success"_s
                                         << true;
    QTest::newRow("level2@tofu.example") << u"level2@tofu.example"_s << Kleo::Level2 << u"By using the key will be trusted more."_s << u"emblem-success"_s
                                         << true;
    QTest::newRow("level3@tofu.example") << u"level3@tofu.example"_s << Kleo::Level2 << u"By using the key will be trusted more."_s << u"emblem-success"_s
                                         << true;
    QTest::newRow("bad@tofu.example") << u"bad@tofu.example"_s << Kleo::Level0 << u"The key is marked as bad."_s << u"emblem-error"_s << false;
}

void KMComposerWinTest::testRecipientAnnotation()
{
    QFETCH(QString, mailaddress);
    QFETCH(Kleo::TrustLevel, trustlevel);
    QFETCH(QString, searchpattern);
    QFETCH(QString, iconname);
    QFETCH(bool, encrypt);

    const auto im = mKernel->identityManager();
    auto ident = im->identityForAddress(u"signandencrypt@test.example"_s);
    const auto msg(createItem(ident, mailaddress.toUtf8()));

    auto composer = KMail::makeComposer(msg);
    composer->show();
    QVERIFY(QTest::qWaitForWindowExposed(composer));
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    auto encryption = composer->findChild<QLabel *>(u"encryptionindicator"_s);
    auto nearExpiryWarning = composer->findChild<NearExpiryWarning *>();
    auto recipientsEditor = composer->findChild<MessageComposer::RecipientsEditor *>();
    QVERIFY(encryption);
    QVERIFY(nearExpiryWarning);
    QVERIFY(recipientsEditor);

    const auto lines = recipientsEditor->lines();
    const auto line = qobject_cast<MessageComposer::RecipientLineNG *>(lines.constFirst());
    const auto lineEdit = line->findChild<MessageComposer::RecipientLineEdit *>();
    QVERIFY(lineEdit);
    const auto toolButton = lineEdit->findChild<QToolButton *>();
    QVERIFY(toolButton);
    auto recipient = line->recipient();

    const auto key = Kleo::KeyCache::instance()->findByEMailAddress(mailaddress.toUtf8().constData()).at(0);
    QCOMPARE(recipient->key().primaryFingerprint(), key.primaryFingerprint());
    if (!encrypt) {
        QCOMPARE(toolButton->isVisible(), false);
    }
    QCOMPARE(encryption->isVisible(), encrypt);
    QCOMPARE(nearExpiryWarning->isVisible(), false);

    // Enable encryption to check if we see error text/icons
    if (!encrypt) {
        auto encryptAction = composer->findChild<KToggleAction *>(u"encrypt_message"_s);
        QVERIFY(encryptAction);

        QCOMPARE(encryptAction->isChecked(), encrypt);

        encryptAction->trigger();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCOMPARE(encryptAction->isChecked(), !encrypt);
        QCOMPARE(encryption->isVisible(), !encrypt);
    }

    QCOMPARE(toolButton->isVisible(), true);
    QCOMPARE(toolButton->icon().name(), iconname);
    qDebug() << toolButton->toolTip();
    switch (trustlevel) {
    case Kleo::Level0:
        QVERIFY(toolButton->toolTip().contains(u"The encryption key is not trusted"_s));
        break;
    case Kleo::Level1:
    case Kleo::Level2:
        QVERIFY(toolButton->toolTip().contains(u"The encryption key is only marginally trusted"_s));
        break;
    case Kleo::Level3:
        QVERIFY(toolButton->toolTip().contains(u"The encryption key is fully trusted"_s));
        break;
    case Kleo::Level4:
        QVERIFY(toolButton->toolTip().contains(u"The encryption key is ultimately trusted"_s));
        break;
    default:
        Q_UNREACHABLE();
    }
    if (!searchpattern.isEmpty()) {
        QVERIFY(toolButton->toolTip().contains(searchpattern));
    }
}

void KMComposerWinTest::checkKeys()
{
    {
        // Somehow the composer triggers something inside gnupg to be able to pass the for tofu info successfully.
        const auto im = mKernel->identityManager();

        auto ident = im->identityForAddress(u"signonly@test.example"_s);
        const auto msg(createItem(ident));

        auto composer = KMail::makeComposer(msg);
        composer->show();
        QVERIFY(QTest::qWaitForWindowExposed(composer));
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }

    const auto instance = Kleo::KeyCache::instance();
    auto key = instance->findByEMailAddress("level0@tofu.example").at(0);
    key.update();
    QCOMPARE(key.userID(0).tofuInfo().validity(), GpgME::TofuInfo::NoHistory);
    QCOMPARE(Kleo::trustLevel(key.userID(0)), Kleo::Level0);

    key = instance->findByEMailAddress("level1@tofu.example").at(0);
    key.update();
    QCOMPARE(key.userID(0).tofuInfo().validity(), GpgME::TofuInfo::LittleHistory);
    QCOMPARE(Kleo::trustLevel(key.userID(0)), Kleo::Level1);

    key = instance->findByEMailAddress("level2@tofu.example").at(0);
    key.update();
    QCOMPARE(key.userID(0).tofuInfo().validity(), GpgME::TofuInfo::BasicHistory);
    QCOMPARE(Kleo::trustLevel(key.userID(0)), Kleo::Level2);

    key = instance->findByEMailAddress("level3@tofu.example").at(0);
    key.update();
    QCOMPARE(key.userID(0).tofuInfo().validity(), GpgME::TofuInfo::LargeHistory);
    QCOMPARE(Kleo::trustLevel(key.userID(0)), Kleo::Level2);

    key = instance->findByEMailAddress("bad@tofu.example").at(0);
    key.update();
    QCOMPARE(key.userID(0).tofuInfo().validity(), GpgME::TofuInfo::LittleHistory);
    QCOMPARE(Kleo::trustLevel(key.userID(0)), Kleo::Level0);

    key = instance->findByEMailAddress("validity0@kde.example").at(0);
    key.update();
    QVERIFY(key.userID(0).tofuInfo().isNull());
    QCOMPARE(key.userID(0).validity(), GpgME::UserID::Unknown);
    QCOMPARE(Kleo::trustLevel(key.userID(0)), Kleo::Level0);

    key = instance->findByEMailAddress("validity1@kde.example").at(0);
    key.update();
    QVERIFY(key.userID(0).tofuInfo().isNull());
    QCOMPARE(key.userID(0).validity(), GpgME::UserID::Undefined);
    QCOMPARE(Kleo::trustLevel(key.userID(0)), Kleo::Level0);

    key = instance->findByEMailAddress("validity2@kde.example").at(0);
    key.update();
    QVERIFY(key.userID(0).tofuInfo().isNull());
    QCOMPARE(key.userID(0).validity(), GpgME::UserID::Never);
    QCOMPARE(Kleo::trustLevel(key.userID(0)), Kleo::Level0);

    key = instance->findByEMailAddress("validity3@kde.example").at(0);
    key.update();
    QVERIFY(key.userID(0).tofuInfo().isNull());
    QCOMPARE(key.userID(0).validity(), GpgME::UserID::Marginal);
    QCOMPARE(Kleo::trustLevel(key.userID(0)), Kleo::Level2);

    key = instance->findByEMailAddress("validity4@kde.example").at(0);
    key.update();
    QVERIFY(key.userID(0).tofuInfo().isNull());
    QCOMPARE(key.userID(0).validity(), GpgME::UserID::Full);
    QCOMPARE(Kleo::trustLevel(key.userID(0)), Kleo::Level3);

    key = instance->findByEMailAddress("validity5@kde.example").at(0);
    key.update();
    QVERIFY(key.userID(0).tofuInfo().isNull());
    QCOMPARE(key.userID(0).validity(), GpgME::UserID::Ultimate);
    QCOMPARE(Kleo::trustLevel(key.userID(0)), Kleo::Level4);
}
#include "moc_kmcomposerwintest.cpp"
