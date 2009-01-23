/**
 *  messagecomposer.cpp
 *
 *  Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#include "messagecomposer.h"

#include "kmmsgpart.h"
#define REALLY_WANT_KMCOMPOSEWIN_H
#include "kmcomposewin.h"
#undef REALLY_WANT_KMCOMPOSEWIN_H
#include "kcursorsaver.h"
#include "messagesender.h"
#include "kmfolder.h"
#include "kmfoldercombobox.h"
#include "keyresolver.h"
#include "kleo_util.h"
#include "globalsettings.h"
#include "custommimeheader.h"
#include "util.h"
#include "kmcomposereditor.h"

#include <gpgme++/key.h>
#include <gpgme++/keylistresult.h>
#include <gpgme++/encryptionresult.h>
#include <gpgme++/signingresult.h>
#include <gpgme++/context.h>

#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>

#include "libkleo/ui/keyselectiondialog.h"
#include "libkleo/ui/keyapprovaldialog.h"
#include "kleo/cryptobackendfactory.h"
#include "kleo/keylistjob.h"
#include "kleo/encryptjob.h"
#include "kleo/signencryptjob.h"
#include "kleo/signjob.h"
#include "kleo/specialjob.h"
#include <ui/messagebox.h>

#include <mimelib/mimepp.h>

#include <kpimutils/email.h>
#include <kmime/kmime_util.h>
#include <kmime/kmime_codecs.h>

#include <kconfiggroup.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kinputdialog.h>
#include <kdebug.h>
#include <kaction.h>
#include <ktoggleaction.h>

#include <QFile>
#include <QTextCodec>
#include <QTextEdit>
#include <QTimer>
#include <QList>
#include <QByteArray>

#include <algorithm>
#include <memory>

#undef MessageBox // Windows: avoid clash between MessageBox define and Kleo::MessageBox

// ## keep default values in sync with configuredialog.cpp, Security::CryptoTab::setup()
// This should be ported to a .kcfg one day I suppose (dfaure).

static inline bool warnSendUnsigned()
{
  KConfigGroup group( KMKernel::config(), "Composer" );
  return group.readEntry( "crypto-warning-unsigned", false );
}

static inline bool warnSendUnencrypted()
{
  KConfigGroup group( KMKernel::config(), "Composer" );
  return group.readEntry( "crypto-warning-unencrypted", false );
}

static inline bool saveMessagesEncrypted()
{
  KConfigGroup group( KMKernel::config(), "Composer" );
  return group.readEntry( "crypto-store-encrypted", true );
}

static inline bool encryptToSelf()
{
  // return !Kpgp::Module::getKpgp() || Kpgp::Module::getKpgp()->encryptToSelf();
  KConfigGroup group( KMKernel::config(), "Composer" );
  return group.readEntry( "crypto-encrypt-to-self", true );
}

static inline bool showKeyApprovalDialog()
{
  KConfigGroup group( KMKernel::config(), "Composer" );
  return group.readEntry( "crypto-show-keys-for-approval", true );
}

static inline int encryptKeyNearExpiryWarningThresholdInDays() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num = composer.readEntry( "crypto-warn-encr-key-near-expire-int", 14 );
  return qMax( 1, num );
}

static inline int signingKeyNearExpiryWarningThresholdInDays()
{
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num = composer.readEntry( "crypto-warn-sign-key-near-expire-int", 14 );
  return qMax( 1, num );
}

static inline int encryptRootCertNearExpiryWarningThresholdInDays()
{
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num = composer.readEntry( "crypto-warn-encr-root-near-expire-int", 14 );
  return qMax( 1, num );
}

static inline int signingRootCertNearExpiryWarningThresholdInDays() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num =
    composer.readEntry( "crypto-warn-sign-root-near-expire-int", 14 );
  return qMax( 1, num );
}

static inline int encryptChainCertNearExpiryWarningThresholdInDays()
{
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num =
    composer.readEntry( "crypto-warn-encr-chaincert-near-expire-int", 14 );
  return qMax( 1, num );
}

static inline int signingChainCertNearExpiryWarningThresholdInDays()
{
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num =
    composer.readEntry( "crypto-warn-sign-chaincert-near-expire-int", 14 );
  return qMax( 1, num );
}

/*
  Design of this:

  The idea is that the main run of applyChanges here makes two jobs:
  the first sets the flags for encryption/signing or not, and the other
  starts the encryption process.

  When a job is run, it has already been removed from the job queue. This
  means if one of the current jobs needs to add new jobs, it can add them
  to the front and that way control when new jobs are added.

  For example, the compose message job will add jobs that will do the
  actual encryption and signing.

  There are two types of jobs: synchronous and asynchronous:

  A synchronous job simply implments the execute() method and performs
  it's operation there and sets mComposer->mRc to false if the compose
  queue should be canceled.

  An asynchronous job only sets up and starts it's operation. Before
  returning, it connects to the result signals of the operation
  (e.g. Kleo::Job's result(...) signal) and sets mComposer->mHoldJobs
  to true. This makes the scheduler return to the event loop. The job
  is now responsible for giving control back to the scheduler by
  calling mComposer->doNextJob().
*/

/*
  Test plan:

  For each message format (e.g. openPGP/MIME)
   1. Body signed
   2. Body encrypted
   3. Body signed and encrypted
   4. Body encrypted, attachments encrypted (they must be encrypted together,
      mEarlyAddAttachments)
   5. Body encrypted, attachments not encrypted
   6. Body encrypted, attachment encrypted and signed (separately)
   7. Body not encrypted, one attachment encrypted+signed, one attachment
      encrypted only, one attachment signed only
      (https://intevation.de/roundup/aegypten/issue295)
      (this is the reason attachments can't be encrypted together)
   8. Body and attachments encrypted+signed (they must be encrypted+signed
      together, mEarlyAddAttachments)
   9. Body encrypted+signed, attachments encrypted
  10. Body encrypted+signed, one attachment signed, one attachment not
      encrypted nor signed
  ...

  I recorded a KDExecutor script sending all of the above (David)

  Further tests (which test opportunistic encryption):
   1. Send a message to a person with valid key but without encryption
      preference and answer the question whether the message should be
      encrypted with Yes.
   2. Send a message to a person with valid key but without encryption
      preference and answer the question whether the message should be
      encrypted with No.
   3. Send a message to a person with valid key and with encryption preference
      "Encrypt whenever possible" (aka opportunistic encryption).
*/

static QString mErrorProcessingStructuringInfo =
  i18n("<qt><p>Structuring information returned by the Crypto plug-in "
       "could not be processed correctly; the plug-in might be damaged.</p>"
       "<p>Please contact your system administrator.</p></qt>");
static QString mErrorNoCryptPlugAndNoBuildIn =
  i18n("<p>No active Crypto Plug-In was found and the built-in OpenPGP code "
       "did not run successfully.</p>"
       "<p>You can do two things to change this:</p>"
       "<ul><li><em>either</em> activate a Plug-In using the "
       "Settings->Configure KMail->Plug-In dialog.</li>"
       "<li><em>or</em> specify traditional OpenPGP settings on the same dialog's "
       "Identity->Advanced tab.</li></ul>");

class MessageComposerJob {
  public:
    MessageComposerJob( MessageComposer *composer ) : mComposer( composer ) {}
    virtual ~MessageComposerJob() {}

    virtual void execute() = 0;

  protected:
    // These are the methods that call the private MessageComposer methods
    // Workaround for friend not being inherited
    void adjustCryptFlags() { mComposer->adjustCryptFlags(); }
    void composeMessage() { mComposer->composeMessage(); }
    void continueComposeMessage( KMMessage &msg, bool doSign, bool doEncrypt,
                                 Kleo::CryptoMessageFormat format )
    {
      mComposer->continueComposeMessage( msg, doSign, doEncrypt, format );
    }
    void chiasmusEncryptAllAttachments() {
      mComposer->chiasmusEncryptAllAttachments();
    }
    MessageComposer *mComposer;
};

class ChiasmusBodyPartEncryptJob : public MessageComposerJob {
  public:
    ChiasmusBodyPartEncryptJob( MessageComposer *composer )
      : MessageComposerJob( composer ) {}

    void execute() {
      chiasmusEncryptAllAttachments();
    }
};

class AdjustCryptFlagsJob : public MessageComposerJob {
  public:
    AdjustCryptFlagsJob( MessageComposer *composer )
      : MessageComposerJob( composer ) {}

    void execute() {
      adjustCryptFlags();
    }
};

class ComposeMessageJob : public MessageComposerJob {
  public:
    ComposeMessageJob( MessageComposer *composer )
      : MessageComposerJob( composer ) {}

    void execute() {
      composeMessage();
    }
};

MessageComposer::MessageComposer( KMComposeWin *win )
  : QObject( win ), mComposeWin( win ), mCurrentJob( 0 ),
    mReferenceMessage( 0 ), mKeyResolver( 0 ),
    mUseOpportunisticEncryption( false ),
    mSignBody( false ), mEncryptBody( false ),
    mSigningRequested(  false ), mEncryptionRequested( false ),
    mDoSign( false ), mDoEncrypt( false ),
    mAllowedCryptoMessageFormats( 0 ),
    mDisableCrypto( false ),
    mDisableBreaking( false ),
    mDebugComposerCrypto( false ),
    mAutoCharset( true ),
    mIsRichText( false ),
    mIdentityUid( 0 ), mRc( true ),
    mHoldJobs( false ),
    mNewBodyPart( 0 ),
    mEarlyAddAttachments( false ), mAllAttachmentsAreInBody( false ),
    mPreviousBoundaryLevel( 0 ),
    mEncryptWithChiasmus( false ),
    mPerformingSignOperation( false )
{
}

MessageComposer::~MessageComposer()
{
  qDeleteAll( mEmbeddedImages );
  mEmbeddedImages.clear();
  delete mKeyResolver;
  mKeyResolver = 0;
}

void MessageComposer::applyChanges( bool disableCrypto )
{
  // Do the initial setup
  if ( !qgetenv( "KMAIL_DEBUG_COMPOSER_CRYPTO" ).isEmpty() ) {
    QByteArray cE = qgetenv( "KMAIL_DEBUG_COMPOSER_CRYPTO" );
    mDebugComposerCrypto = ( cE == "1" || cE.toUpper() == "ON" || cE.toUpper() == "TRUE" );
    kDebug(5006) << "KMAIL_DEBUG_COMPOSER_CRYPTO = TRUE";
  } else {
    mDebugComposerCrypto = false;
    kDebug(5006) << "KMAIL_DEBUG_COMPOSER_CRYPTO = FALSE";
  }

  mHoldJobs = false;
  mRc = true;

  mDisableCrypto = disableCrypto;

  // 1: Read everything from KMComposeWin and set all
  //    trivial parts of the message
  readFromComposeWin();

  // From now on, we're not supposed to read from the composer win
  // TODO: Make it so ;-)
  // 1.5: Replace all body parts with their chiasmus-encrypted equivalent
  mJobs.push_back( new ChiasmusBodyPartEncryptJob( this ) );

  // 2: Set encryption/signing options and resolve keys
  mJobs.push_back( new AdjustCryptFlagsJob( this ) );

  // 3: Build the message (makes the crypto jobs also)
  mJobs.push_back( new ComposeMessageJob( this ) );

  // Finally: Run the jobs
  doNextJob();
}

void MessageComposer::doNextJob()
{
  delete mCurrentJob; mCurrentJob = 0;

  if ( mJobs.isEmpty() ) {
    // No more jobs. Signal that we're done
    emitDone( mRc );
    return;
  }

  if ( !mRc ) {
    // Something has gone wrong - stop the process and bail out
    while ( !mJobs.isEmpty() ) {
      delete mJobs.front();
      mJobs.pop_front();
    }
    emitDone( false );
    return;
  }

  // We have more jobs to do, but allow others to come first
  QTimer::singleShot( 0, this, SLOT( slotDoNextJob() ) );
}

void MessageComposer::emitDone( bool ok )
{
  // Save memory - before sending the mail
  mEncodedBody = QByteArray();
  delete mNewBodyPart; mNewBodyPart = 0;
  mOldBodyPart.clear();
  emit done( ok );
}

