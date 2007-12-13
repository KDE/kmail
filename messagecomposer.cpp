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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "messagecomposer.h"
#include "kmmsgpart.h"
#define REALLY_WANT_KMCOMPOSEWIN_H
#include "kmcomposewin.h"
#undef REALLY_WANT_KMCOMPOSEWIN_H
#include "klistboxdialog.h"
#include "kcursorsaver.h"
#include "messagesender.h"
#include "kmfolder.h"
#include "kmfoldercombobox.h"
#include "keyresolver.h"
#include "kleo_util.h"
#include "globalsettings.h"
#include "custommimeheader.h"
#include "kmedit.h"
#include "util.h"

#include <libkpimidentities/identity.h>
#include <libkpimidentities/identitymanager.h>
#include <libemailfunctions/email.h>

#include <ui/keyselectiondialog.h>
#include <ui/keyapprovaldialog.h>
#include <ui/messagebox.h>
#include <kleo/cryptobackendfactory.h>
#include <kleo/keylistjob.h>
#include <kleo/encryptjob.h>
#include <kleo/signencryptjob.h>
#include <kleo/signjob.h>
#include <kleo/specialjob.h>

#include <kmime_util.h>
#include <kmime_codecs.h>
#include <kpgpblock.h>

#include <mimelib/mimepp.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kinputdialog.h>
#include <kdebug.h>
#include <kaction.h>
#include <qfile.h>
#include <qtextcodec.h>
#include <qtextedit.h>
#include <qtimer.h>

#include <gpgmepp/key.h>
#include <gpgmepp/keylistresult.h>
#include <gpgmepp/encryptionresult.h>
#include <gpgmepp/signingresult.h>
#include <gpgmepp/context.h>

#include <algorithm>
#include <memory>

// ## keep default values in sync with configuredialog.cpp, Security::CryptoTab::setup()
// This should be ported to a .kcfg one day I suppose (dfaure).

static inline bool warnSendUnsigned() {
    KConfigGroup group( KMKernel::config(), "Composer" );
    return group.readBoolEntry( "crypto-warning-unsigned", false );
}
static inline bool warnSendUnencrypted() {
    KConfigGroup group( KMKernel::config(), "Composer" );
    return group.readBoolEntry( "crypto-warning-unencrypted", false );
}
static inline bool saveMessagesEncrypted() {
    KConfigGroup group( KMKernel::config(), "Composer" );
    return group.readBoolEntry( "crypto-store-encrypted", true );
}
static inline bool encryptToSelf() {
    // return !Kpgp::Module::getKpgp() || Kpgp::Module::getKpgp()->encryptToSelf();
    KConfigGroup group( KMKernel::config(), "Composer" );
    return group.readBoolEntry( "crypto-encrypt-to-self", true );
}
static inline bool showKeyApprovalDialog() {
    KConfigGroup group( KMKernel::config(), "Composer" );
    return group.readBoolEntry( "crypto-show-keys-for-approval", true );
}

static inline int encryptKeyNearExpiryWarningThresholdInDays() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readBoolEntry( "crypto-warn-when-near-expire", true ) )
    return -1;
  const int num = composer.readNumEntry( "crypto-warn-encr-key-near-expire-int", 14 );
  return kMax( 1, num );
}

static inline int signingKeyNearExpiryWarningThresholdInDays() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readBoolEntry( "crypto-warn-when-near-expire", true ) )
    return -1;
  const int num = composer.readNumEntry( "crypto-warn-sign-key-near-expire-int", 14 );
  return kMax( 1, num );
}

static inline int encryptRootCertNearExpiryWarningThresholdInDays() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readBoolEntry( "crypto-warn-when-near-expire", true ) )
    return -1;
  const int num = composer.readNumEntry( "crypto-warn-encr-root-near-expire-int", 14 );
  return kMax( 1, num );
}

static inline int signingRootCertNearExpiryWarningThresholdInDays() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readBoolEntry( "crypto-warn-when-near-expire", true ) )
    return -1;
  const int num = composer.readNumEntry( "crypto-warn-sign-root-near-expire-int", 14 );
  return kMax( 1, num );
}

static inline int encryptChainCertNearExpiryWarningThresholdInDays() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readBoolEntry( "crypto-warn-when-near-expire", true ) )
    return -1;
  const int num = composer.readNumEntry( "crypto-warn-encr-chaincert-near-expire-int", 14 );
  return kMax( 1, num );
}

static inline int signingChainCertNearExpiryWarningThresholdInDays() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readBoolEntry( "crypto-warn-when-near-expire", true ) )
    return -1;
  const int num = composer.readNumEntry( "crypto-warn-sign-chaincert-near-expire-int", 14 );
  return kMax( 1, num );
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
 4. Body encrypted, attachments encrypted  (they must be encrypted together, mEarlyAddAttachments)
 5. Body encrypted, attachments not encrypted
 6. Body encrypted, attachment encrypted and signed (separately)
 7. Body not encrypted, one attachment encrypted+signed, one attachment encrypted only, one attachment signed only
       (https://intevation.de/roundup/aegypten/issue295)
       (this is the reason attachments can't be encrypted together)
 8. Body and attachments encrypted+signed (they must be encrypted+signed together, mEarlyAddAttachments)
 9. Body encrypted+signed, attachments encrypted
 10. Body encrypted+signed, one attachment signed, one attachment not encrypted nor signed
 ...

 I recorded a KDExecutor script sending all of the above (David)

 Further tests (which test opportunistic encryption):
 1. Send a message to a person with valid key but without encryption preference
    and answer the question whether the message should be encrypted with Yes.
 2. Send a message to a person with valid key but without encryption preference
    and answer the question whether the message should be encrypted with No.
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
  MessageComposerJob( MessageComposer* composer ) : mComposer( composer ) {}
  virtual ~MessageComposerJob() {}

  virtual void execute() = 0;

protected:
  // These are the methods that call the private MessageComposer methods
  // Workaround for friend not being inherited
  void adjustCryptFlags() { mComposer->adjustCryptFlags(); }
  void composeMessage() { mComposer->composeMessage(); }
  void continueComposeMessage( KMMessage& msg, bool doSign, bool doEncrypt,
                               Kleo::CryptoMessageFormat format )
  {
    mComposer->continueComposeMessage( msg, doSign, doEncrypt, format );
  }
  void chiasmusEncryptAllAttachments() {
    mComposer->chiasmusEncryptAllAttachments();
  }

  MessageComposer* mComposer;
};

class ChiasmusBodyPartEncryptJob : public MessageComposerJob {
public:
  ChiasmusBodyPartEncryptJob( MessageComposer * composer )
    : MessageComposerJob( composer ) {}

  void execute() {
    chiasmusEncryptAllAttachments();
  }
};

class AdjustCryptFlagsJob : public MessageComposerJob {
public:
  AdjustCryptFlagsJob( MessageComposer* composer )
    : MessageComposerJob( composer ) {}

  void execute() {
    adjustCryptFlags();
  }
};

class ComposeMessageJob : public MessageComposerJob {
public:
  ComposeMessageJob( MessageComposer* composer )
    : MessageComposerJob( composer ) {}

  void execute() {
    composeMessage();
  }
};

MessageComposer::MessageComposer( KMComposeWin* win, const char* name )
  : QObject( win, name ), mComposeWin( win ), mCurrentJob( 0 ),
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
  delete mKeyResolver; mKeyResolver = 0;
  delete mNewBodyPart; mNewBodyPart = 0;
}