void MessageComposer::slotDoNextJob()
{
  assert( !mCurrentJob );
  if ( mHoldJobs ) {
    // Always make it run from now. If more than one job should be held,
    // The individual jobs must do this.
    mHoldJobs = false;
  } else {
    assert( !mJobs.empty() );
    // Get the next job
    mCurrentJob = mJobs.front();
    assert( mCurrentJob );
    mJobs.pop_front();

    // Execute it
    mCurrentJob->execute();
  }

  // Finally run the next job if necessary
  if ( !mHoldJobs ) {
    doNextJob();
  }
}

void MessageComposer::readFromComposeWin()
{
  // Copy necessary attributes over
  mDisableBreaking = false;

  mSignBody = mComposeWin->mSignAction->isChecked();
  mSigningRequested = mSignBody; // for now; will be adjusted depending on attachments
  mEncryptBody = mComposeWin->mEncryptAction->isChecked();
  mEncryptionRequested = mEncryptBody; // for now; will be adjusted depending on attachments

  mAutoCharset = mComposeWin->mAutoCharset;
  mCharset = mComposeWin->mCharset;
  mReferenceMessage = mComposeWin->mMsg;
  // if the user made any modifications to the message then the Content-Type
  // of the message is no longer reliable (e. g. if he editted a draft/resent a
  // message and then removed all attachments or changed from PGP/MIME signed
  // to clearsigned);
  // even if the user didn't make any modifications to the message the
  // Content-Type of the message might be wrong, e.g. when inline-forwarding
  // an mp/alt message then the Content-Type is set to mp/alt although it should
  // be text/plain (cf. bug 127526);
  // OTOH we must not reset the Content-Type of inline invitations;
  // therefore we reset the Content-Type to text/plain whenever the current
  // Content-Type is multipart/*.
  if ( mReferenceMessage->type() == DwMime::kTypeMultipart ) {
    mReferenceMessage->setHeaderField( "Content-Type", "text/plain" );
  }
  mUseOpportunisticEncryption = GlobalSettings::self()->pgpAutoEncrypt();
  mAllowedCryptoMessageFormats = mComposeWin->cryptoMessageFormat();
  mEncryptWithChiasmus = mComposeWin->mEncryptWithChiasmus;

  if ( mAutoCharset ) {
    bool charsetFound = autoDetectCharset();
    if ( !charsetFound ) {
      mRc = false;
      return;
    }
  }
  mReferenceMessage->setCharset( mCharset );

  mReferenceMessage->setTo(mComposeWin->to());
  mReferenceMessage->setFrom(mComposeWin->from());
  mReferenceMessage->setCc(mComposeWin->cc());
  mReferenceMessage->setSubject(mComposeWin->subject());
  mReferenceMessage->setReplyTo(mComposeWin->replyTo());
  mReferenceMessage->setBcc(mComposeWin->bcc());

  const KPIMIdentities::Identity &id = mComposeWin->identity();

  KMFolder *f = mComposeWin->mFcc->getFolder();
  assert( f != 0 );
  if ( f->idString() == id.fcc() ) {
    mReferenceMessage->removeHeaderField( "X-KMail-Fcc" );
  } else {
    mReferenceMessage->setFcc( f->idString() );
  }

  // set the correct drafts folder
  mReferenceMessage->setDrafts( id.drafts() );

  if ( id.isDefault() ) {
    mReferenceMessage->removeHeaderField( "X-KMail-Identity" );
  } else {
    mReferenceMessage->setHeaderField( "X-KMail-Identity", QString::number( id.uoid() ) );
  }

  QString replyAddr;
  if ( !mComposeWin->replyTo().isEmpty() ) {
    replyAddr = mComposeWin->replyTo();
  } else {
    replyAddr = mComposeWin->from();
  }

  if ( mComposeWin->mRequestMDNAction->isChecked() ) {
    mReferenceMessage->setHeaderField( "Disposition-Notification-To", replyAddr);
  } else {
    mReferenceMessage->removeHeaderField( "Disposition-Notification-To" );
  }

  if ( mComposeWin->mUrgentAction->isChecked() ) {
    mReferenceMessage->setHeaderField( "X-PRIORITY", "2 (High)" );
    mReferenceMessage->setHeaderField( "Priority", "urgent" );
  } else {
    mReferenceMessage->removeHeaderField( "X-PRIORITY" );
    mReferenceMessage->removeHeaderField( "Priority" );
  }

  int num = GlobalSettings::self()->custHeaderCount();
  for(int ix=0; ix<num; ix++) {
    CustomMimeHeader customMimeHeader( QString::number(ix) );
    customMimeHeader.readConfig();
    mReferenceMessage->setHeaderField(
      KMMsgBase::toUsAscii( customMimeHeader.custHeaderName() ),
      customMimeHeader.custHeaderValue() );
  }

  // we have to remember the Bcc because it might have been overwritten
  // by a custom header (therefore we can't use bcc() later) and because
  // mimelib removes addresses without domain part (therefore we can't use
  // mReferenceMessage->bcc() later and also not now. So get the Bcc from
  // the composer window.)
  mBcc = mComposeWin->bcc();
  mTo = KPIMUtils::splitAddressList( mComposeWin->to().trimmed() );
  mCc = KPIMUtils::splitAddressList( mComposeWin->cc().trimmed() );
  mBccList = KPIMUtils::splitAddressList( mBcc.trimmed() );

  for ( int i = 0; i < mComposeWin->mAtmList.count(); ++i )
    mAttachments.push_back( Attachment( mComposeWin->mAtmList.at(i),
                                        mComposeWin->signFlagOfAttachment( i ),
                                        mComposeWin->encryptFlagOfAttachment( i ) ) );

  mEmbeddedImages = mComposeWin->mEditor->embeddedImages();

  mIsRichText = ( mComposeWin->mEditor->textMode() == KMeditor::Rich );
  mIdentityUid = mComposeWin->identityUid();
  if ( !breakLinesAndApplyCodec() ) {
    mRc = false;
    return;
  }
}

static QByteArray escape_quoted_string( const QByteArray &str ) {
  QByteArray result;
  const unsigned int str_len = str.length();
  result.resize( 2*str_len );
  char *d = result.data();
  for ( unsigned int i = 0; i < str_len; ++i )
    switch ( const char ch = str[i] ) {
    case '\\':
    case '"':
      *d++ = '\\';
    default: // fall through:
      *d++ = ch;
    }
  result.truncate( d - result.begin() );
  return result;
}

bool MessageComposer::encryptWithChiasmus( const Kleo::CryptoBackend::Protocol *chiasmus,
                                           const QByteArray &body,
                                           QByteArray &resultData ) const
{
  std::auto_ptr<Kleo::SpecialJob> job(
    chiasmus->specialJob( "x-encrypt", QMap<QString,QVariant>() ) );
  if ( !job.get() ) {
    const QString msg = i18n( "Chiasmus backend does not offer the "
                              "\"x-encrypt\" function. Please report this bug." );
    KMessageBox::error( mComposeWin, msg, i18n( "Chiasmus Backend Error" ) );
    return false;
  }
  if ( !job->setProperty( "key", GlobalSettings::chiasmusKey() ) ||
       !job->setProperty( "options", GlobalSettings::chiasmusOptions() ) ||
       !job->setProperty( "input", body ) ) {
    const QString msg = i18n( "The \"x-encrypt\" function does not accept "
                              "the expected parameters. Please report this bug." );
    KMessageBox::error( mComposeWin, msg, i18n( "Chiasmus Backend Error" ) );
    return false;
  }
  const GpgME::Error err = job->exec();
  if ( err.isCanceled() || err ) {
    if ( err )
      job->showErrorDialog( mComposeWin, i18n( "Chiasmus Encryption Error" ) );
    return false;
  }
  const QVariant result = job->property( "result" );
  if ( result.type() != QVariant::ByteArray ) {
    const QString msg = i18n( "Unexpected return value from Chiasmus backend: "
                              "The \"x-encrypt\" function did not return a "
                              "byte array. Please report this bug." );
    KMessageBox::error( mComposeWin, msg, i18n( "Chiasmus Backend Error" ) );
    return false;
  }
  resultData = result.toByteArray();
  return true;
}

void MessageComposer::chiasmusEncryptAllAttachments() {
  if ( !mEncryptWithChiasmus )
    return;
  assert( !GlobalSettings::chiasmusKey().isEmpty() ); // kmcomposewin code should have made sure
  if ( mAttachments.empty() )
    return;
  const Kleo::CryptoBackend::Protocol *chiasmus
    = Kleo::CryptoBackendFactory::instance()->protocol( "Chiasmus" );
  assert( chiasmus ); // kmcomposewin code should have made sure

  for ( QVector<Attachment>::iterator it = mAttachments.begin(), end = mAttachments.end();
        it != end; ++it ) {
    KMMessagePart *part = it->part;
    const QString filename = part->fileName();
    if ( filename.endsWith( ".xia", Qt::CaseInsensitive ) )
      continue; // already encrypted
    const QByteArray body = part->bodyDecodedBinary();
    QByteArray resultData;
    if ( !encryptWithChiasmus( chiasmus, body, resultData ) ) {
      mRc = false;
      return;
    }
    // everything ok, so let's fill in the part again:
    QList<int> dummy;
    part->setBodyAndGuessCte( resultData, dummy );
    part->setTypeStr( "application" );
    part->setSubtypeStr( "vnd.de.bund.bsi.chiasmus" );
    part->setName( filename + ".xia" );

    // this is taken from kmmsgpartdlg.cpp:
    QByteArray encoding =
      KMMsgBase::autoDetectCharset( part->charset(),
                                    KMMessage::preferredCharsets(), filename );
    if ( encoding.isEmpty() )
      encoding = "utf-8";
    const QByteArray enc_name = KMMsgBase::encodeRFC2231String( filename + ".xia", encoding );
    const QByteArray cDisp = "attachment;\n\tfilename"
                             + ( QString( enc_name ) != filename + ".xia"
                                 ? "*=" + enc_name
                                 : "=\"" + escape_quoted_string( enc_name ) + '\"' );
    part->setContentDisposition( cDisp );
  }
}

void MessageComposer::adjustCryptFlags()
{
  if ( !mDisableCrypto &&
       mAllowedCryptoMessageFormats & Kleo::InlineOpenPGPFormat &&
       !mAttachments.empty() &&
       ( mSigningRequested || mEncryptionRequested ) )
  {
    int ret;
    if ( mAllowedCryptoMessageFormats == Kleo::InlineOpenPGPFormat ) {
      ret = KMessageBox::warningYesNoCancel( mComposeWin,
                                             i18n("The inline OpenPGP crypto message format "
                                                  "does not support encryption or signing "
                                                  "of attachments.\n"
                                                  "Really use deprecated inline OpenPGP?"),
                                             i18n("Insecure Message Format"),
                                             KGuiItem( i18n("Use Inline OpenPGP") ),
                                             KGuiItem( i18n("Use OpenPGP/MIME")) );
    }
    else {
      // if other crypto message formats are allowed then simply don't use
      // inline OpenPGP
      ret = KMessageBox::No;
    }

    if ( ret == KMessageBox::Cancel ) {
      mRc = false;
      return;
    } else if ( ret == KMessageBox::No ) {
      mAllowedCryptoMessageFormats &= ~Kleo::InlineOpenPGPFormat;
      mAllowedCryptoMessageFormats |= Kleo::OpenPGPMIMEFormat;
      if ( mSigningRequested ) {
        // The composer window disabled signing on the attachments, re-enable it
        for ( int idx = 0; idx < mAttachments.size(); ++idx )
          mAttachments[idx].sign = true;
      }
      if ( mEncryptionRequested ) {
        // The composer window disabled encrypting on the attachments, re-enable it
        // We assume this is what the user wants - after all he chose OpenPGP/MIME for this.
        for ( int idx = 0; idx < mAttachments.size(); ++idx )
          mAttachments[idx].encrypt = true;
      }
    }
  }

  mKeyResolver =
    new Kleo::KeyResolver( encryptToSelf(), showKeyApprovalDialog(),
                           mUseOpportunisticEncryption, mAllowedCryptoMessageFormats,
                           encryptKeyNearExpiryWarningThresholdInDays(),
                           signingKeyNearExpiryWarningThresholdInDays(),
                           encryptRootCertNearExpiryWarningThresholdInDays(),
                           signingRootCertNearExpiryWarningThresholdInDays(),
                           encryptChainCertNearExpiryWarningThresholdInDays(),
                           signingChainCertNearExpiryWarningThresholdInDays() );

  if ( !mDisableCrypto ) {
    const KPIMIdentities::Identity &id =
      kmkernel->identityManager()->identityForUoidOrDefault( mIdentityUid );

    QStringList encryptToSelfKeys;
    if ( !id.pgpEncryptionKey().isEmpty() )
      encryptToSelfKeys.push_back( id.pgpEncryptionKey() );
    if ( !id.smimeEncryptionKey().isEmpty() )
      encryptToSelfKeys.push_back( id.smimeEncryptionKey() );
    if ( mKeyResolver->setEncryptToSelfKeys( encryptToSelfKeys ) != Kpgp::Ok ) {
      mRc = false;
      return;
    }

    QStringList signKeys;
    if ( !id.pgpSigningKey().isEmpty() )
      signKeys.push_back( mPGPSigningKey = id.pgpSigningKey() );
    if ( !id.smimeSigningKey().isEmpty() )
      signKeys.push_back( mSMIMESigningKey = id.smimeSigningKey() );
    if ( mKeyResolver->setSigningKeys( signKeys ) != Kpgp::Ok ) {
      mRc = false;
      return;
    }
  }

  mKeyResolver->setPrimaryRecipients( mTo + mCc );
  mKeyResolver->setSecondaryRecipients( mBccList );

  // check settings of composer buttons *and* attachment check boxes
  bool doSignCompletely    = mSigningRequested;
  bool doEncryptCompletely = mEncryptionRequested;
  for ( int idx = 0; idx < mAttachments.size(); ++idx ) {
    if ( mAttachments[idx].encrypt ) {
      mEncryptionRequested = true;
    } else {
      doEncryptCompletely = false;
    }
    if ( mAttachments[idx].sign ) {
      mSigningRequested = true;
    } else {
      doSignCompletely = false;
    }
  }

  mDoSign = !mDisableCrypto && determineWhetherToSign( doSignCompletely );

  if ( !mRc )
    return;

  mDoEncrypt = !mDisableCrypto && determineWhetherToEncrypt( doEncryptCompletely );

  if ( !mRc )
    return;

  // resolveAllKeys needs to run even if mDisableCrypto == true, since
  // we depend on it collecting all recipients into one dummy
  // SplitInfo to avoid special-casing all over the place:
  if ( mKeyResolver->resolveAllKeys( mDoSign, mDoEncrypt ) != Kpgp::Ok )
    mRc = false;
}

bool MessageComposer::determineWhetherToSign( bool doSignCompletely )
{
  bool sign = false;
  switch ( mKeyResolver->checkSigningPreferences( mSigningRequested ) ) {
  case Kleo::DoIt:
    if ( !mSigningRequested ) {
      markAllAttachmentsForSigning( true );
      return true;
    }
    sign = true;
    break;
  case Kleo::DontDoIt:
    sign = false;
    break;
  case Kleo::AskOpportunistic:
    assert( 0 );
  case Kleo::Ask:
  {
    // the user wants to be asked or has to be asked
    const KCursorSaver idle( KBusyPtr::idle() );
    const QString msg = i18n("Examination of the recipient's signing preferences "
                             "yielded that you be asked whether or not to sign "
                             "this message.\n"
                             "Sign this message?");
    switch ( KMessageBox::questionYesNoCancel( mComposeWin, msg,
                                               i18n("Sign Message?"),
                                               KGuiItem( i18nc("to sign","&Sign") ),
                                               KGuiItem( i18n("Do &Not Sign") ) ) ) {
    case KMessageBox::Cancel:
      mRc = false;
      return false;
    case KMessageBox::Yes:
      markAllAttachmentsForSigning( true );
      return true;
    case KMessageBox::No:
      markAllAttachmentsForSigning( false );
      return false;
    }
  }
  break;
  case Kleo::Conflict:
  {
    // warn the user that there are conflicting signing preferences
    const KCursorSaver idle( KBusyPtr::idle() );
    const QString msg = i18n("There are conflicting signing preferences "
                             "for these recipients.\n"
                             "Sign this message?");
    switch ( KMessageBox::warningYesNoCancel( mComposeWin, msg,
                                              i18n("Sign Message?"),
                                              KGuiItem( i18nc("to sign","&Sign") ),
                                              KGuiItem( i18n("Do &Not Sign") ) ) ) {
    case KMessageBox::Cancel:
      mRc = false;
      return false;
    case KMessageBox::Yes:
      markAllAttachmentsForSigning( true );
      return true;
    case KMessageBox::No:
      markAllAttachmentsForSigning( false );
      return false;
    }
  }
  break;
  case Kleo::Impossible:
  {
    const KCursorSaver idle( KBusyPtr::idle() );
    const QString msg = i18n("You have requested to sign this message, "
                             "but no valid signing keys have been configured "
                             "for this identity.");
    if ( KMessageBox::warningContinueCancel( mComposeWin, msg,
                                             i18n("Send Unsigned?"),
                                             KGuiItem( i18n("Send &Unsigned") ) )
         == KMessageBox::Cancel ) {
      mRc = false;
      return false;
    } else {
      markAllAttachmentsForSigning( false );
      return false;
    }
  }
  }

  if ( !sign || !doSignCompletely ) {
    if ( warnSendUnsigned() ) {
      const KCursorSaver idle( KBusyPtr::idle() );
      const QString msg = sign && !doSignCompletely ?
                          i18n("Some parts of this message will not be signed.\n"
                               "Sending only partially signed messages might violate site policy.\n"
                               "Sign all parts instead?") // oh, I hate this...
                          : i18n("This message will not be signed.\n"
                                 "Sending unsigned message might violate site policy.\n"
                                 "Sign message instead?"); // oh, I hate this...
      const QString buttonText = sign && !doSignCompletely
                                 ? i18n("&Sign All Parts") : i18n("&Sign");
      switch ( KMessageBox::warningYesNoCancel( mComposeWin, msg,
						i18n("Unsigned-Message Warning"),
						KGuiItem( buttonText ),
						KGuiItem( i18n("Send &As Is") ) ) ) {
      case KMessageBox::Cancel:
	mRc = false;
	return false;
      case KMessageBox::Yes:
	markAllAttachmentsForSigning( true );
	return true;
      case KMessageBox::No:
	return sign || doSignCompletely;
      }
    }
  }

  return sign || doSignCompletely;
}

bool MessageComposer::determineWhetherToEncrypt( bool doEncryptCompletely )
{
  bool encrypt = false;
  bool opportunistic = false;
  switch ( mKeyResolver->checkEncryptionPreferences( mEncryptionRequested ) ) {
  case Kleo::DoIt:
    if ( !mEncryptionRequested ) {
      markAllAttachmentsForEncryption( true );
      return true;
    }
    encrypt = true;
    break;
  case Kleo::DontDoIt:
    encrypt = false;
    break;
  case Kleo::AskOpportunistic:
    opportunistic = true;
    // fall through...
  case Kleo::Ask:
  {
    // the user wants to be asked or has to be asked
    const KCursorSaver idle( KBusyPtr::idle() );
    const QString msg = opportunistic
                        ? i18n("Valid trusted encryption keys were found for all recipients.\n"
                               "Encrypt this message?")
                        : i18n("Examination of the recipient's encryption preferences "
                               "yielded that you be asked whether or not to encrypt "
                               "this message.\n"
                               "Encrypt this message?");
    switch ( KMessageBox::questionYesNoCancel( mComposeWin, msg,
                                               i18n("Encrypt Message?"),
                                               KGuiItem( mDoSign
                                                         ? i18n("Sign && &Encrypt")
                                                         : i18n("&Encrypt") ),
                                               KGuiItem( mDoSign
                                                         ? i18n("&Sign Only")
                                                         : i18n("&Send As-Is") ) ) ) {
    case KMessageBox::Cancel:
      mRc = false;
      return false;
    case KMessageBox::Yes:
      markAllAttachmentsForEncryption( true );
      return true;
    case KMessageBox::No:
      markAllAttachmentsForEncryption( false );
      return false;
    }
  }
  break;
  case Kleo::Conflict:
  {
    // warn the user that there are conflicting encryption preferences
    const KCursorSaver idle( KBusyPtr::idle() );
    const QString msg = i18n("There are conflicting encryption preferences "
                             "for these recipients.\n"
                             "Encrypt this message?");
    switch ( KMessageBox::warningYesNoCancel( mComposeWin, msg,
                                              i18n("Encrypt Message?"),
                                              KGuiItem( i18n("&Encrypt") ),
                                              KGuiItem( i18n("Do &Not Encrypt")) ) ) {
    case KMessageBox::Cancel:
      mRc = false;
      return false;
    case KMessageBox::Yes:
      markAllAttachmentsForEncryption( true );
      return true;
    case KMessageBox::No:
      markAllAttachmentsForEncryption( false );
      return false;
    }
  }
  break;
  case Kleo::Impossible:
  {
    const KCursorSaver idle( KBusyPtr::idle() );
    const QString msg = i18n("You have requested to encrypt this message, "
                             "and to encrypt a copy to yourself, "
                             "but no valid trusted encryption keys have been "
                             "configured for this identity.");
    if ( KMessageBox::warningContinueCancel( mComposeWin, msg,
                                             i18n("Send Unencrypted?"),
                                             KGuiItem( i18n("Send &Unencrypted") ) )
         == KMessageBox::Cancel ) {
      mRc = false;
      return false;
    } else {
      markAllAttachmentsForEncryption( false );
      return false;
    }
  }
  }

  if ( !encrypt || !doEncryptCompletely ) {
    if ( warnSendUnencrypted() ) {
      const KCursorSaver idle( KBusyPtr::idle() );
      const QString msg = !doEncryptCompletely ?
                          i18n("Some parts of this message will not be encrypted.\n"
                               "Sending only partially encrypted messages might violate "
                               "site policy and/or leak sensitive information.\n"
                               "Encrypt all parts instead?") // oh, I hate this...
                          : i18n("This message will not be encrypted.\n"
                                 "Sending unencrypted messages might violate site policy and/or "
                                 "leak sensitive information.\n"
                                 "Encrypt messages instead?"); // oh, I hate this...
      const QString buttonText = !doEncryptCompletely
                                 ? i18n("&Encrypt All Parts") : i18n("&Encrypt");
      switch ( KMessageBox::warningYesNoCancel( mComposeWin, msg,
						i18n("Unencrypted Message Warning"),
						KGuiItem( buttonText ),
						KGuiItem( mDoSign
                                                          ? i18n("&Sign Only")
                                                          : i18n("&Send As-Is")) ) ) {
      case KMessageBox::Cancel:
	mRc = false;
	return false;
      case KMessageBox::Yes:
	markAllAttachmentsForEncryption( true );
	return true;
      case KMessageBox::No:
	return encrypt || doEncryptCompletely;
      }
    }
  }

  return encrypt || doEncryptCompletely;
}

void MessageComposer::markAllAttachmentsForSigning( bool sign ) {
  mSignBody = sign;
  for ( QVector<Attachment>::iterator it = mAttachments.begin(); it != mAttachments.end(); ++it )
    it->sign = sign;
}

void MessageComposer::markAllAttachmentsForEncryption( bool enc ) {
  mEncryptBody = enc;
  for ( QVector<Attachment>::iterator it = mAttachments.begin(); it != mAttachments.end(); ++it )
    it->encrypt = enc;
}

void MessageComposer::composeMessage()
{
  for ( unsigned int i = 0; i < numConcreteCryptoMessageFormats; ++i ) {
    if ( mKeyResolver->encryptionItems( concreteCryptoMessageFormats[i] ).empty() )
      continue;
    KMMessage *msg = new KMMessage( *mReferenceMessage );
    composeMessage( *msg, mDoSign, mDoEncrypt, concreteCryptoMessageFormats[i] );
    if ( !mRc )
      return;
  }
}

//
// These are replacements for StructuringInfo(Wrapper):
//

// check whether to use multipart/{signed,encrypted}
static inline bool makeMultiMime( Kleo::CryptoMessageFormat f, bool sign ) {
  switch ( f ) {
  default:
  case Kleo::InlineOpenPGPFormat:
  case Kleo::SMIMEOpaqueFormat:   return false;
  case Kleo::OpenPGPMIMEFormat:   return true;
  case Kleo::SMIMEFormat:         return sign; // only on sign - there's no mp/encrypted for S/MIME
  }
}
static inline bool makeMultiPartSigned( Kleo::CryptoMessageFormat f ) {
  return makeMultiMime( f, true );
}
static inline bool makeMultiPartEncrypted( Kleo::CryptoMessageFormat f ) {
  return makeMultiMime( f, false );
}