void MessageComposer::applyChanges( bool disableCrypto )
{
  // Do the initial setup
  if( getenv("KMAIL_DEBUG_COMPOSER_CRYPTO") != 0 ) {
    QCString cE = getenv("KMAIL_DEBUG_COMPOSER_CRYPTO");
    mDebugComposerCrypto = cE == "1" || cE.upper() == "ON" || cE.upper() == "TRUE";
    kdDebug(5006) << "KMAIL_DEBUG_COMPOSER_CRYPTO = TRUE" << endl;
  } else {
    mDebugComposerCrypto = false;
    kdDebug(5006) << "KMAIL_DEBUG_COMPOSER_CRYPTO = FALSE" << endl;
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

  if( mJobs.isEmpty() ) {
    // No more jobs. Signal that we're done
    emitDone( mRc );
    return;
  }

  if( !mRc ) {
    // Something has gone wrong - stop the process and bail out
    while( !mJobs.isEmpty() ) {
      delete mJobs.front();
      mJobs.pop_front();
    }
    emitDone( false );
    return;
  }

  // We have more jobs to do, but allow others to come first
  QTimer::singleShot( 0, this, SLOT( slotDoNextJob() ) );
}

void MessageComposer::emitDone( bool b )
{
  // Save memory - before sending the mail
  mEncodedBody = QByteArray();
  delete mNewBodyPart; mNewBodyPart = 0;
  mOldBodyPart.clear();
  emit done( b );
}

void MessageComposer::slotDoNextJob()
{
  assert( !mCurrentJob );
  if( mHoldJobs )
    // Always make it run from now. If more than one job should be held,
    // The individual jobs must do this.
    mHoldJobs = false;
  else {
    assert( !mJobs.empty() );
    // Get the next job
    mCurrentJob = mJobs.front();
    assert( mCurrentJob );
    mJobs.pop_front();

    // Execute it
    mCurrentJob->execute();
  }

  // Finally run the next job if necessary
  if( !mHoldJobs )
    doNextJob();
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
  if ( mReferenceMessage->type() == DwMime::kTypeMultipart )
    mReferenceMessage->setHeaderField( "Content-Type", "text/plain" );
  mUseOpportunisticEncryption = GlobalSettings::self()->pgpAutoEncrypt();
  mAllowedCryptoMessageFormats = mComposeWin->cryptoMessageFormat();

  if( mAutoCharset ) {
    QCString charset = KMMsgBase::autoDetectCharset( mCharset, KMMessage::preferredCharsets(), mComposeWin->mEditor->text() );
    if( charset.isEmpty() )
    {
      KMessageBox::sorry( mComposeWin,
                          i18n( "No suitable encoding could be found for "
                                "your message.\nPlease set an encoding "
                                "using the 'Options' menu." ) );
      mRc = false;
      return;
    }
    mCharset = charset;
    // Also apply this to the composer window
    mComposeWin->mCharset = charset;
  }
  mReferenceMessage->setCharset(mCharset);

  mReferenceMessage->setTo(mComposeWin->to());
  mReferenceMessage->setFrom(mComposeWin->from());
  mReferenceMessage->setCc(mComposeWin->cc());
  mReferenceMessage->setSubject(mComposeWin->subject());
  mReferenceMessage->setReplyTo(mComposeWin->replyTo());
  mReferenceMessage->setBcc(mComposeWin->bcc());

  const KPIM::Identity & id = mComposeWin->identity();

  KMFolder *f = mComposeWin->mFcc->getFolder();
  assert( f != 0 );
  if ( f->idString() == id.fcc() )
    mReferenceMessage->removeHeaderField("X-KMail-Fcc");
  else
    mReferenceMessage->setFcc( f->idString() );

  // set the correct drafts folder
  mReferenceMessage->setDrafts( id.drafts() );

  if (id.isDefault())
    mReferenceMessage->removeHeaderField("X-KMail-Identity");
  else mReferenceMessage->setHeaderField("X-KMail-Identity", QString::number( id.uoid() ));

  QString replyAddr;
  if (!mComposeWin->replyTo().isEmpty()) replyAddr = mComposeWin->replyTo();
  else replyAddr = mComposeWin->from();

  if (mComposeWin->mRequestMDNAction->isChecked())
    mReferenceMessage->setHeaderField("Disposition-Notification-To", replyAddr);
  else
    mReferenceMessage->removeHeaderField("Disposition-Notification-To");

  if (mComposeWin->mUrgentAction->isChecked()) {
    mReferenceMessage->setHeaderField("X-PRIORITY", "2 (High)");
    mReferenceMessage->setHeaderField("Priority", "urgent");
  } else {
    mReferenceMessage->removeHeaderField("X-PRIORITY");
    mReferenceMessage->removeHeaderField("Priority");
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
  mTo = KPIM::splitEmailAddrList( mComposeWin->to().stripWhiteSpace() );
  mCc = KPIM::splitEmailAddrList( mComposeWin->cc().stripWhiteSpace() );
  mBccList = KPIM::splitEmailAddrList( mBcc.stripWhiteSpace() );

  for ( unsigned int i = 0 ; i < mComposeWin->mAtmList.count() ; ++i )
    mAttachments.push_back( Attachment( mComposeWin->mAtmList.at(i),
					mComposeWin->signFlagOfAttachment( i ),
					mComposeWin->encryptFlagOfAttachment( i ) ) );

  mEncryptWithChiasmus = mComposeWin->mEncryptWithChiasmus;

  mIsRichText = mComposeWin->mEditor->textFormat() == Qt::RichText;
  mIdentityUid = mComposeWin->identityUid();
  mText = breakLinesAndApplyCodec();
  assert( mText.isEmpty() || mText[mText.size()-1] == '\n' );
  // Hopefully we can get rid of this eventually, it's needed to be able
  // to break the plain/text version of a multipart/alternative (html) mail
  // according to the line breaks of the richtext version.
  mLineBreakColumn = mComposeWin->mEditor->lineBreakColumn();
}

static QCString escape_quoted_string( const QCString & str ) {
  QCString result;
  const unsigned int str_len = str.length();
  result.resize( 2*str_len + 1 );
  char * d = result.data();
  for ( unsigned int i = 0 ; i < str_len ; ++i )
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

bool MessageComposer::encryptWithChiasmus( const Kleo::CryptoBackend::Protocol * chiasmus,
                                           const QByteArray& body,
                                           QByteArray& resultData )
{
  std::auto_ptr<Kleo::SpecialJob> job( chiasmus->specialJob( "x-encrypt", QMap<QString,QVariant>() ) );
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
  const Kleo::CryptoBackend::Protocol * chiasmus
    = Kleo::CryptoBackendFactory::instance()->protocol( "Chiasmus" );
  assert( chiasmus ); // kmcomposewin code should have made sure


  for ( QValueVector<Attachment>::iterator it = mAttachments.begin(), end = mAttachments.end() ; it != end ; ++it ) {
    KMMessagePart * part = it->part;
    const QString filename = part->fileName();
    if ( filename.endsWith( ".xia", false ) )
      continue; // already encrypted
    const QByteArray body = part->bodyDecodedBinary();
    QByteArray resultData;
    if ( !encryptWithChiasmus( chiasmus, body, resultData ) ) {
      mRc = false;
      return;
    }
    // everything ok, so let's fill in the part again:
    QValueList<int> dummy;
    part->setBodyAndGuessCte( resultData, dummy );
    part->setTypeStr( "application" );
    part->setSubtypeStr( "vnd.de.bund.bsi.chiasmus" );
    part->setName( filename + ".xia" );
    // this is taken from kmmsgpartdlg.cpp:
    QCString encoding = KMMsgBase::autoDetectCharset( part->charset(), KMMessage::preferredCharsets(), filename );
    if ( encoding.isEmpty() )
      encoding = "utf-8";
    const QCString enc_name = KMMsgBase::encodeRFC2231String( filename + ".xia", encoding );
    const QCString cDisp = "attachment;\n\tfilename"
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
                                             i18n("Use Inline OpenPGP"),
                                             i18n("Use OpenPGP/MIME") );
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
        for ( unsigned int idx = 0 ; idx < mAttachments.size() ; ++idx )
          mAttachments[idx].sign = true;
      }
      if ( mEncryptionRequested ) {
        // The composer window disabled encrypting on the attachments, re-enable it
        // We assume this is what the user wants - after all he chose OpenPGP/MIME for this.
        for ( unsigned int idx = 0 ; idx < mAttachments.size() ; ++idx )
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
    const KPIM::Identity & id =
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
  for ( unsigned int idx = 0 ; idx < mAttachments.size() ; ++idx ) {
    if ( mAttachments[idx].encrypt )
      mEncryptionRequested = true;
    else
      doEncryptCompletely = false;
    if ( mAttachments[idx].sign )
      mSigningRequested = true;
    else
      doSignCompletely = false;
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

bool MessageComposer::determineWhetherToSign( bool doSignCompletely ) {
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
						 i18n("to sign","&Sign"),
						 i18n("Do &Not Sign") ) ) {
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
						i18n("to sign","&Sign"),
						i18n("Do &Not Sign") ) ) {
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
                                               i18n("Send &Unsigned") )
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
      const QString msg = sign && !doSignCompletely
	? i18n("Some parts of this message will not be signed.\n"
	       "Sending only partially signed messages might violate site policy.\n"
	       "Sign all parts instead?") // oh, I hate this...
	: i18n("This message will not be signed.\n"
	       "Sending unsigned message might violate site policy.\n"
	       "Sign message instead?") ; // oh, I hate this...
      const QString buttonText = sign && !doSignCompletely
	? i18n("&Sign All Parts") : i18n("&Sign") ;
      switch ( KMessageBox::warningYesNoCancel( mComposeWin, msg,
						i18n("Unsigned-Message Warning"),
						buttonText,
						i18n("Send &As Is") ) ) {
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

  return sign || doSignCompletely ;
}

bool MessageComposer::determineWhetherToEncrypt( bool doEncryptCompletely ) {
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
						 mDoSign
						 ? i18n("Sign && &Encrypt")
						 : i18n("&Encrypt"),
						 mDoSign
						 ? i18n("&Sign Only")
						 : i18n("&Send As-Is") ) ) {
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
						i18n("&Encrypt"),
						i18n("Do &Not Encrypt") ) ) {
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
                                               i18n("Send &Unencrypted") )
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
      const QString msg = !doEncryptCompletely
	? i18n("Some parts of this message will not be encrypted.\n"
	       "Sending only partially encrypted messages might violate site policy "
	       "and/or leak sensitive information.\n"
	       "Encrypt all parts instead?") // oh, I hate this...
	: i18n("This message will not be encrypted.\n"
	       "Sending unencrypted messages might violate site policy and/or "
	       "leak sensitive information.\n"
	       "Encrypt messages instead?") ; // oh, I hate this...
      const QString buttonText = !doEncryptCompletely
	? i18n("&Encrypt All Parts") : i18n("&Encrypt") ;
      switch ( KMessageBox::warningYesNoCancel( mComposeWin, msg,
						i18n("Unencrypted Message Warning"),
						buttonText,
						mDoSign
						? i18n("&Sign Only")
						: i18n("&Send As-Is") ) ) {
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

  return encrypt || doEncryptCompletely ;
}

void MessageComposer::markAllAttachmentsForSigning( bool sign ) {
  mSignBody = sign;
  for ( QValueVector<Attachment>::iterator it = mAttachments.begin() ; it != mAttachments.end() ; ++it )
    it->sign = sign;
}

void MessageComposer::markAllAttachmentsForEncryption( bool enc ) {
  mEncryptBody = enc;
  for ( QValueVector<Attachment>::iterator it = mAttachments.begin() ; it != mAttachments.end() ; ++it )
    it->encrypt = enc;
}


void MessageComposer::composeMessage()
{
  for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
    if ( mKeyResolver->encryptionItems( concreteCryptoMessageFormats[i] ).empty() )
      continue;
    KMMessage * msg = new KMMessage( *mReferenceMessage );
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

static inline const char * toplevelContentType( Kleo::CryptoMessageFormat f, bool signing ) {
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
      "protocol=\"application/pgp-encrypted\""
      ;
  case Kleo::SMIMEFormat:
    if ( signing )
      return
	"multipart/signed;\n\t"
	"boundary=\"%boundary\";\n\t"
	"protocol=\"application/pkcs7-signature\";\n\t"
	"micalg=sha1"; // FIXME: obtain this parameter from gpgme!
    // fall through (for encryption, there's no difference between
    // SMIME and SMIMEOpaque, since there is no mp/encrypted for
    // S/MIME):
  case Kleo::SMIMEOpaqueFormat:
    return signing ?
      "application/pkcs7-mime;\n\t"
      "smime-type=signed-data;\n\t"
      "name=\"smime.p7m\";\n\t"
      :
      "application/pkcs7-mime;\n\t"
      "smime-type=enveloped-data;\n\t"
      "name=\"smime.p7m\";\n\t"
      ;
  }
}

static inline const char * toplevelContentDisposition( Kleo::CryptoMessageFormat f, bool signing ) {
  switch ( f ) {
  default:
  case Kleo::InlineOpenPGPFormat:
  case Kleo::OpenPGPMIMEFormat:
    return 0;
  case Kleo::SMIMEFormat:
    if ( signing )
      return 0;
  case Kleo::SMIMEOpaqueFormat:
    return "attachment; filename=\"smime.p7m\"";
  }
}

static inline bool includeCleartextWhenSigning( Kleo::CryptoMessageFormat f ) {
  return makeMultiPartSigned( f );
}

static inline const char * nestedContentType( Kleo::CryptoMessageFormat f, bool signing ) {
  switch ( f ) {
  case Kleo::OpenPGPMIMEFormat:
    return signing ? "application/pgp-signature; name=signature.asc \nContent-Description: This is a digitally signed message part." : "application/octet-stream" ;
  case Kleo::SMIMEFormat:
    if ( signing )
      return "application/pkcs7-signature; name=\"smime.p7s\"";
    // fall through:
  default:
  case Kleo::InlineOpenPGPFormat:
  case Kleo::SMIMEOpaqueFormat:
    return 0;
  }
}

static inline const char * nestedContentDisposition( Kleo::CryptoMessageFormat f, bool signing ) {
  if ( !signing && f == Kleo::OpenPGPMIMEFormat )
    return "inline; filename=\"msg.asc\"";
  if ( signing && f == Kleo::SMIMEFormat )
    return "attachment; filename=\"smime.p7s\"";
  return 0;
}

static inline bool binaryHint( Kleo::CryptoMessageFormat f ) {
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

static inline bool armor( Kleo::CryptoMessageFormat f ) {
  return !binaryHint( f );
}

static inline bool textMode( Kleo::CryptoMessageFormat f ) {
  return f == Kleo::InlineOpenPGPFormat;
}

static inline GpgME::Context::SignatureMode signingMode( Kleo::CryptoMessageFormat f ) {
  switch ( f ) {
  case Kleo::SMIMEOpaqueFormat:
    return GpgME::Context::Normal;
  case Kleo::InlineOpenPGPFormat:
    return GpgME::Context::Clearsigned;
  default:
  case Kleo::SMIMEFormat:
  case Kleo::OpenPGPMIMEFormat:
    return GpgME::Context::Detached;
  }
}

//
// END replacements for StructuringInfo(Wrapper)
//

class EncryptMessageJob : public MessageComposerJob {
public:
  EncryptMessageJob( KMMessage* msg, const Kleo::KeyResolver::SplitInfo & si,
                     bool doSign, bool doEncrypt, const QByteArray& encodedBody,
                     int boundaryLevel, /*const KMMessagePart& oldBodyPart,*/
                     KMMessagePart* newBodyPart, Kleo::CryptoMessageFormat format,
		     MessageComposer* composer )
    : MessageComposerJob( composer ), mMsg( msg ), mSplitInfo( si ),
      mDoSign( doSign ), mDoEncrypt( doEncrypt ), mEncodedBody( encodedBody ),
      mBoundaryLevel( boundaryLevel ), /*mOldBodyPart( oldBodyPart ),*/
      mNewBodyPart( newBodyPart ), mFormat( format ) {}

  void execute() {
    KMMessagePart tmpNewBodyPart;
    tmpNewBodyPart.duplicate( *mNewBodyPart ); // slow - we duplicate everything again

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
  KMMessage* mMsg;
  Kleo::KeyResolver::SplitInfo mSplitInfo;
  bool mDoSign, mDoEncrypt;
  QByteArray mEncodedBody;
  int mBoundaryLevel;
  //KMMessagePart mOldBodyPart;
  KMMessagePart* mNewBodyPart;
  Kleo::CryptoMessageFormat mFormat;
};

class SetLastMessageAsUnencryptedVersionOfLastButOne : public MessageComposerJob {
public:
  SetLastMessageAsUnencryptedVersionOfLastButOne( MessageComposer * composer )
    : MessageComposerJob( composer ) {}

  void execute() {
    KMMessage * last = mComposer->mMessageList.back();
    mComposer->mMessageList.pop_back();
    mComposer->mMessageList.back()->setUnencryptedMsg( last );
  }
};

void MessageComposer::composeInlineOpenPGPMessage( KMMessage& theMessage,
                                                   bool doSign, bool doEncrypt )
{
  // preprocess the body text
  const QByteArray bodyData = mText;
  if (bodyData.isNull()) {
    mRc = false;
    return;
  }

  mNewBodyPart = 0; // unused
  mEarlyAddAttachments = false;
  mAllAttachmentsAreInBody = false;

  // set the main headers
  theMessage.deleteBodyParts();
  QString oldContentType = theMessage.headerField( "Content-Type" );
  theMessage.removeHeaderField("Content-Type");
  theMessage.removeHeaderField("Content-Transfer-Encoding");

  const std::vector<Kleo::KeyResolver::SplitInfo> splitInfos
    = mKeyResolver->encryptionItems( Kleo::InlineOpenPGPFormat );
  kdWarning( splitInfos.empty() )
    << "MessageComposer::continueComposeMessage(): splitInfos.empty() for InlineOpenPGPFormat"
    << endl;
  std::vector<Kleo::KeyResolver::SplitInfo>::const_iterator it;
  for ( it = splitInfos.begin() ; it != splitInfos.end() ; ++it ) {
    const Kleo::KeyResolver::SplitInfo& splitInfo = *it;
    KMMessage* msg = new KMMessage( theMessage );
    if ( doEncrypt ) {
      Kpgp::Result result;
      QByteArray encryptedBody;
      if ( doSign ) {  // Sign and encrypt
        const std::vector<GpgME::Key> signingKeys = mKeyResolver->signingKeys( Kleo::InlineOpenPGPFormat );
        result = pgpSignedAndEncryptedMsg( encryptedBody, bodyData, signingKeys,
                                           splitInfo.keys, Kleo::InlineOpenPGPFormat );
      } else { // Encrypt but don't sign
        result = pgpEncryptedMsg( encryptedBody, bodyData,
                                  splitInfo.keys, Kleo::InlineOpenPGPFormat );
      }
      if ( result != Kpgp::Ok ) {
        mRc = false;
        return;
      }
      assert( !encryptedBody.isNull() ); // if you hit this, check gpg-agent is running, then blame gpgme.
      mOldBodyPart.setBodyEncodedBinary( encryptedBody );
    } else {
      if ( doSign ) { // Sign but don't encrypt
        pgpSignedMsg( bodyData, Kleo::InlineOpenPGPFormat );
        if ( mSignature.isNull() ) {
          mRc = false;
          return;
        }
        mOldBodyPart.setBodyEncodedBinary( mSignature );
      } else { // don't sign nor encrypt -> nothing to do
        assert( !bodyData.isNull() );
        mOldBodyPart.setBodyEncodedBinary( bodyData );
      }
    }
    mOldBodyPart.setContentDisposition( "inline" );
    mOldBodyPart.setOriginalContentTypeStr( oldContentType.utf8() );
    mOldBodyPart.setCharset(mCharset);
    addBodyAndAttachments( msg, splitInfo, false, false, mOldBodyPart, Kleo::InlineOpenPGPFormat );
    mMessageList.push_back( msg );
    if ( it == splitInfos.begin() ) {
      if ( doEncrypt && !saveMessagesEncrypted() ) {
        mOldBodyPart.setBodyEncodedBinary( bodyData );
        KMMessage* msgUnenc = new KMMessage( theMessage );
        addBodyAndAttachments( msgUnenc, splitInfo, false, false, mOldBodyPart, Kleo::InlineOpenPGPFormat );
        msg->setUnencryptedMsg( msgUnenc );
      }
    }
  } // end for
}

// very much inspired by composeInlineOpenPGPMessage
void MessageComposer::composeChiasmusMessage( KMMessage& theMessage, Kleo::CryptoMessageFormat format )
{
  assert( !GlobalSettings::chiasmusKey().isEmpty() ); // kmcomposewin code should have made sure
  const Kleo::CryptoBackendFactory * cpf = Kleo::CryptoBackendFactory::instance();
  assert( cpf );
  const Kleo::CryptoBackend::Protocol * chiasmus
    = cpf->protocol( "Chiasmus" );
  assert( chiasmus ); // kmcomposewin code should have made sure

  // preprocess the body text
  const QByteArray bodyData = mText;
  if (bodyData.isNull()) {
    mRc = false;
    return;
  }

  mNewBodyPart = 0; // unused
  mEarlyAddAttachments = false;
  mAllAttachmentsAreInBody = false;

  // set the main headers
  theMessage.deleteBodyParts();
  QString oldContentType = theMessage.headerField( "Content-Type" );
  theMessage.removeHeaderField("Content-Type");
  theMessage.removeHeaderField("Content-Transfer-Encoding");

  // This reads strange, but we know that AdjustCryptFlagsJob created a single splitinfo,
  // under the given "format" (usually openpgp/mime; doesn't matter)
  const std::vector<Kleo::KeyResolver::SplitInfo> splitInfos
    = mKeyResolver->encryptionItems( format );
  assert( splitInfos.size() == 1 );
  for ( std::vector<Kleo::KeyResolver::SplitInfo>::const_iterator it = splitInfos.begin() ; it != splitInfos.end() ; ++it )
  {
    const Kleo::KeyResolver::SplitInfo& splitInfo = *it;
    KMMessage* msg = new KMMessage( theMessage );
    QByteArray encryptedBody;

    if ( !encryptWithChiasmus( chiasmus, bodyData, encryptedBody ) ) {
      mRc = false;
      return;
    }
    assert( !encryptedBody.isNull() );
    // This leaves CTE==7-bit, no good
    //mOldBodyPart.setBodyEncodedBinary( encryptedBody );

    bool doSign = false;
    QValueList<int> allowedCTEs;
    mOldBodyPart.setBodyAndGuessCte( encryptedBody, allowedCTEs,
                                     !kmkernel->msgSender()->sendQuotedPrintable() && !doSign,
				     doSign );


    mOldBodyPart.setContentDisposition( "inline" );
    // Used in case of no attachments
    mOldBodyPart.setOriginalContentTypeStr( "application/vnd.de.bund.bsi.chiasmus-text;chiasmus-charset=" + mCharset );
    // Used in case of attachments
    mOldBodyPart.setTypeStr( "application" );
    mOldBodyPart.setSubtypeStr( "vnd.de.bund.bsi.chiasmus-text" );
    mOldBodyPart.setAdditionalCTypeParamStr( QCString( "chiasmus-charset=" + mCharset ) );
    addBodyAndAttachments( msg, splitInfo, false, false, mOldBodyPart, Kleo::InlineOpenPGPFormat );
    mMessageList.push_back( msg );

    if ( it == splitInfos.begin() && !saveMessagesEncrypted() ) {
      mOldBodyPart.setBodyEncodedBinary( bodyData );
      KMMessage* msgUnenc = new KMMessage( theMessage );
      addBodyAndAttachments( msgUnenc, splitInfo, false, false, mOldBodyPart, Kleo::InlineOpenPGPFormat );
      msg->setUnencryptedMsg( msgUnenc );
    }
  }
}

void MessageComposer::composeMessage( KMMessage& theMessage,
                                      bool doSign, bool doEncrypt,
                                      Kleo::CryptoMessageFormat format )
{
#ifdef DEBUG
  kdDebug(5006) << "entering KMComposeWin::composeMessage" << endl;
#endif
  if ( format == Kleo::InlineOpenPGPFormat ) {
    composeInlineOpenPGPMessage( theMessage, doSign, doEncrypt );
    return;
  }

  if ( mEncryptWithChiasmus )
  {
    composeChiasmusMessage( theMessage, format );
    return;
  }

  // create informative header for those that have no mime-capable
  // email client
  theMessage.setBody( "This message is in MIME format." );

  // preprocess the body text
  QByteArray bodyData = mText;
  if (bodyData.isNull()) {
    mRc = false;
    return;
  }

  // set the main headers
  QString oldContentType = theMessage.headerField( "Content-Type" );
  theMessage.deleteBodyParts();
  theMessage.removeHeaderField("Content-Type");
  theMessage.removeHeaderField("Content-Transfer-Encoding");
  theMessage.setAutomaticFields(true); // == multipart/mixed

  // this is our *final* body part
  mNewBodyPart = new KMMessagePart;

  // this is the boundary depth of the surrounding MIME part
  mPreviousBoundaryLevel = 0;

  // whether the body must be signed/encrypted
  const bool doEncryptBody = doEncrypt && mEncryptBody;
  const bool doSignBody = doSign && mSignBody;

  // create temporary bodyPart for editor text
  // (and for all attachments, if mail is to be signed and/or encrypted)
  mEarlyAddAttachments = !mAttachments.empty() && ( doSignBody || doEncryptBody );

  mAllAttachmentsAreInBody = mEarlyAddAttachments;

  // test whether there ARE attachments that can be included into the body
  if( mEarlyAddAttachments ) {
    bool someOk = false;
    for ( QValueVector<Attachment>::const_iterator it = mAttachments.begin() ; it != mAttachments.end() ; ++it ) {
      if ( it->encrypt == doEncryptBody && it->sign == doSignBody )
        someOk = true;
      else
        mAllAttachmentsAreInBody = false;
    }
    if( !mAllAttachmentsAreInBody && !someOk )
      mEarlyAddAttachments = false;
  }

  kdDebug(5006) << "mEarlyAddAttachments=" << mEarlyAddAttachments << " mAllAttachmentsAreInBody=" << mAllAttachmentsAreInBody << endl;

  // if an html message is to be generated, make a text/plain and text/html part
  mMultipartMixedBoundary = "";
  if ( mEarlyAddAttachments ) {
    mOldBodyPart.setTypeStr( "multipart" );
    mOldBodyPart.setSubtypeStr( "mixed" );
    // calculate a boundary string
    DwMediaType tmpCT;
    tmpCT.CreateBoundary( ++mPreviousBoundaryLevel );
    mMultipartMixedBoundary = tmpCT.Boundary().c_str();
  }
  else if ( mIsRichText ) {
    mOldBodyPart.setTypeStr( "multipart" );
    mOldBodyPart.setSubtypeStr( "alternative" );
  }
  else
    mOldBodyPart.setOriginalContentTypeStr( oldContentType.utf8() );

  mOldBodyPart.setContentDisposition( "inline" );

  if ( mIsRichText ) { // create a multipart body
    // calculate a boundary string
    QCString boundaryCStr;  // storing boundary string data
    QCString newbody;
    DwMediaType tmpCT;
    tmpCT.CreateBoundary( ++mPreviousBoundaryLevel );
    boundaryCStr = KMail::Util::CString( tmpCT.Boundary() );
    QValueList<int> allowedCTEs;

    KMMessagePart textBodyPart;
    textBodyPart.setTypeStr("text");
    textBodyPart.setSubtypeStr("plain");

    QCString textbody = plainTextFromMarkup( mText /* converted to QString */ );

    // the signed body must not be 8bit encoded
    textBodyPart.setBodyAndGuessCte( textbody, allowedCTEs,
                                     !kmkernel->msgSender()->sendQuotedPrintable() && !doSign,
				     doSign );
    textBodyPart.setCharset( mCharset );
    textBodyPart.setBodyEncoded( textbody );
    DwBodyPart* textDwPart = theMessage.createDWBodyPart( &textBodyPart );
    textDwPart->Assemble();
    newbody += "--";
    newbody +=     boundaryCStr;
    newbody +=                 "\n";
    newbody += textDwPart->AsString().c_str();
    delete textDwPart;
    textDwPart = 0;

    KMMessagePart htmlBodyPart;
    htmlBodyPart.setTypeStr("text");
    htmlBodyPart.setSubtypeStr("html");
    QByteArray htmlbody = mText;
    // the signed body must not be 8bit encoded
    htmlBodyPart.setBodyAndGuessCte( htmlbody, allowedCTEs,
                                     !kmkernel->msgSender()->sendQuotedPrintable() && !doSign,
				     doSign );
    htmlBodyPart.setCharset( mCharset );
    htmlBodyPart.setBodyEncodedBinary( htmlbody );
    DwBodyPart* htmlDwPart = theMessage.createDWBodyPart( &htmlBodyPart );
    htmlDwPart->Assemble();
    newbody += "\n--";
    newbody +=     boundaryCStr;
    newbody +=                 "\n";
    newbody += htmlDwPart->AsString().c_str();
    delete htmlDwPart;
    htmlDwPart = 0;

    newbody += "--";
    newbody +=     boundaryCStr;
    newbody +=                 "--\n";
    bodyData = KMail::Util::byteArrayFromQCStringNoDetach( newbody );
    mOldBodyPart.setBodyEncodedBinary( bodyData );

    mSaveBoundary = tmpCT.Boundary();
  }

  // Prepare attachments that will be signed/encrypted
  for ( QValueVector<Attachment>::const_iterator it = mAttachments.begin() ; it != mAttachments.end() ; ++it ) {
    // signed/encrypted body parts must be either QP or base64 encoded
    // Why not 7 bit? Because the LF->CRLF canonicalization would render
    // e.g. 7 bit encoded shell scripts unusable because of the CRs.
    //
    // (marc) this is a workaround for the KMail bug that doesn't
    // respect the CRLF->LF de-canonicalisation. We should
    // eventually get rid of this:
    if( it->sign || it->encrypt ) {
      QCString cte = it->part->cteStr().lower();
      if( ( "8bit" == cte && it->part->type() != DwMime::kTypeMessage )
          || ( ( it->part->type() == DwMime::kTypeText )
               && ( "7bit" == cte ) ) ) {
        const QByteArray body = it->part->bodyDecodedBinary();
        QValueList<int> dummy;
        it->part->setBodyAndGuessCte(body, dummy, false, it->sign);
        kdDebug(5006) << "Changed encoding of message part from "
                      << cte << " to " << it->part->cteStr() << endl;
      }
    }
  }

  if( mEarlyAddAttachments ) {
    // add the normal body text
    KMMessagePart innerBodyPart;
    if ( mIsRichText ) {
      innerBodyPart.setTypeStr(   "multipart");//text" );
      innerBodyPart.setSubtypeStr("alternative");//html");
    }
    else {
      innerBodyPart.setOriginalContentTypeStr( oldContentType.utf8() );
    }
    innerBodyPart.setContentDisposition( "inline" );
    QValueList<int> allowedCTEs;
    // the signed body must not be 8bit encoded
    innerBodyPart.setBodyAndGuessCte( bodyData, allowedCTEs,
                                      !kmkernel->msgSender()->sendQuotedPrintable() && !doSign,
                                      doSign );
    if ( !mIsRichText )
      innerBodyPart.setCharset( mCharset );
    innerBodyPart.setBodyEncodedBinary( bodyData ); // do we need this, since setBodyAndGuessCte does this already?
    DwBodyPart* innerDwPart = theMessage.createDWBodyPart( &innerBodyPart );
    innerDwPart->Assemble();
    QByteArray tmpbody = KMail::Util::ByteArray( innerDwPart->AsString() );
    if ( mIsRichText ) { // and add our mp/a boundary
        int boundPos = tmpbody.find( '\n' );
        if( -1 < boundPos ) {
          QCString bStr( ";\n  boundary=\"" );
          bStr += mSaveBoundary.c_str();
          bStr += "\"";
          bodyData = tmpbody;
          KMail::Util::insert( bodyData, boundPos, bStr );
          KMail::Util::insert( bodyData, 0, "--" + mMultipartMixedBoundary + "\n" ); // prepend
        }
    }
    else {
      bodyData = tmpbody;
      KMail::Util::insert( bodyData, 0, "--" + mMultipartMixedBoundary + "\n" ); // prepend
    }
    delete innerDwPart;
    innerDwPart = 0;
    // add all matching Attachments
    // NOTE: This code will be changed when KMime is complete.
    for ( QValueVector<Attachment>::iterator it = mAttachments.begin() ; it != mAttachments.end() ; ++it ) {
      if ( it->encrypt == doEncryptBody && it->sign == doSignBody ) {
        innerDwPart = theMessage.createDWBodyPart( it->part );
        innerDwPart->Assemble();
        KMail::Util::append( bodyData, QCString( "\n--" + mMultipartMixedBoundary + "\n" ) );
        KMail::Util::append( bodyData, innerDwPart->AsString().c_str() );
        delete innerDwPart;
        innerDwPart = 0;
      }
    }
    KMail::Util::append( bodyData, QCString( "\n--" + mMultipartMixedBoundary + "--\n" ) );
  } else { // !earlyAddAttachments
    QValueList<int> allowedCTEs;
    // the signed body must not be 8bit encoded
    mOldBodyPart.setBodyAndGuessCte(bodyData, allowedCTEs, !kmkernel->msgSender()->sendQuotedPrintable() && !doSign,
                                    doSign);
    if ( !mIsRichText )
      mOldBodyPart.setCharset(mCharset);
  }
  // create S/MIME body part for signing and/or encrypting
  mOldBodyPart.setBodyEncodedBinary( bodyData );

  if( doSignBody || doEncryptBody ) {
    // get string representation of body part (including the attachments)

    DwBodyPart* dwPart;
    if ( mIsRichText && !mEarlyAddAttachments ) {
      // if we are using richtext and not already have a mp/a body
      // make the body a mp/a body
      dwPart = theMessage.createDWBodyPart( &mOldBodyPart );
      DwHeaders& headers = dwPart->Headers();
      DwMediaType& ct = headers.ContentType();
      ct.SetBoundary(mSaveBoundary);
      dwPart->Assemble();
    }
    else {
      dwPart = theMessage.createDWBodyPart( &mOldBodyPart );
      dwPart->Assemble();
    }
    mEncodedBody = KMail::Util::ByteArray( dwPart->AsString() );
    delete dwPart;
    dwPart = 0;

    // manually add a boundary definition to the Content-Type header
    if( !mMultipartMixedBoundary.isEmpty() ) {
      int boundPos = mEncodedBody.find( '\n' );
      if( -1 < boundPos ) {
        // insert new "boundary" parameter
        QCString bStr( ";\n  boundary=\"" );
        bStr += mMultipartMixedBoundary;
        bStr += "\"";
        KMail::Util::insert( mEncodedBody, boundPos, bStr.data() );
      }
    }

    // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
    // according to RfC 2633, 3.1.1 Canonicalization
    //kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
    mEncodedBody = KMail::Util::lf2crlf( mEncodedBody );
  }

  if ( doSignBody ) {
    mPerformingSignOperation = true;  // this lets the KMComposeWin know if it is safe to close the window.
    pgpSignedMsg( mEncodedBody, format );
    mPerformingSignOperation = false;

    if ( mSignature.isEmpty() ) {
      kdDebug() << "signature was empty" << endl;
      mRc = false;
      return;
    }
    mRc = processStructuringInfo( QString::null,
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
    } else
      KMessageBox::sorry( mComposeWin,
			  mErrorProcessingStructuringInfo );
  }

  if ( !mRc )
    return;

  continueComposeMessage( theMessage, doSign, doEncrypt, format );
}

// Do the encryption stuff
void MessageComposer::continueComposeMessage( KMMessage& theMessage,
                                              bool doSign, bool doEncrypt,
                                              Kleo::CryptoMessageFormat format )
{

  const std::vector<Kleo::KeyResolver::SplitInfo> splitInfos
    = mKeyResolver->encryptionItems( format );
  kdWarning( splitInfos.empty() )
    << "MessageComposer::continueComposeMessage(): splitInfos.empty() for "
    << Kleo::cryptoMessageFormatToString( format ) << endl;

  if ( !splitInfos.empty() && doEncrypt && !saveMessagesEncrypted() ) {
    mJobs.push_front( new SetLastMessageAsUnencryptedVersionOfLastButOne( this ) );
    mJobs.push_front( new EncryptMessageJob( new KMMessage( theMessage ),
					     Kleo::KeyResolver::SplitInfo( splitInfos.front().recipients ), doSign,
					     false, mEncodedBody,
					     mPreviousBoundaryLevel,
					     /*mOldBodyPart,*/ mNewBodyPart,
					     format, this ) );
  }

  for ( std::vector<Kleo::KeyResolver::SplitInfo>::const_iterator it = splitInfos.begin() ; it != splitInfos.end() ; ++it )
    mJobs.push_front( new EncryptMessageJob( new KMMessage( theMessage ), *it, doSign,
					     doEncrypt, mEncodedBody,
					     mPreviousBoundaryLevel,
					     /*mOldBodyPart,*/ mNewBodyPart,
					     format, this ) );
}

void MessageComposer::encryptMessage( KMMessage* msg,
				      const Kleo::KeyResolver::SplitInfo & splitInfo,
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
      DwBodyPart* dwPart = msg->createDWBodyPart( &newBodyPart );
      dwPart->Assemble();
      innerContent = KMail::Util::ByteArray( dwPart->AsString() );
      delete dwPart;
      dwPart = 0;
    } else {
      innerContent = mEncodedBody;
    }

    // now do the encrypting:
    // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
    // according to RfC 2633, 3.1.1 Canonicalization
    //kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
    innerContent = KMail::Util::lf2crlf( innerContent );
    //kdDebug(5006) << "                                                       done." << endl;

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
    if ( !mRc )
      KMessageBox::sorry(mComposeWin, mErrorProcessingStructuringInfo);
  }

  // process the attachments that are not included into the body
  if( mRc ) {
    const bool useNewBodyPart = doSignBody || doEncryptBody;
    addBodyAndAttachments( msg, splitInfo, doSign, doEncrypt,
      useNewBodyPart ? newBodyPart : mOldBodyPart, format );
  }
}

void MessageComposer::addBodyAndAttachments( KMMessage* msg,
                                             const Kleo::KeyResolver::SplitInfo & splitInfo,
                                             bool doSign, bool doEncrypt,
                                             const KMMessagePart& ourFineBodyPart,
                                             Kleo::CryptoMessageFormat format )
{
  const bool doEncryptBody = doEncrypt && mEncryptBody;
  const bool doSignBody = doSign && mSignBody;

  if( !mAttachments.empty()
      && ( !mEarlyAddAttachments || !mAllAttachmentsAreInBody ) ) {
    // set the content type header
    msg->headers().ContentType().SetType( DwMime::kTypeMultipart );
    msg->headers().ContentType().SetSubtype( DwMime::kSubtypeMixed );
    msg->headers().ContentType().CreateBoundary( 0 );
    kdDebug(5006) << "MessageComposer::addBodyAndAttachments() : set top level Content-Type to Multipart/Mixed" << endl;

    // add our Body Part
    DwBodyPart* tmpDwPart = msg->createDWBodyPart( &ourFineBodyPart );
    DwHeaders& headers = tmpDwPart->Headers();
    DwMediaType& ct = headers.ContentType();
    if ( !mSaveBoundary.empty() )
      ct.SetBoundary(mSaveBoundary);
    tmpDwPart->Assemble();

    //KMMessagePart newPart;
    //newPart.setBody(tmpDwPart->AsString().c_str());
    msg->addDwBodyPart(tmpDwPart); // only this method doesn't add it as text/plain

    // add Attachments
    // create additional bodyparts for the attachments (if any)
    KMMessagePart newAttachPart;
    for ( QValueVector<Attachment>::iterator it = mAttachments.begin() ; it != mAttachments.end() ; ++it ) {

      const bool cryptFlagsDifferent = ( it->encrypt != doEncryptBody || it->sign != doSignBody ) ;

      if ( !cryptFlagsDifferent && mEarlyAddAttachments )
        continue;

      const bool encryptThisNow = doEncrypt && cryptFlagsDifferent && it->encrypt ;
      const bool signThisNow = doSign && cryptFlagsDifferent && it->sign ;

      if ( !encryptThisNow && !signThisNow ) {
        msg->addBodyPart( it->part );
        // Assemble the message. Not sure why, but this fixes the vanishing boundary parameter
        (void)msg->asDwMessage();
        continue;
      }

      KMMessagePart& rEncryptMessagePart( *it->part );

      DwBodyPart* innerDwPart = msg->createDWBodyPart( it->part );
      innerDwPart->Assemble();
      QByteArray encodedAttachment = KMail::Util::ByteArray( innerDwPart->AsString() );
      delete innerDwPart;
      innerDwPart = 0;

      // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
      // according to RfC 2633, 3.1.1 Canonicalization
      //kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
      encodedAttachment = KMail::Util::lf2crlf( encodedAttachment );

      // sign this attachment
      if( signThisNow ) {
        pgpSignedMsg( encodedAttachment, format );
        mRc = !mSignature.isEmpty();
        if( mRc ) {
          mRc = processStructuringInfo( "http://www.gnupg.org/aegypten/",
                                        it->part->contentDescription(),
                                        it->part->typeStr(),
                                        it->part->subtypeStr(),
                                        it->part->contentDisposition(),
                                        it->part->contentTransferEncodingStr(),
                                        encodedAttachment,
                                        "signature",
                                        mSignature,
                                        newAttachPart, true, format );
          if( mRc ) {
            if( encryptThisNow ) {
              rEncryptMessagePart = newAttachPart;
              DwBodyPart* dwPart = msg->createDWBodyPart( &newAttachPart );
              dwPart->Assemble();
              encodedAttachment = KMail::Util::ByteArray( dwPart->AsString() );
              delete dwPart;
              dwPart = 0;
            }
          } else
            KMessageBox::sorry( mComposeWin, mErrorProcessingStructuringInfo );
        } else {
          // quit the attachments' loop
          break;
        }
      }
      if( encryptThisNow ) {
        QByteArray encryptedBody;
        Kpgp::Result result = pgpEncryptedMsg( encryptedBody,
                                               encodedAttachment,
                                               splitInfo.keys,
                                               format );

        if( Kpgp::Ok == result ) {
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
          if ( !mRc )
            KMessageBox::sorry( mComposeWin, mErrorProcessingStructuringInfo );
        } else
          mRc = false;
      }
      msg->addBodyPart( &newAttachPart );
      (void)msg->asDwMessage(); // Assemble the message. One gets a completely empty message otherwise :/
    }
  } else { // no attachments in the final message
    if( ourFineBodyPart.originalContentTypeStr() ) {
      msg->headers().ContentType().FromString( ourFineBodyPart.originalContentTypeStr() );
      msg->headers().ContentType().Parse();
      kdDebug(5006) << "MessageComposer::addBodyAndAttachments() : set top level Content-Type from originalContentTypeStr()=" << ourFineBodyPart.originalContentTypeStr() << endl;
    } else {
      QCString ct = ourFineBodyPart.typeStr() + "/" + ourFineBodyPart.subtypeStr();
      if ( ct == "multipart/mixed" )
        ct += ";\n\tboundary=\"" + mMultipartMixedBoundary + '"';
      else if ( ct == "multipart/alternative" )
        ct += ";\n\tboundary=\"" + QCString(mSaveBoundary.c_str()) + '"';
      msg->headers().ContentType().FromString( ct );
      msg->headers().ContentType().Parse();
      kdDebug(5006) << "MessageComposer::addBodyAndAttachments() : set top level Content-Type to " << ct << endl;
    }
    if ( !ourFineBodyPart.charset().isEmpty() )
      msg->setCharset( ourFineBodyPart.charset() );
    msg->setHeaderField( "Content-Transfer-Encoding",
                         ourFineBodyPart.contentTransferEncodingStr() );
    msg->setHeaderField( "Content-Description",
                         ourFineBodyPart.contentDescription() );
    msg->setHeaderField( "Content-Disposition",
                         ourFineBodyPart.contentDisposition() );

    if ( mDebugComposerCrypto )
      kdDebug(5006) << "MessageComposer::addBodyAndAttachments() : top level headers and body adjusted" << endl;

    // set body content
    msg->setBody( ourFineBodyPart.dwBody() );

  }

  msg->setHeaderField( "X-KMail-Recipients",
                       splitInfo.recipients.join(", "), KMMessage::Address );

  if ( mDebugComposerCrypto ) {
    kdDebug(5006) << "MessageComposer::addBodyAndAttachments():\n      Final message:\n|||" << msg->asString() << "|||\n\n" << endl;
    msg->headers().Assemble();
    kdDebug(5006) << "\n\n\nMessageComposer::addBodyAndAttachments():\n      Final headers:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
  }
}

//-----------------------------------------------------------------------------
// This method does not call any crypto ops, so it does not need to be async
bool MessageComposer::processStructuringInfo( const QString bugURL,
                                              const QString contentDescClear,
                                              const QCString contentTypeClear,
                                              const QCString contentSubtypeClear,
                                              const QCString contentDispClear,
                                              const QCString contentTEncClear,
                                              const QByteArray& clearCStr,
                                              const QString /*contentDescCiph*/,
                                              const QByteArray& ciphertext,
                                              KMMessagePart& resultingPart,
					      bool signing, Kleo::CryptoMessageFormat format )
{
  assert( clearCStr.isEmpty() || clearCStr[clearCStr.size()-1] != '\0' ); // I was called with a QCString !?
  bool bOk = true;

  if ( makeMimeObject( format, signing ) ) {
    QCString mainHeader = "Content-Type: ";
    const char * toplevelCT = toplevelContentType( format, signing );
    if ( toplevelCT )
      mainHeader += toplevelCT;
    else {
      if( makeMultiMime( format, signing ) )
        mainHeader += "text/plain";
      else
        mainHeader += contentTypeClear + '/' + contentSubtypeClear;
    }

    const QCString boundaryCStr = KMime::multiPartBoundary();
    // add "boundary" parameter
    if ( makeMultiMime( format, signing ) )
      mainHeader.replace( "%boundary", boundaryCStr );

    if ( toplevelCT ) {
      if ( const char * str = toplevelContentDisposition( format, signing ) ) {
        mainHeader += "\nContent-Disposition: ";
        mainHeader += str;
      }
      if ( !makeMultiMime( format, signing ) &&
	   binaryHint( format ) )
        mainHeader += "\nContent-Transfer-Encoding: base64";
    } else {
      if( 0 < contentDispClear.length() ) {
        mainHeader += "\nContent-Disposition: ";
        mainHeader += contentDispClear;
      }
      if( 0 < contentTEncClear.length() ) {
        mainHeader += "\nContent-Transfer-Encoding: ";
        mainHeader += contentTEncClear;
      }
    }

    //kdDebug(5006) << "processStructuringInfo: mainHeader=" << mainHeader << endl;

    DwString mainDwStr;
    mainDwStr = mainHeader + "\n\n";
    DwBodyPart mainDwPa( mainDwStr, 0 );
    mainDwPa.Parse();
    KMMessage::bodyPart( &mainDwPa, &resultingPart );
    if( !makeMultiMime( format, signing ) ) {
      if ( signing && includeCleartextWhenSigning( format ) ) {
        QByteArray bodyText( clearCStr );
        KMail::Util::append( bodyText, "\n" );
        KMail::Util::append( bodyText, ciphertext );
        resultingPart.setBodyEncodedBinary( bodyText );
      } else {
        resultingPart.setBodyEncodedBinary( ciphertext );
      }
    } else {
      // Build the encapsulated MIME parts.
      // Build a MIME part holding the version information
      // taking the body contents returned in
      // structuring.data.bodyTextVersion.
      QCString versCStr, codeCStr;
      if ( !signing && format == Kleo::OpenPGPMIMEFormat )
        versCStr =
	  "Content-Type: application/pgp-encrypted\n"
	  "Content-Disposition: attachment\n"
	  "\n"
	  "Version: 1";

      // Build a MIME part holding the code information
      // taking the body contents returned in ciphertext.
      const char * nestedCT = nestedContentType( format, signing );
      assert( nestedCT );
      codeCStr = "Content-Type: ";
      codeCStr += nestedCT;
      codeCStr += '\n';
      if ( const char * str = nestedContentDisposition( format, signing ) ) {
	codeCStr += "Content-Disposition: ";
	codeCStr += str;
	codeCStr += '\n';
      }
      if ( binaryHint( format ) ) {
	codeCStr += "Content-Transfer-Encoding: base64\n\n";
	codeCStr += KMime::Codec::codecForName( "base64" )->encodeToQCString( ciphertext );
      } else
	codeCStr += '\n' + QCString( ciphertext.data(), ciphertext.size() + 1 );


      QByteArray mainStr;
      KMail::Util::append( mainStr, "--" );
      KMail::Util::append( mainStr, boundaryCStr );
      if ( signing && includeCleartextWhenSigning( format ) &&
	   !clearCStr.isEmpty() ) {
        KMail::Util::append( mainStr, "\n" );
        // clearCStr is the one that can be very big for large attachments, don't merge with the other lines
        KMail::Util::append( mainStr, clearCStr );
        KMail::Util::append( mainStr, "\n--" + boundaryCStr );
      }
      if ( !versCStr.isEmpty() )
        KMail::Util::append( mainStr, "\n" + versCStr + "\n--" + boundaryCStr );
      if( !codeCStr.isEmpty() )
        KMail::Util::append( mainStr, "\n" + codeCStr + "\n--" + boundaryCStr );
      KMail::Util::append( mainStr, "--\n" );

      //kdDebug(5006) << "processStructuringInfo: mainStr=" << mainStr << endl;
      resultingPart.setBodyEncodedBinary( mainStr );
    }

  } else { //  not making a mime object, build a plain message body.

    resultingPart.setContentDescription( contentDescClear );
    resultingPart.setTypeStr( contentTypeClear );
    resultingPart.setSubtypeStr( contentSubtypeClear );
    resultingPart.setContentDisposition( contentDispClear );
    resultingPart.setContentTransferEncodingStr( contentTEncClear );
    QByteArray resultingBody;

    if ( signing && includeCleartextWhenSigning( format ) ) {
      if( !clearCStr.isEmpty() )
        KMail::Util::append( resultingBody, clearCStr );
    }
    if ( !ciphertext.isEmpty() )
      KMail::Util::append( resultingBody, ciphertext );
    else {
      // Plugin error!
      KMessageBox::sorry( mComposeWin,
                          i18n( "<qt><p>Error: The backend did not return "
                                "any encoded data.</p>"
                                "<p>Please report this bug:<br>%2</p></qt>" )
                          .arg( bugURL ) );
      bOk = false;
    }
    resultingPart.setBodyEncodedBinary( resultingBody );
  }

  return bOk;
}

//-----------------------------------------------------------------------------
QCString MessageComposer::plainTextFromMarkup( const QString& markupText )
{
  QTextEdit *hackConspiratorTextEdit = new QTextEdit( markupText );
  hackConspiratorTextEdit->setTextFormat(Qt::PlainText);
  if ( !mDisableBreaking ) {
    hackConspiratorTextEdit->setWordWrap( QTextEdit::FixedColumnWidth );
    hackConspiratorTextEdit->setWrapColumnOrWidth( mLineBreakColumn );
  }
  QString text = hackConspiratorTextEdit->text();
  QCString textbody;

  const QTextCodec *codec = KMMsgBase::codecForName( mCharset );
  if( mCharset == "us-ascii" ) {
    textbody = KMMsgBase::toUsAscii( text );
  } else if( codec == 0 ) {
    kdDebug(5006) << "Something is wrong and I can not get a codec." << endl;
    textbody = text.local8Bit();
  } else {
    text = codec->toUnicode( text.latin1(), text.length() );
    textbody = codec->fromUnicode( text );
  }
  if (textbody.isNull()) textbody = "";

  delete hackConspiratorTextEdit;
  return textbody;
}

//-----------------------------------------------------------------------------
QByteArray MessageComposer::breakLinesAndApplyCodec()
{
  QString text;
  QCString cText;

  if( mDisableBreaking || mIsRichText || !GlobalSettings::self()->wordWrap() )
    text = mComposeWin->mEditor->text();
  else
    text = mComposeWin->mEditor->brokenText();
  text.truncate( text.length() ); // to ensure text.size()==text.length()+1

  QString newText;
  const QTextCodec *codec = KMMsgBase::codecForName( mCharset );

  if( mCharset == "us-ascii" ) {
    cText = KMMsgBase::toUsAscii( text );
    newText = QString::fromLatin1( cText );
  } else if( codec == 0 ) {
    kdDebug(5006) << "Something is wrong and I can not get a codec." << endl;
    cText = text.local8Bit();
    newText = QString::fromLocal8Bit( cText );
  } else {
    cText = codec->fromUnicode( text );
    newText = codec->toUnicode( cText );
  }
  if (cText.isNull()) cText = "";

  if( !text.isEmpty() && (newText != text) ) {
    QString oldText = mComposeWin->mEditor->text();
    mComposeWin->mEditor->setText( newText );
    KCursorSaver idle( KBusyPtr::idle() );
    bool anyway = ( KMessageBox::warningYesNo( mComposeWin,
                                               i18n("<qt>Not all characters fit into the chosen"
                                                    " encoding.<br><br>Send the message anyway?</qt>"),
                                               i18n("Some Characters Will Be Lost"),
                                               i18n("Lose Characters"), i18n("Change Encoding") ) == KMessageBox::Yes );
    if( !anyway ) {
      mComposeWin->mEditor->setText(oldText);
      return QByteArray();
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
  if( cText.isEmpty() || cText[cText.length()-1] != '\n' ) {
    kdDebug(5006) << "Added an <LF> on the last line" << endl;
    cText += "\n";
  }
  return KMail::Util::byteArrayFromQCStringNoDetach( cText );
}


//-----------------------------------------------------------------------------
void MessageComposer::pgpSignedMsg( const QByteArray& cText, Kleo::CryptoMessageFormat format ) {

  assert( cText.isEmpty() || cText[cText.size()-1] != '\0' ); // I was called with a QCString !?
  mSignature = QByteArray();

  const std::vector<GpgME::Key> signingKeys = mKeyResolver->signingKeys( format );

  assert( !signingKeys.empty() );

  // TODO: ASync call? Likely, yes :-)
  const Kleo::CryptoBackendFactory * cpf = Kleo::CryptoBackendFactory::instance();
  assert( cpf );
  const Kleo::CryptoBackend::Protocol * proto
    = isSMIME( format ) ? cpf->smime() : cpf->openpgp() ;
  assert( proto ); /// hmmm.....?

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

  QByteArray signature;
  const GpgME::SigningResult res =
    job->exec( signingKeys, cText, signingMode( format ), signature );
  if ( res.error().isCanceled() ) {
    kdDebug() << "signing was canceled by user" << endl;
    return;
  }
  if ( res.error() ) {
    kdDebug() << "signing failed: " << res.error().asString() << endl;
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
Kpgp::Result MessageComposer::pgpEncryptedMsg( QByteArray & encryptedBody,
                                               const QByteArray& cText,
                                               const std::vector<GpgME::Key> & encryptionKeys,
					       Kleo::CryptoMessageFormat format )
{
  // TODO: ASync call? Likely, yes :-)
  const Kleo::CryptoBackendFactory * cpf = Kleo::CryptoBackendFactory::instance();
  assert( cpf );
  const Kleo::CryptoBackend::Protocol * proto
    = isSMIME( format ) ? cpf->smime() : cpf->openpgp() ;
  assert( proto ); // hmmmm....?

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

  const GpgME::EncryptionResult res =
    job->exec( encryptionKeys, cText, true /* we do ownertrust ourselves */, encryptedBody );
  if ( res.error().isCanceled() ) {
    kdDebug() << "encryption was canceled by user" << endl;
    return Kpgp::Canceled;
  }
  if ( res.error() ) {
    kdDebug() << "encryption failed: " << res.error().asString() << endl;
    job->showErrorDialog( mComposeWin );
    return Kpgp::Failure;
  }

  if ( GlobalSettings::showGnuPGAuditLogAfterSuccessfulSignEncrypt() )
      Kleo::MessageBox::auditLog( 0, job.get(), i18n("GnuPG Audit Log for Encryption Operation") );

  return Kpgp::Ok;
}

Kpgp::Result MessageComposer::pgpSignedAndEncryptedMsg( QByteArray & encryptedBody,
							const QByteArray& cText,
							const std::vector<GpgME::Key> & signingKeys,
							const std::vector<GpgME::Key> & encryptionKeys,
							Kleo::CryptoMessageFormat format )
{
  // TODO: ASync call? Likely, yes :-)
  const Kleo::CryptoBackendFactory * cpf = Kleo::CryptoBackendFactory::instance();
  assert( cpf );
  const Kleo::CryptoBackend::Protocol * proto
    = isSMIME( format ) ? cpf->smime() : cpf->openpgp() ;
  assert( proto ); // hmmmm....?

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
    kdDebug() << "encrypt/sign was canceled by user" << endl;
    return Kpgp::Canceled;
  }
  if ( res.first.error() || res.second.error() ) {
    if ( res.first.error() )
      kdDebug() << "signing failed: " << res.first.error().asString() << endl;
    else
      kdDebug() << "encryption failed: " << res.second.error().asString() << endl;
    job->showErrorDialog( mComposeWin );
    return Kpgp::Failure;
  }

  if ( GlobalSettings::showGnuPGAuditLogAfterSuccessfulSignEncrypt() )
      Kleo::MessageBox::auditLog( 0, job.get(), i18n("GnuPG Audit Log for Encryption Operation") );

  return Kpgp::Ok;
}


#include "messagecomposer.moc"