static inline bool makeMimeObject( Kleo::CryptoMessageFormat f, bool /*signing*/ ) {
  return f != Kleo::InlineOpenPGPFormat;
}

static inline const char *toplevelContentType( Kleo::CryptoMessageFormat f, bool signing ) {
  switch ( f ) {
  default:
  case Kleo::InlineOpenPGPFormat: return 0;
  case Kleo::OpenPGPMIMEFormat:
    return signing ?
      "multipart/signed;\n\t"
      "boundary=\"%boundary\";\n\t"
      "protocol=\"application/pgp-signature\";\n\t"
      "micalg=pgp-sha1" // FIXME: obtain this parameter from gpgme!
      :
    "multipart/encrypted;\n\t"
      "boundary=\"%boundary\";\n\t"
      "protocol=\"application/pgp-encrypted\"";
  case Kleo::SMIMEFormat:
    if ( signing ) {
      return
        "multipart/signed;\n\t"
        "boundary=\"%boundary\";\n\t"
        "protocol=\"application/pkcs7-signature\";\n\t"
        "micalg=sha1"; // FIXME: obtain this parameter from gpgme!
    // fall through (for encryption, there's no difference between
    // SMIME and SMIMEOpaque, since there is no mp/encrypted for
    // S/MIME):
    }
  case Kleo::SMIMEOpaqueFormat:
    return signing ?
      "application/pkcs7-mime;\n\t"
      "smime-type=signed-data;\n\t"
      "name=\"smime.p7m\";\n\t"
      :
    "application/pkcs7-mime;\n\t"
      "smime-type=enveloped-data;\n\t"
      "name=\"smime.p7m\";\n\t";
  }
}

static inline const char *toplevelContentDisposition( Kleo::CryptoMessageFormat f, bool signing )
{
  switch ( f ) {
  default:
  case Kleo::InlineOpenPGPFormat:
  case Kleo::OpenPGPMIMEFormat:
    return 0;
  case Kleo::SMIMEFormat:
    if ( signing ) {
      return 0;
    }
  case Kleo::SMIMEOpaqueFormat:
    return "attachment; filename=\"smime.p7m\"";
  }
}

static inline bool includeCleartextWhenSigning( Kleo::CryptoMessageFormat f )
{
  return makeMultiPartSigned( f );
}

static inline const char *nestedContentType( Kleo::CryptoMessageFormat f, bool signing )
{
  switch ( f ) {
  case Kleo::OpenPGPMIMEFormat:
    return signing ?
      "application/pgp-signature; name=signature.asc \nContent-Description: This is a digitally signed message part." :
    "application/octet-stream";
  case Kleo::SMIMEFormat:
    if ( signing ) {
      return "application/pkcs7-signature; name=\"smime.p7s\"";
    }
    // fall through:
  default:
  case Kleo::InlineOpenPGPFormat:
  case Kleo::SMIMEOpaqueFormat:
    return 0;
  }
}

static inline const char *nestedContentDisposition( Kleo::CryptoMessageFormat f, bool signing )
{
  if ( !signing && f == Kleo::OpenPGPMIMEFormat ) {
    return "inline; filename=\"msg.asc\"";
  }
  if ( signing && f == Kleo::SMIMEFormat ) {
    return "attachment; filename=\"smime.p7s\"";
  }
  return 0;
}

static inline bool binaryHint( Kleo::CryptoMessageFormat f )
{
  switch ( f ) {
  case Kleo::SMIMEFormat:
  case Kleo::SMIMEOpaqueFormat:
    return true;
  default:
  case Kleo::OpenPGPMIMEFormat:
  case Kleo::InlineOpenPGPFormat:
    return false;
  }
}

static inline bool armor( Kleo::CryptoMessageFormat f )
{
  return !binaryHint( f );
}

static inline bool textMode( Kleo::CryptoMessageFormat f )
{
  return f == Kleo::InlineOpenPGPFormat;
}

static inline GpgME::SignatureMode signingMode( Kleo::CryptoMessageFormat f )
{
  switch ( f ) {
  case Kleo::SMIMEOpaqueFormat:
    return GpgME::NormalSignatureMode;
  case Kleo::InlineOpenPGPFormat:
    return GpgME::Clearsigned;
  default:
  case Kleo::SMIMEFormat:
  case Kleo::OpenPGPMIMEFormat:
    return GpgME::Detached;
  }
}

//
// END replacements for StructuringInfo(Wrapper)
//

class EncryptMessageJob : public MessageComposerJob {
  public:
    EncryptMessageJob( KMMessage *msg, const Kleo::KeyResolver::SplitInfo &si,
                       bool doSign, bool doEncrypt, const QByteArray &encodedBody,
                       int boundaryLevel, const KMMessagePart &oldBodyPart,
                       KMMessagePart *newBodyPart, Kleo::CryptoMessageFormat format,
                       MessageComposer *composer )
      : MessageComposerJob( composer ), mMsg( msg ), mSplitInfo( si ),
        mDoSign( doSign ), mDoEncrypt( doEncrypt ), mEncodedBody( encodedBody ),
        mBoundaryLevel( boundaryLevel ), mOldBodyPart( oldBodyPart ),
        mNewBodyPart( newBodyPart ), mFormat( format ) {}

    void execute() {
      KMMessagePart tmpNewBodyPart;
      tmpNewBodyPart.duplicate( *mNewBodyPart );

      // TODO: Async call

      mComposer->encryptMessage( mMsg, mSplitInfo, mDoSign, mDoEncrypt,
                                 tmpNewBodyPart, mFormat );
      if ( !mComposer->mRc ) {
        delete mMsg; mMsg = 0;
        return;
      }
      mComposer->mMessageList.push_back( mMsg );
    }

  private:
    KMMessage *mMsg;
    Kleo::KeyResolver::SplitInfo mSplitInfo;
    bool mDoSign, mDoEncrypt;
    QByteArray mEncodedBody;
    int mBoundaryLevel;
    KMMessagePart mOldBodyPart;
    KMMessagePart *mNewBodyPart;
    Kleo::CryptoMessageFormat mFormat;
};

class SetLastMessageAsUnencryptedVersionOfLastButOne : public MessageComposerJob {
  public:
    SetLastMessageAsUnencryptedVersionOfLastButOne( MessageComposer *composer )
      : MessageComposerJob( composer ) {}

    void execute() {
      KMMessage *last = mComposer->mMessageList.back();
      mComposer->mMessageList.pop_back();
      mComposer->mMessageList.back()->setUnencryptedMsg( last );
    }
};

void MessageComposer::composeInlineOpenPGPMessage( KMMessage &theMessage,
                                                   bool doSign, bool doEncrypt )
{
  // preprocess the body text
  QByteArray body = mBodyText;
  if (body.isNull()) {
    mRc = false;
    return;
  }

  mNewBodyPart = 0; // unused
  mEarlyAddAttachments = false;
  mAllAttachmentsAreInBody = false;

  // set the main headers
  theMessage.deleteBodyParts();
  QString oldContentType = theMessage.headerField( "Content-Type" );
  theMessage.removeHeaderField( "Content-Type" );
  theMessage.removeHeaderField( "Content-Transfer-Encoding" );

  const std::vector<Kleo::KeyResolver::SplitInfo> splitInfos =
    mKeyResolver->encryptionItems( Kleo::InlineOpenPGPFormat );
  kWarning( splitInfos.empty() ) << "splitInfos.empty() for InlineOpenPGPFormat";
  std::vector<Kleo::KeyResolver::SplitInfo>::const_iterator it;
  for ( it = splitInfos.begin(); it != splitInfos.end(); ++it ) {
    const Kleo::KeyResolver::SplitInfo &splitInfo = *it;
    KMMessage *msg = new KMMessage( theMessage );
    if ( doEncrypt ) {
      Kpgp::Result result;
      QByteArray encryptedBody;
      if ( doSign ) {  // Sign and encrypt
        const std::vector<GpgME::Key> signingKeys =
          mKeyResolver->signingKeys( Kleo::InlineOpenPGPFormat );
        result = pgpSignedAndEncryptedMsg( encryptedBody, body, signingKeys,
                                           splitInfo.keys, Kleo::InlineOpenPGPFormat );
      } else { // Encrypt but don't sign
        result = pgpEncryptedMsg( encryptedBody, body,
                                  splitInfo.keys, Kleo::InlineOpenPGPFormat );
      }
      if ( result != Kpgp::Ok ) {
        mRc = false;
        return;
      }
      assert( !encryptedBody.isNull() ); // if gpg-agent is running, then blame gpgme if this is hit
      mOldBodyPart.setBodyEncodedBinary( encryptedBody );
    } else {
      if ( doSign ) { // Sign but don't encrypt
        pgpSignedMsg( body, Kleo::InlineOpenPGPFormat );
        if ( mSignature.isNull() ) {
          mRc = false;
          return;
        }
        mOldBodyPart.setBodyEncodedBinary( mSignature );
      } else { // don't sign nor encrypt -> nothing to do
        assert( !body.isNull() );
        mOldBodyPart.setBodyEncoded( body );
      }
    }
    mOldBodyPart.setContentDisposition( "inline" );
    mOldBodyPart.setOriginalContentTypeStr( oldContentType.toUtf8() );
    mOldBodyPart.setCharset( mCharset );
    addBodyAndAttachments( msg, splitInfo, false, false, mOldBodyPart, Kleo::InlineOpenPGPFormat );
    mMessageList.push_back( msg );
    if ( it == splitInfos.begin() ) {
      if ( doEncrypt && !saveMessagesEncrypted() ) {
        mOldBodyPart.setBodyEncoded( body );
        KMMessage *msgUnenc = new KMMessage( theMessage );
        addBodyAndAttachments( msgUnenc, splitInfo, false, false, mOldBodyPart,
                               Kleo::InlineOpenPGPFormat );
        msg->setUnencryptedMsg( msgUnenc );
      }
    }
  } // end for
}

// very much inspired by composeInlineOpenPGPMessage
void MessageComposer::composeChiasmusMessage( KMMessage &theMessage,
                                              Kleo::CryptoMessageFormat format )
{
  assert( !GlobalSettings::chiasmusKey().isEmpty() ); // kmcomposewin code should have made sure
  const Kleo::CryptoBackendFactory *cpf = Kleo::CryptoBackendFactory::instance();
  assert( cpf );
  const Kleo::CryptoBackend::Protocol *chiasmus = cpf->protocol( "Chiasmus" );
  assert( chiasmus ); // kmcomposewin code should have made sure

  // preprocess the body text
  QByteArray body = mBodyText;
  if (body.isNull()) {
    mRc = false;
    return;
  }

  mNewBodyPart = 0; // unused
  mEarlyAddAttachments = false;
  mAllAttachmentsAreInBody = false;

  // set the main headers
  theMessage.deleteBodyParts();
  QString oldContentType = theMessage.headerField( "Content-Type" );
  theMessage.removeHeaderField( "Content-Type" );
  theMessage.removeHeaderField( "Content-Transfer-Encoding" );

  QByteArray plainText( body );

  // This reads strange, but we know that AdjustCryptFlagsJob created a single splitinfo,
  // under the given "format" (usually openpgp/mime; doesn't matter)
  const std::vector<Kleo::KeyResolver::SplitInfo> splitInfos =
    mKeyResolver->encryptionItems( format );
  assert( splitInfos.size() == 1 );
  for ( std::vector<Kleo::KeyResolver::SplitInfo>::const_iterator it = splitInfos.begin();
        it != splitInfos.end(); ++it ) {
    const Kleo::KeyResolver::SplitInfo &splitInfo = *it;
    KMMessage *msg = new KMMessage( theMessage );
    QByteArray encryptedBody;

    if ( !encryptWithChiasmus( chiasmus, plainText, encryptedBody ) ) {
      mRc = false;
      return;
    }
    assert( !encryptedBody.isNull() );
    // This leaves CTE==7-bit, no good
    //mOldBodyPart.setBodyEncodedBinary( encryptedBody );

    bool doSign = false;
    QList<int> allowedCTEs;
    mOldBodyPart.setBodyAndGuessCte(
      encryptedBody, allowedCTEs,
      !kmkernel->msgSender()->sendQuotedPrintable() && !doSign, doSign );

    mOldBodyPart.setContentDisposition( "inline" );
    // Used in case of no attachments
    mOldBodyPart.setOriginalContentTypeStr(
      "application/vnd.de.bund.bsi.chiasmus-text;chiasmus-charset=" + mCharset );
    // Used in case of attachments
    mOldBodyPart.setTypeStr( "application" );
    mOldBodyPart.setSubtypeStr( "vnd.de.bund.bsi.chiasmus-text" );
    mOldBodyPart.setAdditionalCTypeParamStr( QByteArray( "chiasmus-charset=" + mCharset ) );
    addBodyAndAttachments( msg, splitInfo, false, false, mOldBodyPart,
                           Kleo::InlineOpenPGPFormat );
    mMessageList.push_back( msg );

    if ( it == splitInfos.begin() && !saveMessagesEncrypted() ) {
      mOldBodyPart.setBodyEncoded( body );
      KMMessage *msgUnenc = new KMMessage( theMessage );
      addBodyAndAttachments( msgUnenc, splitInfo, false, false, mOldBodyPart,
                             Kleo::InlineOpenPGPFormat );
      msg->setUnencryptedMsg( msgUnenc );
    }
  }
}

void MessageComposer::composeMessage( KMMessage &theMessage,
                                      bool doSign, bool doEncrypt,
                                      Kleo::CryptoMessageFormat format )
{
  kDebug() << "Starting to compose message";

  // Handle Inline OpenPGP seperatly
  if ( format == Kleo::InlineOpenPGPFormat ) {
    composeInlineOpenPGPMessage( theMessage, doSign, doEncrypt );
    return;
  }

  // Handle Chiasmus seperatly
  if ( mEncryptWithChiasmus ) {
    composeChiasmusMessage( theMessage, format );
    return;
  }

  // How composing is done here:
  // ===========================
  //
  // The main body part created and then saved to mOldBodyPart. Main body part here
  // means everything that is included before signing or encrypting the message.
  // That means the main body part doesn't contain any crypto-related bits like signatures
  // or encryption.
  // The main body part also contains attachments that have the same signing / encryption
  // policy like the composer text.
  // Attachments outside the main body part are signed / encrypted seperatly later in
  // addBodyAndAttachments().
  // Thus, the main body part can be multipart/alternative for HTML mails,
  // multipart/related for HTML mails with images, text/plain for plain old mails
  // or multipart/mixed for messages with attachments (more specifc, 'early' attachments
  // that are not added seperatly later).
  //
  // There are two main body parts: mOldBodyPart and mNewBodyPart.
  // mOldBodyPart is the main body part which we create here. mNewBodyPart is the main body
  // part plus signatures / encryption and late attachments.
  // mNewBodyPart is first created later in this function, when doing signing, and expanded
  // when the EncyptMessageJob is executed.
  //
  // theMessage is a copy of the reference message from the composer window. Below,
  // we delete all body parts from it, and therefore only care about its header fields.
  // Apart from header fields, theMessage is also used for calling createDWBodyPart() on it,
  // but for nothing else.
  // Later, in addBodyAndAttachments(), we actually add mNewBodyPart as the body part of
  // the message. After that, theMessage is complete, and is the final message which will
  // be sent.
  // (Actually, there are multiple messages since the encryption job can create several messages,
  // but that is a detail. For this, the encryption job actually creates several copies of
  // theMessage)

  // create informative header for those that have no mime-capable
  // email client
  theMessage.setBody( "This message is in MIME format." );

  // set the main headers
  QString oldContentType = theMessage.headerField( "Content-Type" );
  theMessage.deleteBodyParts();
  theMessage.removeHeaderField( "Content-Type" );
  theMessage.removeHeaderField( "Content-Transfer-Encoding" );
  theMessage.setAutomaticFields( true ); // == multipart/mixed

  // this is the boundary depth of the surrounding MIME part
  mPreviousBoundaryLevel = 0;
  mSaveBoundary.clear();

  // whether the body must be signed/encrypted
  const bool doEncryptBody = doEncrypt && mEncryptBody;
  const bool doSignBody = doSign && mSignBody;

  // Find out the values for mEarlyAddAttachments and mAllAttachmentsAreInBody.
  // If an attachment has the same signing / encryption policy as the composer text,
  // then those attachments are included in the body we create here, otherwise
  // they are added to the body afterwards in addBodyAndAttachments(), which is called
  // after the main body was signed and encrypted.
  //
  // If mEarlyAddAttachments is true, we have at least one attachment which is added
  // to the body here.
  // If mAllAttachmentsAreInBody is true, all of the attachments are added here.
  mEarlyAddAttachments = !mAttachments.empty() && ( doSignBody || doEncryptBody );
  mAllAttachmentsAreInBody = mEarlyAddAttachments;
  if ( mEarlyAddAttachments ) {
    bool someOk = false;
    for ( QVector<Attachment>::const_iterator it = mAttachments.constBegin();
          it != mAttachments.constEnd(); ++it ) {
      if ( it->encrypt == doEncryptBody && it->sign == doSignBody ) {
        someOk = true;
      } else {
        mAllAttachmentsAreInBody = false;
      }
    }
    if ( !mAllAttachmentsAreInBody && !someOk ) {
      mEarlyAddAttachments = false;
    }
  }
  kDebug() << "mEarlyAddAttachments=" << mEarlyAddAttachments
           << "mAllAttachmentsAreInBody=" << mAllAttachmentsAreInBody;

  // Prepare attachments that will be signed/encrypted.
  // If necessary, the attachment body is re-encoded here for signing/encrypting.
  // This does not change the body of the main body part.
  for ( QVector<Attachment>::const_iterator it = mAttachments.constBegin();
        it != mAttachments.constEnd(); ++it ) {
    // signed/encrypted body parts must be either QP or base64 encoded
    // Why not 7 bit? Because the LF->CRLF canonicalization would render
    // e.g. 7 bit encoded shell scripts unusable because of the CRs.
    //
    // (marc) this is a workaround for the KMail bug that doesn't
    // respect the CRLF->LF de-canonicalisation. We should
    // eventually get rid of this:
    if ( it->sign || it->encrypt ) {
      QByteArray cte = it->part->cteStr().toLower();
      if ( ( "8bit" == cte && it->part->type() != DwMime::kTypeMessage ) ||
           ( ( it->part->type() == DwMime::kTypeText ) && ( "7bit" == cte ) ) ) {
        const QByteArray body = it->part->bodyDecodedBinary();
        QList<int> dummy;
        it->part->setBodyAndGuessCte( body, dummy, false, it->sign );
        kDebug(5006) <<"Changed encoding of message part from"
                     << cte << "to" << it->part->cteStr();
      }
    }
  }

  //
  // Now, create the core body part by calling all helper functions.
  //
  DwBodyPart * theInnerBodyPart = innerBodypart( theMessage, doSign, oldContentType );
  DwBodyPart * theImageBodyPart = imageBodyPart( theMessage, theInnerBodyPart );
  DwBodyPart * theMixedBodyPart = mixedBodyPart( theMessage, theImageBodyPart,
                                                 doSignBody, doEncryptBody );

  // Set the saved boundary, so that the header will have the same boundary information in the
  // content-type header like in the body.
  if ( mSaveBoundary.length() > 0 ) {
    theMixedBodyPart->Headers().ContentType().SetBoundary( mSaveBoundary );
    theMixedBodyPart->Assemble();
  }

  //
  // Ok, we finished creating our main body part here, now actually set it as the body text
  // of mOldBodyPart.
  // Everything after here deals with signing/encrypting and late attachments.
  //
  mOldBodyPart.setTypeStr( theMixedBodyPart->Headers().ContentType().TypeStr().c_str() );
  mOldBodyPart.setSubtypeStr( theMixedBodyPart->Headers().ContentType().SubtypeStr().c_str() );
  mOldBodyPart.setOriginalContentTypeStr( theMixedBodyPart->Headers().ContentType().AsString().c_str() );
  mOldBodyPart.setContentTransferEncodingStr( theMixedBodyPart->Headers().ContentTransferEncoding().AsString().c_str() );
  mOldBodyPart.setContentDescription( theMixedBodyPart->Headers().ContentDescription().AsString().c_str() );
  mOldBodyPart.setBody( theMixedBodyPart->Body().AsString().c_str() );

  delete theInnerBodyPart;
  if ( theInnerBodyPart != theImageBodyPart )
    delete theImageBodyPart;
  if ( theMixedBodyPart != theImageBodyPart && theMixedBodyPart != theInnerBodyPart )
    delete theMixedBodyPart;

  // This will be our *final* body part, which contains the main body part (mOldBodyPart) plus
  // signing, encryption and late attachments.
  mNewBodyPart = new KMMessagePart;

  // If we want to sign our main body part, get string representation of it, which we save
  // into mEncodedBody
  if ( doSignBody || doEncryptBody ) {

    DwBodyPart *dwPart;
    dwPart = theMessage.createDWBodyPart( &mOldBodyPart );

    // Make sure to set the boundary and content-type of the body part,
    // that is the only thing not done in createDWBodyPart()
    DwHeaders &headers = dwPart->Headers();
    headers.ContentType().FromString( mOldBodyPart.originalContentTypeStr() );
    headers.ContentType().Parse();
    dwPart->Assemble();

    mEncodedBody = dwPart->AsString().c_str();
    delete dwPart;
    dwPart = 0;

    // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
    // according to RfC 2633, 3.1.1 Canonicalization
    //kDebug(5006) <<"Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)";
    mEncodedBody = KMail::Util::lf2crlf( mEncodedBody );
  }

  // Now actually do the signing of the main body part. The signed main body part will
  // be stored in mNewBodyPart.
  if ( doSignBody ) {

    // this lets the KMComposeWin know if it is safe to close the window.
    mPerformingSignOperation = true;

    pgpSignedMsg( mEncodedBody, format );
    mPerformingSignOperation = false;

    if ( mSignature.isEmpty() ) {
      kDebug(5006) <<"signature was empty";
      mRc = false;
      return;
    }
    mRc = processStructuringInfo( QString(),
                                  mOldBodyPart.contentDescription(),
                                  mOldBodyPart.typeStr(),
                                  mOldBodyPart.subtypeStr(),
                                  mOldBodyPart.contentDisposition(),
                                  mOldBodyPart.contentTransferEncodingStr(),
                                  mEncodedBody, "signature",
                                  mSignature,
                                  *mNewBodyPart, true, format );
    if ( mRc ) {
      if ( !makeMultiPartSigned( format ) ) {
        mNewBodyPart->setCharset( mCharset );
      }
    } else {
      KMessageBox::sorry( mComposeWin,
                          mErrorProcessingStructuringInfo );
    }
  }

  if ( !mRc ) {
    return;
  }

  // Now that we have created the main body part and signed it, continue with the rest, which
  // is encryption and adding late attachments.
  continueComposeMessage( theMessage, doSign, doEncrypt, format );
}

QByteArray MessageComposer::innerBodypartBody( KMMessage &theMessage, bool doSign )
{
  QByteArray body;
  if ( !mIsRichText ) {
    body = mPlainText;
  }
  else {

    // Ok, here we add a text/plain and a text/html part to the main body,
    // seperated by a boundary.

    // Calculate a boundary string
    QByteArray boundaryCStr;  // storing boundary string data
    DwMediaType tmpCT;
    tmpCT.CreateBoundary( ++mPreviousBoundaryLevel ); // was 0
    boundaryCStr = tmpCT.Boundary().c_str();

    //
    // Create a KMMessagePart for the text/plain part, and add mPlainText as the body of it.
    // Then, add the complete part to 'body', together with the boundary.
    //
    KMMessagePart textBodyPart;
    textBodyPart.setTypeStr( "text" );
    textBodyPart.setSubtypeStr( "plain" );

    // the signed body must not be 8bit encoded
    QList<int> allowedCTEs;
    textBodyPart.setBodyAndGuessCte(
      mPlainText, allowedCTEs,
      !kmkernel->msgSender()->sendQuotedPrintable() && !doSign, doSign );
    textBodyPart.setCharset( mCharset );
    textBodyPart.setBodyEncoded( mPlainText );
    DwBodyPart *textDwPart = theMessage.createDWBodyPart( &textBodyPart );
    textDwPart->Assemble();
    body += "--";
    body +=     boundaryCStr;
    body +=                 '\n';
    body += textDwPart->AsString().c_str();
    delete textDwPart;
    textDwPart = 0;

    //
    // Create a KMMessagePart for the text/plain part, and add mHtmlSource as the body of it.
    // Then, add the complete part to 'body', together with the boundary.
    //
    KMMessagePart htmlBodyPart;
    htmlBodyPart.setTypeStr( "text" );
    htmlBodyPart.setSubtypeStr( "html" );
    QByteArray htmlbody = mHtmlSource;

    // For all embedded images, replace the image name in the <img> tag with cid:content-id,
    // so that the HTML references the image body parts, see RFC 2557.
    if ( mEmbeddedImages.size() > 0 ) {
      foreach( const KMail::EmbeddedImage *image, mEmbeddedImages ) {
        QString newImageName = "cid:" + image->contentID.toLocal8Bit();
        htmlbody.replace( QByteArray( "\"" + image->imageName.toLocal8Bit() + "\"" ),
                          QByteArray( "\"" + newImageName.toLocal8Bit() ) + "\"" );
      }
    }

    // the signed body must not be 8bit encoded
    htmlBodyPart.setBodyAndGuessCte(
      htmlbody, allowedCTEs,
      !kmkernel->msgSender()->sendQuotedPrintable() && !doSign, doSign );
    htmlBodyPart.setCharset( mCharset );
    htmlBodyPart.setBodyEncoded( htmlbody );
    DwBodyPart *htmlDwPart = theMessage.createDWBodyPart( &htmlBodyPart );
    htmlDwPart->Assemble();
    body += "\n--";
    body +=      boundaryCStr;
    body +=                  '\n';
    body += htmlDwPart->AsString().c_str();
    delete htmlDwPart;
    htmlDwPart = 0;

    body += "--";
    body +=    boundaryCStr;
    body +=                "--\n";

    // we'll need this boundary for the header when there are no embedded images, and no attachments
    mSaveBoundary = tmpCT.Boundary();
  }

  return body;
}

DwBodyPart* MessageComposer::innerBodypart( KMMessage &theMessage, bool doSign,
                                            QString oldContentType )
{
  KMMessagePart innerBodyPart;
  if ( mIsRichText ) {
    innerBodyPart.setTypeStr( "multipart" );
    innerBodyPart.setSubtypeStr( "alternative" );
  }
  innerBodyPart.setContentDisposition( "inline" );

  // Set the body of the body part
  QList<int> allowedCTEs;
  QByteArray body = innerBodypartBody( theMessage, doSign );
  // the signed body must not be 8bit encoded
  innerBodyPart.setBodyAndGuessCte( body, allowedCTEs,
                                    !kmkernel->msgSender()->sendQuotedPrintable() && !doSign,
                                    doSign );
  if ( !mIsRichText ) {
    innerBodyPart.setCharset( mCharset );
  }

  // Now, create a DwBodyPart of the KMMessagePart and return its string representation.
  DwBodyPart* innerDwPart = theMessage.createDWBodyPart( &innerBodyPart );
  DwHeaders &headers = innerDwPart->Headers();
  DwMediaType &ct = headers.ContentType();

  // Set the old content-type, if we have one. This happens with inline invitations.
  if ( !mIsRichText && !oldContentType.isEmpty() ) {
    ct.FromString( oldContentType.toUtf8() );
    ct.Parse();
  }

  // add the boundary in the MPA header
  if ( mIsRichText ) {
    ct.SetBoundary( mSaveBoundary );
  }

  innerDwPart->Assemble();
  return innerDwPart;
}

DwBodyPart* MessageComposer::imageBodyPart( KMMessage &theMessage, DwBodyPart *innerBodyPart )
{
  if ( mEmbeddedImages.size() == 0 )
    return innerBodyPart;

  // Get a boundary string
  QByteArray multipartRelatedBoundary;
  DwMediaType tmpCT;
  tmpCT.CreateBoundary( ++mPreviousBoundaryLevel );
  multipartRelatedBoundary = tmpCT.Boundary().c_str();

  // Create the body part
  KMMessagePart imageBodyPart;
  imageBodyPart.setTypeStr( "multipart" );
  imageBodyPart.setSubtypeStr( "related" );
  imageBodyPart.setContentDisposition( "inline" );

  // Create the body of the body part, which contains all images seperated by boundaries, and
  // the old multipart/alternative body
  QByteArray imageBodyPartBody;
  imageBodyPartBody += "\n--";
  imageBodyPartBody +=     multipartRelatedBoundary;
  imageBodyPartBody +=                            '\n';
  imageBodyPartBody += innerBodyPart->AsString().c_str();
  foreach ( const KMail::EmbeddedImage *image, mEmbeddedImages )
  {
    // Create the KMMessagePart and add the encoded image to it.
    KMMessagePart singleImageBodyPart;
    singleImageBodyPart.setName( image->imageName.toLocal8Bit() );
    singleImageBodyPart.setBodyEncodedBinary( image->image );
    singleImageBodyPart.setContentTransferEncodingStr( "base64" );
    singleImageBodyPart.setTypeStr( "image" );
    singleImageBodyPart.setSubtypeStr( "png" );
    QString contentId = "<" + image->contentID + ">";
    singleImageBodyPart.setContentId( QByteArray( contentId.toLocal8Bit() ) );

    // Create a DwBodyPart out of the KMMessagePart, get its string representation, and add
    // its string representation and the boundary to the body part body of the multipart/related
    // body part.
    DwBodyPart* singleImageDwPart = theMessage.createDWBodyPart( &singleImageBodyPart );
    singleImageDwPart->Assemble();
    imageBodyPartBody += "\n--";
    imageBodyPartBody +=     multipartRelatedBoundary;
    imageBodyPartBody +=                            '\n';
    imageBodyPartBody += singleImageDwPart->AsString().c_str();
    delete singleImageDwPart;
  }
  imageBodyPartBody += "\n--";
  imageBodyPartBody +=     multipartRelatedBoundary;
  imageBodyPartBody +=                            "--\n";

  // Add the body part body to the body part
  imageBodyPart.setBodyEncoded( imageBodyPartBody );

  // Now, create a DwBodyPart of the KMMessagePart and return its string representation.
  DwBodyPart* imageDwPart = theMessage.createDWBodyPart( &imageBodyPart );
  DwHeaders &headers = imageDwPart->Headers();
  DwMediaType &ct = headers.ContentType();
  ct.SetBoundary( tmpCT.Boundary() );
  imageDwPart->Assemble();

  mSaveBoundary = tmpCT.Boundary();
  return imageDwPart;
}

DwBodyPart* MessageComposer::mixedBodyPart( KMMessage &theMessage, DwBodyPart *imageBodyPart,
                                            bool doSignBody, bool doEncryptBody )
{
  if ( !mEarlyAddAttachments )
    return imageBodyPart;

  // Get a boundary string
  QByteArray multipartMixedBoundary;
  DwMediaType tmpCT;
  tmpCT.CreateBoundary( ++mPreviousBoundaryLevel );
  multipartMixedBoundary = tmpCT.Boundary().c_str();

  // Create the body part
  KMMessagePart mixedBodyPart;
  mixedBodyPart.setTypeStr( "multipart" );
  mixedBodyPart.setSubtypeStr( "mixed" );
  mixedBodyPart.setContentDisposition( "inline" );

  // Create the body of the body part, which contains all attachments seperated by boundaries, and
  // the old image body part
  QByteArray mixedBodyPartBody;
  mixedBodyPartBody += "\n--";
  mixedBodyPartBody +=     multipartMixedBoundary;
  mixedBodyPartBody +=                            '\n';
  mixedBodyPartBody += imageBodyPart->AsString().c_str();
  for ( QVector<Attachment>::iterator it = mAttachments.begin();
        it != mAttachments.end(); ++it ) {
    if ( it->encrypt == doEncryptBody && it->sign == doSignBody ) {
      DwBodyPart* attachDwPart = theMessage.createDWBodyPart( it->part );
      attachDwPart->Assemble();
      mixedBodyPartBody += "\n--";
      mixedBodyPartBody +=     multipartMixedBoundary;
      mixedBodyPartBody +=                            '\n';
      mixedBodyPartBody += attachDwPart->AsString().c_str();
      delete attachDwPart;
    }
  }
  mixedBodyPartBody += "\n--";
  mixedBodyPartBody +=     multipartMixedBoundary;
  mixedBodyPartBody +=                            "--\n";

  // Add the body part body to the body part
  mixedBodyPart.setBodyEncoded( mixedBodyPartBody );

  // Now, create a DwBodyPart of the KMMessagePart and return its string representation.
  DwBodyPart* mixedDwPart = theMessage.createDWBodyPart( &mixedBodyPart );
  DwHeaders &headers = mixedDwPart->Headers();
  DwMediaType &ct = headers.ContentType();
  ct.SetBoundary( tmpCT.Boundary() );
  mixedDwPart->Assemble();

  mSaveBoundary = tmpCT.Boundary();
  return mixedDwPart;
}

void MessageComposer::continueComposeMessage( KMMessage &theMessage,
                                              bool doSign, bool doEncrypt,
                                              Kleo::CryptoMessageFormat format )
{

  const std::vector<Kleo::KeyResolver::SplitInfo> splitInfos =
    mKeyResolver->encryptionItems( format );
  kWarning( splitInfos.empty() )
    << "MessageComposer::continueComposeMessage(): splitInfos.empty() for"
    << Kleo::cryptoMessageFormatToString( format );

  if ( !splitInfos.empty() && doEncrypt && !saveMessagesEncrypted() ) {
    mJobs.push_front( new SetLastMessageAsUnencryptedVersionOfLastButOne( this ) );
    mJobs.push_front( new EncryptMessageJob(
                        new KMMessage( theMessage ),
                        Kleo::KeyResolver::SplitInfo( splitInfos.front().recipients ), doSign,
                        false, mEncodedBody,
                        mPreviousBoundaryLevel,
                        mOldBodyPart, mNewBodyPart,
                        format, this ) );
  }

  for ( std::vector<Kleo::KeyResolver::SplitInfo>::const_iterator it = splitInfos.begin();
        it != splitInfos.end(); ++it ) {
    mJobs.push_front( new EncryptMessageJob(
                        new KMMessage( theMessage ), *it, doSign,
                        doEncrypt, mEncodedBody,
                        mPreviousBoundaryLevel,
                        mOldBodyPart, mNewBodyPart,
                        format, this ) );
  }
}

void MessageComposer::encryptMessage( KMMessage *msg,
                                      const Kleo::KeyResolver::SplitInfo &splitInfo,
                                      bool doSign, bool doEncrypt,
                                      KMMessagePart newBodyPart,
                                      Kleo::CryptoMessageFormat format )
{
  if ( doEncrypt && splitInfo.keys.empty() ) {
    // the user wants to send the message unencrypted
    //mComposeWin->setEncryption( false, false );
    //FIXME why is this talkback needed? Till
    doEncrypt = false;
  }

  const bool doEncryptBody = doEncrypt && mEncryptBody;
  const bool doSignBody = doSign && mSignBody;

  if ( doEncryptBody ) {
    QByteArray innerContent;
    if ( doSignBody ) {
      // extract signed body from newBodyPart
      DwBodyPart *dwPart = msg->createDWBodyPart( &newBodyPart );
      dwPart->Assemble();
      innerContent = dwPart->AsString().c_str();
      delete dwPart;
      dwPart = 0;
    } else {
      innerContent = mEncodedBody;
    }

    // now do the encrypting:
    // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
    // according to RfC 2633, 3.1.1 Canonicalization
    //kDebug(5006) <<"Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)";
    innerContent = KMail::Util::lf2crlf( innerContent );
    //kDebug(5006) <<"                                                       done.";

    QByteArray encryptedBody;
    Kpgp::Result result = pgpEncryptedMsg( encryptedBody, innerContent,
                                           splitInfo.keys, format );
    if ( result != Kpgp::Ok ) {
      mRc = false;
      return;
    }
    mRc = processStructuringInfo( "http://www.gnupg.org/aegypten/",
                                  newBodyPart.contentDescription(),
                                  newBodyPart.typeStr(),
                                  newBodyPart.subtypeStr(),
                                  newBodyPart.contentDisposition(),
                                  newBodyPart.contentTransferEncodingStr(),
                                  innerContent,
                                  "encrypted data",
                                  encryptedBody,
                                  newBodyPart, false, format );
    if ( !mRc ) {
      KMessageBox::sorry( mComposeWin, mErrorProcessingStructuringInfo );
    }
  }

  // process the attachments that are not included into the body
  if ( mRc ) {
    const bool useNewBodyPart = doSignBody || doEncryptBody;
    addBodyAndAttachments( msg, splitInfo, doSign, doEncrypt,
                           useNewBodyPart ? newBodyPart : mOldBodyPart, format );
  }
}

void MessageComposer::addBodyAndAttachments( KMMessage *msg,
                                             const Kleo::KeyResolver::SplitInfo &splitInfo,
                                             bool doSign, bool doEncrypt,
                                             const KMMessagePart &ourFineBodyPart,
                                             Kleo::CryptoMessageFormat format )
{
  const bool doEncryptBody = doEncrypt && mEncryptBody;
  const bool doSignBody = doSign && mSignBody;

  if ( !mAttachments.empty() &&
       ( !mEarlyAddAttachments || !mAllAttachmentsAreInBody ) ) {
    // set the content type header
    msg->headers().ContentType().SetType( DwMime::kTypeMultipart );
    msg->headers().ContentType().SetSubtype( DwMime::kSubtypeMixed );
    msg->headers().ContentType().CreateBoundary( 0 );
    kDebug(5006) << "Set top level Content-Type to Multipart/Mixed";

    // add our Body Part
    DwBodyPart *tmpDwPart = msg->createDWBodyPart( &ourFineBodyPart );
    DwHeaders &headers = tmpDwPart->Headers();
    DwMediaType &ct = headers.ContentType();
    if ( !mSaveBoundary.empty() ) {
      ct.SetBoundary( mSaveBoundary );
    }
    tmpDwPart->Assemble();

    //KMMessagePart newPart;
    //newPart.setBody(tmpDwPart->AsString().c_str());
    msg->addDwBodyPart( tmpDwPart ); // only this method doesn't add it as text/plain

    // add Attachments
    // create additional bodyparts for the attachments (if any)
    KMMessagePart newAttachPart;
    for ( QVector<Attachment>::iterator it = mAttachments.begin();
          it != mAttachments.end(); ++it ) {

      const bool cryptFlagsDifferent = ( it->encrypt != doEncryptBody || it->sign != doSignBody );

      if ( !cryptFlagsDifferent && mEarlyAddAttachments ) {
        continue;
      }

      const bool encryptThisNow = doEncrypt && cryptFlagsDifferent && it->encrypt;
      const bool signThisNow = doSign && cryptFlagsDifferent && it->sign;

      if ( !encryptThisNow && !signThisNow ) {
        msg->addBodyPart( it->part );
        // Assemble the message. Not sure why, but this fixes the vanishing boundary parameter
        (void)msg->asDwMessage();
        continue;
      }

      KMMessagePart &rEncryptMessagePart( *it->part );

      DwBodyPart *innerDwPart = msg->createDWBodyPart( it->part );
      innerDwPart->Assemble();
      QByteArray encodedAttachment = innerDwPart->AsString().c_str();
      delete innerDwPart;
      innerDwPart = 0;

      // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
      // according to RfC 2633, 3.1.1 Canonicalization
      //kDebug(5006) <<"Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)";
      encodedAttachment = KMail::Util::lf2crlf( encodedAttachment );

      // sign this attachment
      if ( signThisNow ) {
        pgpSignedMsg( encodedAttachment, format );
        QByteArray signature = mSignature;
        mRc = !signature.isEmpty();
        if ( mRc ) {
          mRc = processStructuringInfo( "http://www.gnupg.org/aegypten/",
                                        it->part->contentDescription(),
                                        it->part->typeStr(),
                                        it->part->subtypeStr(),
                                        it->part->contentDisposition(),
                                        it->part->contentTransferEncodingStr(),
                                        encodedAttachment,
                                        "signature",
                                        signature,
                                        newAttachPart, true, format );
          if ( mRc ) {
            if ( encryptThisNow ) {
              rEncryptMessagePart = newAttachPart;
              DwBodyPart *dwPart = msg->createDWBodyPart( &newAttachPart );
              dwPart->Assemble();
              encodedAttachment = dwPart->AsString().c_str();
              delete dwPart;
              dwPart = 0;
            }
          } else {
            KMessageBox::sorry( mComposeWin, mErrorProcessingStructuringInfo );
          }
        } else {
          // quit the attachments' loop
          break;
        }
      }
      if ( encryptThisNow ) {
        QByteArray encryptedBody;
        Kpgp::Result result = pgpEncryptedMsg( encryptedBody,
                                               encodedAttachment,
                                               splitInfo.keys,
                                               format );

        if ( Kpgp::Ok == result ) {
          mRc = processStructuringInfo( "http://www.gnupg.org/aegypten/",
                                        rEncryptMessagePart.contentDescription(),
                                        rEncryptMessagePart.typeStr(),
                                        rEncryptMessagePart.subtypeStr(),
                                        rEncryptMessagePart.contentDisposition(),
                                        rEncryptMessagePart.contentTransferEncodingStr(),
                                        encodedAttachment,
                                        "encrypted data",
                                        encryptedBody,
                                        newAttachPart, false, format );
          if ( !mRc ) {
            KMessageBox::sorry( mComposeWin, mErrorProcessingStructuringInfo );
          }
        } else {
          mRc = false;
        }
      }
      msg->addBodyPart( &newAttachPart );

      // Assemble the message. One gets a completely empty message otherwise :/
      (void)msg->asDwMessage();
    }
  } else {
    // no attachments in the final message
    if ( !ourFineBodyPart.originalContentTypeStr().isEmpty() ) {
      msg->headers().ContentType().FromString( ourFineBodyPart.originalContentTypeStr() );
      msg->headers().ContentType().Parse();
      kDebug(5006) << "Set top level Content-Type from originalContentTypeStr()="
                   << ourFineBodyPart.originalContentTypeStr();
    } else {
      msg->headers().ContentType().FromString(
        ourFineBodyPart.typeStr() + '/' + ourFineBodyPart.subtypeStr() );
      kDebug(5006) << "Set top level Content-Type to"
                   << ourFineBodyPart.typeStr() << "/" << ourFineBodyPart.subtypeStr();
    }
    if ( !ourFineBodyPart.charset().isEmpty() ) {
      msg->setCharset( ourFineBodyPart.charset() );
    }
    msg->setHeaderField( "Content-Transfer-Encoding",
                         ourFineBodyPart.contentTransferEncodingStr() );
    msg->setHeaderField( "Content-Description",
                         ourFineBodyPart.contentDescription() );
    msg->setHeaderField( "Content-Disposition",
                         ourFineBodyPart.contentDisposition() );

    if ( mDebugComposerCrypto ) {
      kDebug(5006) << "Top level headers and body adjusted";
    }

    // set body content
    if ( mIsRichText && !( doSign || doEncrypt ) ) { // add the boundary to the header
      msg->headers().ContentType().SetBoundary( mSaveBoundary );
      msg->headers().ContentType().Assemble();
    }
    msg->setBody( ourFineBodyPart.body() );

  }

  msg->setHeaderField( "X-KMail-Recipients",
                       splitInfo.recipients.join( ", " ), KMMessage::Address );

  if ( mDebugComposerCrypto ) {
    kDebug(5006) << "Final message:\n|||" << msg->asString() << "|||\n\n";
    msg->headers().Assemble();
    kDebug(5006) << "\n\n\nFinal headers:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n";
  }
}

//-----------------------------------------------------------------------------
// This method does not call any crypto ops, so it does not need to be async
bool MessageComposer::processStructuringInfo( const QString bugURL,
                                              const QString contentDescClear,
                                              const QByteArray contentTypeClear,
                                              const QByteArray contentSubtypeClear,
                                              const QByteArray contentDispClear,
                                              const QByteArray contentTEncClear,
                                              const QByteArray &clearCStr,
                                              const QString contentDescCiph,
                                              const QByteArray &ciphertext,
                                              KMMessagePart &resultingPart,
                                              bool signing, Kleo::CryptoMessageFormat format ) const
{
  Q_UNUSED( contentDescCiph );
  bool bOk = true;

  if ( makeMimeObject( format, signing ) ) {
    QByteArray mainHeader = "Content-Type: ";
    const char *toplevelCT = toplevelContentType( format, signing );
    if ( toplevelCT ) {
      mainHeader += toplevelCT;
    } else {
      if ( makeMultiMime( format, signing ) ) {
        mainHeader += "text/plain";
      } else {
        mainHeader += contentTypeClear + '/' + contentSubtypeClear;
      }
    }

    const QByteArray boundaryCStr = KMime::multiPartBoundary();
    // add "boundary" parameter
    if ( makeMultiMime( format, signing ) ) {
      mainHeader.replace( "%boundary", boundaryCStr );
    }

    if ( toplevelCT ) {
      if ( const char *str = toplevelContentDisposition( format, signing ) ) {
        mainHeader += "\nContent-Disposition: ";
        mainHeader += str;
      }
      if ( !makeMultiMime( format, signing ) && binaryHint( format ) ) {
        mainHeader += "\nContent-Transfer-Encoding: base64";
      }
    } else {
      if ( 0 < contentDispClear.length() ) {
        mainHeader += "\nContent-Disposition: ";
        mainHeader += contentDispClear;
      }
      if ( 0 < contentTEncClear.length() ) {
        mainHeader += "\nContent-Transfer-Encoding: ";
        mainHeader += contentTEncClear;
      }
    }

    //kDebug(5006) <<"processStructuringInfo: mainHeader=" << mainHeader;

    DwString mainDwStr = KMail::Util::dwString( mainHeader );
    mainDwStr += "\n\n";
    DwBodyPart mainDwPa( mainDwStr, 0 );
    mainDwPa.Parse();
    KMMessage::bodyPart( &mainDwPa, &resultingPart );
    if ( !makeMultiMime( format, signing ) ) {
      if ( signing && includeCleartextWhenSigning( format ) ) {
        QByteArray bodyText( clearCStr );
        bodyText += '\n';
        bodyText += ciphertext;
        resultingPart.setBodyEncoded( bodyText );
      } else {
        resultingPart.setBodyEncodedBinary( ciphertext );
      }
    } else {
      // Build the encapsulated MIME parts.
      // Build a MIME part holding the version information
      // taking the body contents returned in
      // structuring.data.bodyTextVersion.
      QByteArray versCStr, codeCStr;
      if ( !signing && format == Kleo::OpenPGPMIMEFormat ) {
        versCStr =
          "Content-Type: application/pgp-encrypted\n"
          "Content-Disposition: attachment\n"
          "\n"
          "Version: 1";
      }

      // Build a MIME part holding the code information
      // taking the body contents returned in ciphertext.
      const char *nestedCT = nestedContentType( format, signing );
      assert( nestedCT );
      codeCStr = "Content-Type: ";
      codeCStr += nestedCT;
      codeCStr += '\n';
      if ( const char *str = nestedContentDisposition( format, signing ) ) {
        codeCStr += "Content-Disposition: ";
        codeCStr += str;
        codeCStr += '\n';
      }
      if ( binaryHint( format ) ) {
        codeCStr += "Content-Transfer-Encoding: base64\n\n";
        codeCStr += KMime::Codec::codecForName( "base64" )->encode( ciphertext );
      } else {
        codeCStr += '\n' + ciphertext;
      }

      QByteArray mainStr = "--" + boundaryCStr;
      if ( signing && includeCleartextWhenSigning( format ) &&
           !clearCStr.isEmpty() ) {
        mainStr += '\n' + clearCStr + "\n--" + boundaryCStr;
      }
      if ( !versCStr.isEmpty() ) {
        mainStr += '\n' + versCStr + "\n--" + boundaryCStr;
      }
      if ( !codeCStr.isEmpty() ) {
        mainStr += '\n' + codeCStr + "\n--" + boundaryCStr;
      }
      mainStr += "--\n";

      //kDebug(5006) <<"processStructuringInfo: mainStr=" << mainStr;
      resultingPart.setBodyEncoded( mainStr );
    }

  } else { //  not making a mime object, build a plain message body.

    resultingPart.setContentDescription( contentDescClear );
    resultingPart.setTypeStr( contentTypeClear );
    resultingPart.setSubtypeStr( contentSubtypeClear );
    resultingPart.setContentDisposition( contentDispClear );
    resultingPart.setContentTransferEncodingStr( contentTEncClear );
    QByteArray resultingBody;

    if ( signing && includeCleartextWhenSigning( format ) ) {
      if ( !clearCStr.isEmpty() ) {
        resultingBody += clearCStr;
      }
    }
    if ( !ciphertext.isEmpty() ) {
      resultingBody += ciphertext;
    } else {
      // Plugin error!
      KMessageBox::sorry( mComposeWin,
                          i18n( "<qt><p>Error: The backend did not return "
                                "any encoded data.</p>"
                                "<p>Please report this bug:<br />%1</p></qt>",
                                bugURL ) );
      bOk = false;
    }
    resultingPart.setBodyEncoded( resultingBody );
  }

  return bOk;
}

//-----------------------------------------------------------------------------
bool MessageComposer::autoDetectCharset()
{
  QByteArray charset =
      KMMsgBase::autoDetectCharset( mCharset, KMMessage::preferredCharsets(),
                                    mComposeWin->mEditor->textOrHtml() );
  if ( charset.isEmpty() ) {
    KMessageBox::sorry( mComposeWin,
                        i18n( "No suitable encoding could be found for "
                              "your message.\nPlease set an encoding "
                              "using the 'Options' menu." ) );
    return false;
  }
  mCharset = charset;
  return true;
}

//-----------------------------------------------------------------------------
bool MessageComposer::getSourceText( QByteArray &plainTextEncoded, QByteArray &htmlSourceEncoded ) const
{
  // Get the HTML source and the plain text, with proper breaking
  //
  // Note that previous versions of KMail only stored mBodyText and then used
  // a hidden edit widget to remove the formatting in case mBodyText was HTML,
  // but this was not reliable as the hidden edit widget introduced bugs such
  // as http://bugs.kde.org/show_bug.cgi?id=165190.
  //
  // Therefore, we now read the plain text version directly from the composer,
  // which returns the correct result.
  QString htmlSource, plainText, newPlainText;
  htmlSource = mComposeWin->mEditor->toCleanHtml();

  if ( mDisableBreaking || !GlobalSettings::self()->wordWrap() )
    plainText = mComposeWin->mEditor->toCleanPlainText();
  else
    plainText = mComposeWin->mEditor->toWrappedPlainText();

  // Now, convert the string to a bytearray with the right codec.
  const QTextCodec *codec = KMMsgBase::codecForName( mCharset );
  if ( mCharset == "us-ascii" ) {
    plainTextEncoded = KMMsgBase::toUsAscii( plainText );
    htmlSourceEncoded = KMMsgBase::toUsAscii( htmlSource );
    newPlainText = QString::fromLatin1( plainTextEncoded );
  } else if ( codec == 0 ) {
    kDebug() << "Something is wrong and I can not get a codec.";
    plainTextEncoded = plainText.toLocal8Bit();
    htmlSourceEncoded = htmlSource.toLocal8Bit();
    newPlainText = QString::fromLocal8Bit( plainTextEncoded );
  } else {
    plainTextEncoded = codec->fromUnicode( plainText );
    htmlSourceEncoded = codec->fromUnicode( htmlSource );
    newPlainText = codec->toUnicode( plainTextEncoded );
  }

  // Check if we didn't loose any characters during encoding.
  // This can be checked by decoding the encoded text and comparing it with the
  // original, it should be the same.
  if ( !newPlainText.isEmpty() && ( newPlainText != plainText ) )
    return false;
  else
    return true;
}

//-----------------------------------------------------------------------------
bool MessageComposer::breakLinesAndApplyCodec()
{
  // Get the encoded text and HTML
  QByteArray plainTextEncoded, htmlSourceEncoded;
  bool ok = getSourceText( plainTextEncoded, htmlSourceEncoded );

  // Not all chars fit into the current charset, ask the user what to do about it.
  if ( !ok ) {
    KCursorSaver idle( KBusyPtr::idle() );
    bool loseChars = ( KMessageBox::warningYesNo(
                       mComposeWin,
                       i18n("<qt>Not all characters fit into the chosen"
                            " encoding.<br /><br />Send the message anyway?</qt>"),
                       i18n("Some Characters Will Be Lost"),
                       KGuiItem ( i18n("Lose Characters") ),
                       KGuiItem( i18n("Change Encoding") ) ) == KMessageBox::Yes );
    if ( loseChars ) {
      const QTextCodec *codec = KMMsgBase::codecForName( mCharset );
      mComposeWin->mEditor->replaceUnknownChars( codec );
    }
    else {
      // The user wants to change the encoding, so auto-detect a correct one.
      bool charsetFound = autoDetectCharset();
      if ( charsetFound ) {
        // Read the values again, this time they should be correct, since we just
        // set the correct charset.
        bool ok = getSourceText( plainTextEncoded, htmlSourceEncoded );
        if ( !ok ) {
          kWarning() << "The autodetected charset is still wrong!";
        }

        // Now, set the charset of the composer, since we just changed it.
        // Useful when this questionbox was brought up by autosave.
        mComposeWin->setCharset( mCharset );
      }
      else { // no charset could be autodetected
        mRc = false;
        return false;
      }
    }
  }

  // From RFC 3156:
  //  Note: The accepted OpenPGP convention is for signed data to end
  //  with a <CR><LF> sequence.  Note that the <CR><LF> sequence
  //  immediately preceding a MIME boundary delimiter line is considered
  //  to be part of the delimiter in [3], 5.1.  Thus, it is not part of
  //  the signed data preceding the delimiter line.  An implementation
  //  which elects to adhere to the OpenPGP convention has to make sure
  //  it inserts a <CR><LF> pair on the last line of the data to be
  //  signed and transmitted (signed message and transmitted message
  //  MUST be identical).
  // So make sure that the body ends with a <LF>.
  if ( plainTextEncoded.isEmpty() || !plainTextEncoded.endsWith('\n') ) {
    kDebug() << "Added an <LF> on the last line";
    plainTextEncoded += '\n';
  }
  if ( htmlSourceEncoded.isEmpty() || !htmlSourceEncoded.endsWith('\n') ) {
    kDebug() << "Added an <LF> on the last line";
    htmlSourceEncoded += '\n';
  }

  mPlainText = plainTextEncoded;
  mHtmlSource = htmlSourceEncoded;
  if ( mIsRichText )
    mBodyText = mHtmlSource;
  else
    mBodyText = mPlainText;

  return true;
}

//-----------------------------------------------------------------------------
void MessageComposer::pgpSignedMsg( const QByteArray &cText, Kleo::CryptoMessageFormat format ) {

  mSignature = QByteArray();

  const std::vector<GpgME::Key> signingKeys = mKeyResolver->signingKeys( format );
  if ( signingKeys.empty() ) {
    KMessageBox::sorry( mComposeWin, i18n("This message could not be signed, "
                                          "since no valid signing keys have been found; "
                                          "this should actually never happen, "
                                          "please report this bug.") );
    return;
  }

  // TODO: ASync call? Likely, yes :-)
  const Kleo::CryptoBackendFactory *cpf = Kleo::CryptoBackendFactory::instance();
  assert( cpf );
  const Kleo::CryptoBackend::Protocol *proto = isSMIME( format ) ?
                                               cpf->smime() : cpf->openpgp();
  assert( proto );

  std::auto_ptr<Kleo::SignJob> job( proto->signJob( armor( format ),
                                                    textMode( format ) ) );

  if ( !job.get() ) {
    KMessageBox::sorry( mComposeWin,
                        i18n("This message could not be signed, "
                             "since the chosen backend does not seem to support "
                             "signing; this should actually never happen, "
                             "please report this bug.") );
    return;
  }

  QByteArray plainText( cText );
  QByteArray signature;
  const GpgME::SigningResult res =
    job->exec( signingKeys, plainText, signingMode( format ), signature );
  if ( res.error().isCanceled() ) {
    kDebug(5006) <<"signing was canceled by user";
    return;
  }
  if ( res.error() ) {
    kDebug(5006) <<"signing failed:" << res.error().asString();
    job->showErrorDialog( mComposeWin );
    return;
  }

  if ( GlobalSettings::showGnuPGAuditLogAfterSuccessfulSignEncrypt() )
    Kleo::MessageBox::auditLog( 0, job.get(), i18n("GnuPG Audit Log for Signing Operation") );

  mSignature = signature;
  if ( mSignature.isEmpty() ) {
    KMessageBox::sorry( mComposeWin,
                        i18n( "The signing operation failed. "
                              "Please make sure that the gpg-agent program "
                              "is running." ) );
  }
}

//-----------------------------------------------------------------------------
Kpgp::Result MessageComposer::pgpEncryptedMsg( QByteArray &encryptedBody,
                                               const QByteArray &cText,
                                               const std::vector<GpgME::Key> &encryptionKeys,
                                               Kleo::CryptoMessageFormat format ) const
{
  // TODO: ASync call? Likely, yes :-)
  const Kleo::CryptoBackendFactory *cpf = Kleo::CryptoBackendFactory::instance();
  assert( cpf );
  const Kleo::CryptoBackend::Protocol *proto = isSMIME( format ) ?
                                               cpf->smime() : cpf->openpgp();
  assert( proto );

  std::auto_ptr<Kleo::EncryptJob> job( proto->encryptJob( armor( format ),
                                                          textMode( format ) ) );
  if ( !job.get() ) {
    KMessageBox::sorry( mComposeWin,
                        i18n("This message could not be encrypted, "
                             "since the chosen backend does not seem to support "
                             "encryption; this should actually never happen, "
                             "please report this bug.") );
    return Kpgp::Failure;
  }

  QByteArray plainText( cText );

  const GpgME::EncryptionResult res =
    job->exec( encryptionKeys, plainText, true /* we do ownertrust ourselves */, encryptedBody );
  if ( res.error().isCanceled() ) {
    kDebug(5006) <<"encryption was canceled by user";
    return Kpgp::Canceled;
  }
  if ( res.error() ) {
    kDebug(5006) <<"encryption failed:" << res.error().asString();
    job->showErrorDialog( mComposeWin );
    return Kpgp::Failure;
  }

  if ( GlobalSettings::showGnuPGAuditLogAfterSuccessfulSignEncrypt() )
    Kleo::MessageBox::auditLog( 0, job.get(), i18n("GnuPG Audit Log for Encryption Operation") );

  return Kpgp::Ok;
}

Kpgp::Result MessageComposer::pgpSignedAndEncryptedMsg( QByteArray &encryptedBody,
                                                        const QByteArray &cText,
                                                        const std::vector<GpgME::Key> &signingKeys,
                                                        const std::vector<GpgME::Key> &encryptionKeys,
                                                        Kleo::CryptoMessageFormat format ) const
{
  // TODO: ASync call? Likely, yes :-)
  const Kleo::CryptoBackendFactory *cpf = Kleo::CryptoBackendFactory::instance();
  assert( cpf );
  const Kleo::CryptoBackend::Protocol *proto = isSMIME( format ) ?
                                               cpf->smime() : cpf->openpgp();
  assert( proto );

  std::auto_ptr<Kleo::SignEncryptJob> job( proto->signEncryptJob( armor( format ),
                                                                  textMode( format ) ) );
  if ( !job.get() ) {
    KMessageBox::sorry( mComposeWin,
                        i18n("This message could not be signed and encrypted, "
                             "since the chosen backend does not seem to support "
                             "combined signing and encryption; this should actually never happen, "
                             "please report this bug.") );
    return Kpgp::Failure;
  }

  const std::pair<GpgME::SigningResult,GpgME::EncryptionResult> res =
    job->exec( signingKeys, encryptionKeys, cText, false, encryptedBody );
  if ( res.first.error().isCanceled() || res.second.error().isCanceled() ) {
    kDebug(5006) <<"encrypt/sign was canceled by user";
    return Kpgp::Canceled;
  }
  if ( res.first.error() || res.second.error() ) {
    if ( res.first.error() ) {
      kDebug(5006) <<"signing failed:" << res.first.error().asString();
    } else {
      kDebug(5006) <<"encryption failed:" << res.second.error().asString();
    }
    job->showErrorDialog( mComposeWin );
    return Kpgp::Failure;
  }

  if ( GlobalSettings::showGnuPGAuditLogAfterSuccessfulSignEncrypt() )
    Kleo::MessageBox::auditLog( 0, job.get(), i18n("GnuPG Audit Log for Encryption Operation") );

  return Kpgp::Ok;
}

#include "messagecomposer.moc"
