/**
 *  messagecomposer.cpp
 *
 *  Copyright (c) 2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#include "kmcomposewin.h"
#include "kmmessage.h"
#include "klistboxdialog.h"
#include "kcursorsaver.h"
#include "kmkernel.h"
#include "kmsender.h"
#include "kmidentity.h"
#include "kmfolder.h"
#include "identitymanager.h"
#include "identitycombo.h"
#include "kmfoldercombobox.h"

#include <kpgpblock.h>
#include <mimelib/mimepp.h>
#include <cryptplugwrapper.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kinputdialog.h>
#include <kdebug.h>
#include <kaction.h>
#include <qfile.h>
#include <qtextcodec.h>
#include <qtimer.h>


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

  A synchronous job simply implements the execute() method and performs
  it's operation there. When execute returns, this returns to the event
  queue and returns to executes the next job.

  An asynchronous job only sets up and starts it's operation. Before
  returning, it connects to the result signals of the operation
  (e.g. Kleo::Job's result(...) signal) and sets mComposer->mHoldJobs
  to true. This makes the scheduler return to the event loop. The job
  is now responsible for giving control back to the scheduler by
  calling mComposer->doNextJob().

  If a problem occurs, set mComposer->mRc to false, and the job
  queue be stopped.

  If mRc is set to false in an asynchronous job, you must still call
  doNextJob(). This will do the deletion of the command queue.
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
  void composeMessage2() { mComposer->composeMessage2(); }
  void composeMessage3() { mComposer->composeMessage3(); }
  void continueComposeMessage( KMMessage& msg, bool doSign, bool doEncrypt,
                               bool ignoreBcc )
  {
    mComposer->continueComposeMessage( msg, doSign, doEncrypt, ignoreBcc );
  }

  MessageComposer* mComposer;
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

class ComposeMessage2Job : public MessageComposerJob {
public:
  ComposeMessage2Job( MessageComposer* composer )
    : MessageComposerJob( composer ) {}

  void execute() {
    composeMessage2();
  }
};

class ComposeMessage3Job : public MessageComposerJob {
public:
  ComposeMessage3Job( MessageComposer* composer )
    : MessageComposerJob( composer ) {}

  void execute() {
    composeMessage3();
  }
};

class ContinueComposeMessageJob : public MessageComposerJob {
public:
  ContinueComposeMessageJob( MessageComposer* composer, KMMessage& msg,
                             bool doSign, bool doEncrypt, bool ignoreBcc )
    : MessageComposerJob( composer ), mMsg( msg ), mDoSign( doSign ),
      mDoEncrypt( doEncrypt ), mIgnoreBcc( ignoreBcc ) {}

  void execute() {
    continueComposeMessage( mMsg, mDoSign, mDoEncrypt, mIgnoreBcc );
  }

private:
  KMMessage& mMsg;
  bool mDoSign, mDoEncrypt, mIgnoreBcc;
};


MessageComposer::MessageComposer( KMComposeWin* win, const char* name )
  : QObject( win, name ), mComposeWin( win ), mCurrentJob( 0 )
{
}

MessageComposer::~MessageComposer()
{
}

void MessageComposer::applyChanges( bool dontSign, bool dontEncrypt )
{
  // Do the initial setup
  mDisableBreaking = false;

  if( getenv("KMAIL_DEBUG_COMPOSER_CRYPTO") != 0 ) {
    QCString cE = getenv("KMAIL_DEBUG_COMPOSER_CRYPTO");
    mDebugComposerCrypto = cE == "1" || cE.upper() == "ON" || cE.upper() == "TRUE";
    kdDebug(5006) << "KMAIL_DEBUG_COMPOSER_CRYPTO = TRUE" << endl;
  } else {
    mDebugComposerCrypto = false;
    kdDebug(5006) << "KMAIL_DEBUG_COMPOSER_CRYPTO = FALSE" << endl;
  }

  mDontSign = !mComposeWin->mSignAction->isChecked() ||
    mComposeWin->mNeverSign || dontSign;
  mDontEncrypt = !mComposeWin->mEncryptAction->isChecked() ||
    mComposeWin->mNeverEncrypt || dontEncrypt;

  mHoldJobs = false;
  mRc = true;

  // 1: Read everything from KMComposeWin and set all
  //    trivial parts of the message
  readFromComposeWin();

  // From now on, we're not supposed to read from the composer win
  // TODO: Make it so ;-)

  // 2: Set encryption/signing options
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
    // Unlock the GUI again
    // TODO: This should be back in the KMComposeWin
    mComposeWin->setEnabled( true );

    // No more jobs. Signal that we're done
    emit done( mRc );
    return;
  }

  if( !mRc ) {
    // Something has gone wrong - stop the process and bail out
    while( !mJobs.isEmpty() ) {
      delete mJobs.front();
      mJobs.pop_front();
    }

    // Unlock the GUI again
    // TODO: This should be back in the KMComposeWin
    mComposeWin->setEnabled( true );

    emit done( false );
    return;
  }

  // We have more jobs to do, but allow others to come first
  QTimer::singleShot( 0, this, SLOT( slotDoNextJob() ) );
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
  mSelectedCryptPlug = mComposeWin->mSelectedCryptPlug;
  mAutoCharset = mComposeWin->mAutoCharset;
  mCharset = mComposeWin->mCharset;
  mMsg = mComposeWin->mMsg;
  mAutoPgpEncrypt = mComposeWin->mAutoPgpEncrypt;
  mPgpIdentity = mComposeWin->pgpIdentity();

  mComposeWin->mBccMsgList.clear();

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
  mMsg->setCharset(mCharset);

  mMsg->setTo(mComposeWin->to());
  mMsg->setFrom(mComposeWin->from());
  mMsg->setCc(mComposeWin->cc());
  mMsg->setSubject(mComposeWin->subject());
  mMsg->setReplyTo(mComposeWin->replyTo());
  mMsg->setBcc(mComposeWin->bcc());

  const KMIdentity & id
    = kmkernel->identityManager()->identityForUoid( mComposeWin->mIdentity->currentIdentity() );

  KMFolder *f = mComposeWin->mFcc->getFolder();
  assert( f != 0 );
  if ( f->idString() == id.fcc() )
    mMsg->removeHeaderField("X-KMail-Fcc");
  else
    mMsg->setFcc( f->idString() );

  // set the correct drafts folder
  mMsg->setDrafts( id.drafts() );

  if (id.isDefault())
    mMsg->removeHeaderField("X-KMail-Identity");
  else mMsg->setHeaderField("X-KMail-Identity", QString::number( id.uoid() ));

  QString replyAddr;
  if (!mComposeWin->replyTo().isEmpty()) replyAddr = mComposeWin->replyTo();
  else replyAddr = mComposeWin->from();

  if (mComposeWin->mRequestMDNAction->isChecked())
    mMsg->setHeaderField("Disposition-Notification-To", replyAddr);
  else
    mMsg->removeHeaderField("Disposition-Notification-To");

  if (mComposeWin->mUrgentAction->isChecked()) {
    mMsg->setHeaderField("X-PRIORITY", "2 (High)");
    mMsg->setHeaderField("Priority", "urgent");
  } else {
    mMsg->removeHeaderField("X-PRIORITY");
    mMsg->removeHeaderField("Priority");
  }

  _StringPair *pCH;
  for (pCH  = mComposeWin->mCustHeaders.first();
       pCH != 0;
       pCH  = mComposeWin->mCustHeaders.next()) {
    mMsg->setHeaderField(KMMsgBase::toUsAscii(pCH->name), pCH->value);
  }

  // we have to remember the Bcc because it might have been overwritten
  // by a custom header (therefore we can't use bcc() later) and because
  // mimelib removes addresses without domain part (therefore we can't use
  // mMsg->bcc() later)
  mBcc = mMsg->bcc();
}

void MessageComposer::adjustCryptFlags()
{
  // Apply the overriding settings
  // And apply the users choice
  bool doSign = !mDontSign;
  bool doEncrypt = !mDontEncrypt;

  // check settings of composer buttons *and* attachment check boxes
  bool doSignCompletely    = doSign;
  bool doEncryptCompletely = doEncrypt;
  bool doEncryptPartially  = doEncrypt;
  if( mSelectedCryptPlug && ( !mComposeWin->mAtmList.isEmpty() ) ) {
    int idx=0;
    KMMessagePart *attachPart;
    for( attachPart = mComposeWin->mAtmList.first();
         attachPart;
         attachPart=mComposeWin->mAtmList.next(), ++idx ) {
      if( mComposeWin->encryptFlagOfAttachment( idx ) ) {
        doEncryptPartially = true;
      }
      else {
        doEncryptCompletely = false;
      }
      if( !mComposeWin->signFlagOfAttachment( idx ) )
        doSignCompletely = false;
    }
  }

  if( !doSignCompletely ) {
    if( mSelectedCryptPlug ) {
      // note: only ask for signing if "Warn me" flag is set! (khz)
      if( mSelectedCryptPlug->warnSendUnsigned() && !mDontSign ) {
        int ret =
        KMessageBox::warningYesNoCancel( mComposeWin,
          QString( "<qt><b>"
            + i18n("Warning:")
            + "</b><br>"
            + ((doSign && !doSignCompletely)
              ? i18n("You specified not to sign some parts of this message, but"
                     " you wanted to be warned not to send unsigned messages!")
              : i18n("You specified not to sign this message, but"
                     " you wanted to be warned not to send unsigned messages!") )
            + "<br>&nbsp;<br><b>"
            + i18n("Sign all parts of this message?")
            + "</b></qt>" ),
          i18n("Signature Warning"),
          KGuiItem( i18n("&Sign All Parts") ),
          KGuiItem( i18n("Send &as is") ) );
        if( ret == KMessageBox::Cancel ) {
          mRc = false;
          return;
        } else if( ret == KMessageBox::Yes ) {
          doSign = true;
          doSignCompletely = true;
        }
      }
    } else {
      // ask if the message should be encrypted via old build-in pgp code
      // pending (who ever wants to implement it)
    }
  }

  if( !mDontEncrypt ) {
    // check whether all encrypted messages should be encrypted to self
    bool bEncryptToSelf = mSelectedCryptPlug
      ? mSelectedCryptPlug->alwaysEncryptToSelf()
      : Kpgp::Module::getKpgp()->encryptToSelf();
    // check whether we have the user's key if necessary
    bool bEncryptionPossible = !bEncryptToSelf || !mPgpIdentity.isEmpty();
    // check whether we are using OpenPGP (built-in or plug-in)
    bool bUsingOpenPgp = !mSelectedCryptPlug ||
      ( mSelectedCryptPlug &&
        ( -1 != mSelectedCryptPlug->libName().find( "openpgp" ) ) );
    // only try automatic encryption if all of the following conditions hold
    // a) the user enabled automatic encryption
    // b) we have the user's key if he wants to encrypt to himself
    // c) we are using OpenPGP
    // d) no message part is marked for encryption
    if( mAutoPgpEncrypt && bEncryptionPossible && bUsingOpenPgp &&
        !doEncryptPartially ) {
      // check if encryption is possible and if yes suggest encryption
      // first determine the complete list of recipients
      QString _to = mComposeWin->to().simplifyWhiteSpace();
      if( !mComposeWin->cc().isEmpty() ) {
        if( !_to.endsWith(",") )
          _to += ",";
        _to += mComposeWin->cc().simplifyWhiteSpace();
      }
      if( !mBcc.isEmpty() ) {
        if( !_to.endsWith(",") )
          _to += ",";
        _to += mBcc.simplifyWhiteSpace();
      }
      QStringList allRecipients = KMMessage::splitEmailAddrList(_to);
      // now check if encrypting to these recipients is possible and desired
      Kpgp::Module *pgp = Kpgp::Module::getKpgp();
      int status = pgp->encryptionPossible( allRecipients );
      if( 1 == status ) {
        // encrypt all message parts
        doEncrypt = true;
        doEncryptCompletely = true;
      } else if( 2 == status ) {
        // the user wants to be asked or has to be asked
        KCursorSaver idle(KBusyPtr::idle());
        int ret;
        if( doSign )
          ret = KMessageBox::questionYesNoCancel( mComposeWin,
                                                  i18n("<qt><p>You have a trusted OpenPGP key for every "
                                                       "recipient of this message and the message will "
                                                       "be signed.</p>"
                                                       "<p>Should this message also be "
                                                       "encrypted?</p></qt>"),
                                                  i18n("Encrypt Message?"),
                                                  KGuiItem( i18n("Sign && &Encrypt") ),
                                                  KGuiItem( i18n("&Sign Only") ) );
        else
          ret = KMessageBox::questionYesNoCancel( mComposeWin,
                                                  i18n("<qt><p>You have a trusted OpenPGP key for every "
                                                       "recipient of this message.</p>"
                                                       "<p>Should this message be encrypted?</p></qt>"),
                                                  i18n("Encrypt Message?"),
                                                  KGuiItem( i18n("&Encrypt") ),
                                                  KGuiItem( i18n("&Don't Encrypt") ) );
        if( KMessageBox::Cancel == ret ) {
          mRc = false;
          return;
        } else if( KMessageBox::Yes == ret ) {
          // encrypt all message parts
          doEncrypt = true;
          doEncryptCompletely = true;
        }
      } else if( status == -1 ) {
        // warn the user that there are conflicting encryption preferences
        KCursorSaver idle(KBusyPtr::idle());
        int ret =
          KMessageBox::warningYesNoCancel( mComposeWin,
                                           i18n("<qt><p>There are conflicting encryption "
                                                "preferences!</p>"
                                                "<p>Should this message be encrypted?</p></qt>"),
                                           i18n("Encrypt Message?"),
                                           KGuiItem( i18n("&Encrypt") ),
                                           KGuiItem( i18n("&Don't Encrypt") ) );
        if( KMessageBox::Cancel == ret ) {
          mRc = false;
          return;
        } else if( KMessageBox::Yes == ret ) {
          // encrypt all message parts
          doEncrypt = true;
          doEncryptCompletely = true;
        }
      }
    } else if( !doEncryptCompletely && mSelectedCryptPlug ) {
      // note: only ask for encrypting if "Warn me" flag is set! (khz)
      if( mSelectedCryptPlug->warnSendUnencrypted() ) {
        int ret =
          KMessageBox::warningYesNoCancel( mComposeWin,
                                           QString( "<qt><b>"
                                                    + i18n("Warning:")
                                                    + "</b><br>"
                                                    + ((doEncrypt && !doEncryptCompletely)
                                                       ? i18n("You specified not to encrypt some parts of this message, but"
                                                              " you wanted to be warned not to send unencrypted messages!")
                                                       : i18n("You specified not to encrypt this message, but"
                                                              " you wanted to be warned not to send unencrypted messages!") )
                                                    + "<br>&nbsp;<br><b>"
                                                    + i18n("Encrypt all parts of this message?")
                                                    + "</b></qt>" ),
                                           i18n("Encryption Warning"),
                                           KGuiItem( i18n("&Encrypt All Parts") ),
                                           KGuiItem( i18n("Send &as is") ) );
        if( ret == KMessageBox::Cancel ) {
          mRc = false;
          return;
        } else if( ret == KMessageBox::Yes ) {
          doEncrypt = true;
          doEncryptCompletely = true;
        }
      }

      /*
        note: Processing the mSelectedCryptPlug->encryptEmail() flag here would
        be absolutely wrong: this is used for specifying
        if messages should be encrypted 'in general'.
        --> This sets the initial state of a freshly started Composer.
        --> This does *not* mean overriding user setting made while
        editing in that composer window!         (khz, 2002/06/26)
      */
    }
  }

  // if necessary mark all attachments for signing/encryption
  if( mSelectedCryptPlug && ( !mComposeWin->mAtmList.isEmpty() ) &&
      ( doSignCompletely || doEncryptCompletely ) ) {
    for( KMAtmListViewItem* lvi = (KMAtmListViewItem*)mComposeWin->mAtmItemList.first();
         lvi; lvi = (KMAtmListViewItem*)mComposeWin->mAtmItemList.next() ) {
      if( doSignCompletely )
        lvi->setSign( true );
      if( doEncryptCompletely )
        lvi->setEncrypt( true );
    }
  }
}

void MessageComposer::composeMessage()
{
  mExtraMessage = new KMMessage( *mMsg );
  mJobs.push_front( new ComposeMessage2Job( this ) );
  composeMessage( *mMsg, !mDontSign, !mDontEncrypt, false );
}

void MessageComposer::composeMessage2()
{
  bool saveMessagesEncrypted = mSelectedCryptPlug ?
    mSelectedCryptPlug->saveMessagesEncrypted() : true;

  // note: The following define is specified on top of this file. To compile
  //       a less strict version of KMail just comment it out there above.
#ifdef STRICT_RULES_OF_GERMAN_GOVERNMENT_01
  // Hack to make sure the S/MIME CryptPlugs follows the strict requirement
  // of german government:
  // --> Encrypted messages *must* be stored in unencrypted form after sending.
  //     ( "Abspeichern ausgegangener Nachrichten in entschluesselter Form" )
  // --> Signed messages *must* be stored including the signature after sending.
  //     ( "Aufpraegen der Signatur" )
  // So we provide the user with a non-deactivateble warning and let her/him
  // choose to obey the rules or to ignore them explicitly.
  if( mSelectedCryptPlug
      && ( 0 <= mSelectedCryptPlug->libName().find( "smime", 0, false ) )
      && ( doEncrypt && saveMessagesEncrypted ) ) {
    QString headTxt = i18n("Warning: Your S/MIME Plug-in configuration is unsafe.");
    QString encrTxt = i18n("Encrypted messages should be stored in unencrypted form; saving locally in encrypted form is not allowed.");
    QString footTxt = i18n("Please correct the wrong settings in KMail's Plug-in configuration pages as soon as possible.");
    QString question = i18n("Store message in the recommended way?");
    if( KMessageBox::Yes == KMessageBox::warningYesNo( mComposeWin,
                                                       "<qt><p><b>" + headTxt + "</b><br>" + encrTxt + "</p><p>"
                                                       + footTxt + "</p><p><b>" + question + "</b></p></qt>",
                                                       i18n("Unsafe S/MIME Configuration"),
                                                       KGuiItem( i18n("Save &Unencrypted") ),
                                                       KGuiItem( i18n("Save &Encrypted") ) ) )
      saveMessagesEncrypted = false;
  }
  kdDebug(5006) << "MessageComposer::applyChanges(void)  -  Send encrypted=" << doEncrypt << "  Store encrypted=" << saveMessagesEncrypted << endl;
#endif
  if( !mDontEncrypt && !saveMessagesEncrypted ) {
    if( mSelectedCryptPlug ) {
      for( KMAtmListViewItem* entry = (KMAtmListViewItem*)mComposeWin->mAtmItemList.first();
           entry;
           entry = (KMAtmListViewItem*)mComposeWin->mAtmItemList.next() )
        entry->setEncrypt( false );
    }
    mJobs.push_front( new ComposeMessage3Job( this ) );
    composeMessage( *mExtraMessage, !mDontSign, false, true );
  } else
    delete mExtraMessage;
}

void MessageComposer::composeMessage3()
{
  mExtraMessage->cleanupHeader();
  mMsg->setUnencryptedMsg( mExtraMessage );
  mExtraMessage = 0;
}


class SignSMimeJob : public MessageComposerJob {
public:
  SignSMimeJob( CryptPlugWrapper* selectedCryptPlug, QCString& encodedBody,
                uint boundaryLevel, KMMessagePart& oldBodyPart,
                const QCString& charset, KMMessagePart *newBodyPart,
                MessageComposer* composer )
    : MessageComposerJob( composer ), mSelectedCryptPlug( selectedCryptPlug ),
      mEncodedBody( encodedBody ), mBoundaryLevel( boundaryLevel ),
      mOldBodyPart( oldBodyPart ), mCharset( charset ),
      mNewBodyPart( newBodyPart ), mStructuring( selectedCryptPlug )
  {
    mBugUrl = QString::fromUtf8( mSelectedCryptPlug->bugURL() );
  }

  void execute() {
    kdDebug(5006) << "************** mEncodedBody:\n" << mEncodedBody << endl;

    if( !mComposer->mSignature.isEmpty() ) {
      mComposer->mRc = mComposer->
        processStructuringInfo( mBugUrl, mBoundaryLevel,
                                mOldBodyPart.contentDescription(),
                                mOldBodyPart.typeStr(),
                                mOldBodyPart.subtypeStr(),
                                mOldBodyPart.contentDisposition(),
                                mOldBodyPart.contentTransferEncodingStr(),
                                mEncodedBody, "signature",
                                mComposer->mSignature, mStructuring,
                                *mNewBodyPart );
      if( mComposer->mRc ) {
        if( !mStructuring.data.makeMultiMime ) {
          mNewBodyPart->setCharset( mCharset );
        }
      } else
        KMessageBox::sorry( mComposer->mComposeWin,
                            mErrorProcessingStructuringInfo );
    } else
      mComposer->mRc = false;
  }

  StructuringInfoWrapper& structuringInfo() { return mStructuring; }

private:
  CryptPlugWrapper* mSelectedCryptPlug;
  QString mBugUrl;
  QCString &mEncodedBody;
  uint mBoundaryLevel;
  KMMessagePart& mOldBodyPart;
  QCString mCharset;
  KMMessagePart* mNewBodyPart;
  StructuringInfoWrapper mStructuring;
};

#if 0
// This class is not yet implemented. Should get the last part of the
// gpg signing done
class SignOldGPGJob : public MessageComposerJob {
public:
  SignJob( CryptPlugWrapper* selectedCryptPlug, const QCString& encodedBody,
           uint boundaryLevel, const KMMessagePart& oldBodyPart,
           const QCString& charset, const QCString& pgpIdentity,
           KMMessagePart *newBodyPart, MessageComposer* composer )
    : MessageComposerJob( composer ), mSelectedCryptPlug( selectedCryptPlug ),
      mEncodedBody( encodedBody ), mBoundaryLevel( boundaryLevel ),
      mOldBodyPart( oldBodyPart ), mCharset( charset ),
      mPgpIdentity( pgpIdentity ), mNewBodyPart( newBodyPart ) {}

  void execute() {
    adjustCryptFlags();
  }

private:
  CryptPlugWrapper* mSelectedCryptPlug;
  QCString mEncodedBody;
  uint mBoundaryLevel;
  KMMessagePart mOldBodyPart;
  QCString mCharset;
  QCString mPgpIdentity;
  KMMessagePart *mNewBodyPart;
};
#endif

class EncryptMessageJob : public MessageComposerJob {
public:
  EncryptMessageJob( const KMMessage& msg, const QStringList& recipients,
                     bool doSign, bool doEncrypt, const QCString& encodedBody,
                     int boundaryLevel, const KMMessagePart& oldBodyPart,
                     KMMessagePart* newBodyPart, MessageComposer* composer )
    : MessageComposerJob( composer ), mRecipients( recipients ),
      mDoSign( doSign ), mDoEncrypt( doEncrypt ), mEncodedBody( encodedBody ),
      mBoundaryLevel( boundaryLevel ), mOldBodyPart( oldBodyPart ),
      mBCCMessage( true ), mNewBodyPart( newBodyPart )
  {
    mMsg = new KMMessage( msg );
  }

  EncryptMessageJob( KMMessage* msg, const QStringList& recipients,
                     bool doSign, bool doEncrypt, const QCString& encodedBody,
                     int boundaryLevel, const KMMessagePart& oldBodyPart,
                     KMMessagePart* newBodyPart, MessageComposer* composer )
    : MessageComposerJob( composer ), mMsg( msg ), mRecipients( recipients ),
      mDoSign( doSign ), mDoEncrypt( doEncrypt ), mEncodedBody( encodedBody ),
      mBoundaryLevel( boundaryLevel ), mOldBodyPart( oldBodyPart ),
      mBCCMessage( false ), mNewBodyPart( newBodyPart ) {}

  void execute() {
    KMMessagePart tmpNewBodyPart = *mNewBodyPart;

    // TODO: Async call

    mComposer->encryptMessage( mMsg, mRecipients, mDoSign, mDoEncrypt,
                               mBoundaryLevel, tmpNewBodyPart );
    if( mComposer->mRc && mBCCMessage ) {
      mMsg->setHeaderField( "X-KMail-Recipients", mRecipients.last() );
      mComposer->handleBCCMessage( mMsg );
    }
  }

private:
  KMMessage* mMsg;
  QStringList mRecipients;
  bool mDoSign, mDoEncrypt;
  QCString mEncodedBody;
  int mBoundaryLevel;
  KMMessagePart mOldBodyPart;
  bool mBCCMessage;
  KMMessagePart* mNewBodyPart;
};

void MessageComposer::handleBCCMessage( KMMessage* msg )
{
  mComposeWin->mBccMsgList.append( msg );
}

void MessageComposer::composeMessage( KMMessage& theMessage,
                                      bool doSign, bool doEncrypt,
                                      bool ignoreBcc )
{
  // create informative header for those that have no mime-capable
  // email client
  theMessage.setBody( "This message is in MIME format." );

  // preprocess the body text
  QCString body = breakLinesAndApplyCodec();

  if (body.isNull()) {
    mRc = false;
    return;
  }

  if (body.isEmpty()) body = "\n"; // don't crash

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
  if( body[body.length()-1] != '\n' ) {
    kdDebug(5006) << "Added an <LF> on the last line" << endl;
    body += "\n";
  }

  // set the main headers
  theMessage.deleteBodyParts();
  theMessage.removeHeaderField("Content-Type");
  theMessage.removeHeaderField("Content-Transfer-Encoding");
  theMessage.setAutomaticFields(TRUE); // == multipart/mixed

  // this is our *final* body part
  mNewBodyPart = new KMMessagePart;

  // this is the boundary depth of the surrounding MIME part
  mPreviousBoundaryLevel = 0;

  // create temporary bodyPart for editor text
  // (and for all attachments, if mail is to be singed and/or encrypted)
  mEarlyAddAttachments = mSelectedCryptPlug &&
    ( !mComposeWin->mAtmList.isEmpty() ) && ( doSign || doEncrypt );

  mAllAttachmentsAreInBody = mEarlyAddAttachments;

  // test whether there ARE attachments that can be included into the body
  if( mEarlyAddAttachments ) {
    bool someOk = false;
    int idx;
    KMMessagePart *attachPart;
    for( idx=0, attachPart = mComposeWin->mAtmList.first();
         attachPart; attachPart=mComposeWin->mAtmList.next(), ++idx )
      if( doEncrypt == mComposeWin->encryptFlagOfAttachment( idx )
          && doSign == mComposeWin->signFlagOfAttachment( idx ) )
        someOk = true;
      else
        mAllAttachmentsAreInBody = false;
    if( !mAllAttachmentsAreInBody && !someOk )
      mEarlyAddAttachments = false;
  }

  mOldBodyPart.setTypeStr( mEarlyAddAttachments ? "multipart" : "text" );
  mOldBodyPart.setSubtypeStr( mEarlyAddAttachments ? "mixed" : "plain" );
  mOldBodyPart.setContentDisposition( "inline" );

  QCString boundaryCStr;

  if( mEarlyAddAttachments ) {
    // calculate a boundary string
    ++mPreviousBoundaryLevel;
    DwMediaType tmpCT;
    tmpCT.CreateBoundary( mPreviousBoundaryLevel );
    boundaryCStr = tmpCT.Boundary().c_str();
    // add the normal body text
    KMMessagePart innerBodyPart;
    innerBodyPart.setTypeStr( "text" );
    innerBodyPart.setSubtypeStr( "plain" );
    innerBodyPart.setContentDisposition( "inline" );
    QValueList<int> allowedCTEs;
    // the signed body must not be 8bit encoded
    innerBodyPart.setBodyAndGuessCte( body, allowedCTEs,
                                      !kmkernel->msgSender()->sendQuotedPrintable() && !doSign,
                                      doSign );
    innerBodyPart.setCharset( mCharset );
    innerBodyPart.setBodyEncoded( body );
    DwBodyPart* innerDwPart = theMessage.createDWBodyPart( &innerBodyPart );
    innerDwPart->Assemble();
    body = "--" + boundaryCStr + "\n" + innerDwPart->AsString().c_str();
    delete innerDwPart;
    innerDwPart = 0;
    // add all matching Attachments
    // NOTE: This code will be changed when KMime is complete.
    int idx = 0;
    KMMessagePart* attachPart = mComposeWin->mAtmList.first();
    for( ; attachPart; attachPart=mComposeWin->mAtmList.next(), ++idx ) {
      bool bEncrypt = mComposeWin->encryptFlagOfAttachment( idx );
      bool bSign = mComposeWin->signFlagOfAttachment( idx );
      if( !mSelectedCryptPlug || ( ( doEncrypt == bEncrypt )
                                   && ( doSign == bSign ) ) ) {
        // signed/encrypted body parts must be either QP or base64 encoded
        // Why not 7 bit? Because the LF->CRLF canonicalization would render
        // e.g. 7 bit encoded shell scripts unusable because of the CRs.
        if( bSign || bEncrypt ) {
          QCString cte = attachPart->cteStr().lower();
          if( ( "8bit" == cte )
              || ( ( attachPart->type() == DwMime::kTypeText )
                   && ( "7bit" == cte ) ) ) {
            QByteArray body = attachPart->bodyDecodedBinary();
            QValueList<int> dummy;
            attachPart->setBodyAndGuessCte(body, dummy, false, bSign);
            kdDebug(5006) << "Changed encoding of message part from "
                          << cte << " to " << attachPart->cteStr() << endl;
          }
        }
        innerDwPart = theMessage.createDWBodyPart( attachPart );
        innerDwPart->Assemble();
        body += "\n--" + boundaryCStr + "\n" + innerDwPart->AsString().c_str();
        delete innerDwPart;
        innerDwPart = 0;
      }
    }
    body += "\n--" + boundaryCStr + "--\n";
  } else {
    QValueList<int> allowedCTEs;
    // the signed body must not be 8bit encoded
    mOldBodyPart.setBodyAndGuessCte(body, allowedCTEs, !kmkernel->msgSender()->sendQuotedPrintable() && !doSign,
                                   doSign);
    mOldBodyPart.setCharset(mCharset);
  }
  // create S/MIME body part for signing and/or encrypting
  mOldBodyPart.setBodyEncoded( body );

  if( doSign || doEncrypt ) {
    if( mSelectedCryptPlug ) {
      // get string representation of body part (including the attachments)
      DwBodyPart* dwPart = theMessage.createDWBodyPart( &mOldBodyPart );
      dwPart->Assemble();
      mEncodedBody = dwPart->AsString().c_str();
      delete dwPart;
      dwPart = 0;

      // manually add a boundary definition to the Content-Type header
      if( !boundaryCStr.isEmpty() ) {
        int boundPos = mEncodedBody.find( '\n' );
        if( -1 < boundPos ) {
          // insert new "boundary" parameter
          QCString bStr( ";\n  boundary=\"" );
          bStr += boundaryCStr;
          bStr += "\"";
          mEncodedBody.insert( boundPos, bStr );
        }
      }

      // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
      // according to RfC 2633, 3.1.1 Canonicalization
      kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
      mEncodedBody = KMMessage::lf2crlf( mEncodedBody );
    } else {
      mEncodedBody = body;
    }
  }

  MessageComposerJob* signJob = 0;
  if( doSign ) {
    if( mSelectedCryptPlug ) {
      // TODO: Async call
      SignSMimeJob* job = new SignSMimeJob( mSelectedCryptPlug, mEncodedBody,
                                            mPreviousBoundaryLevel + doEncrypt ? 3 : 2,
                                            mOldBodyPart, mCharset,
                                            mNewBodyPart, this );
      signJob = job;
      pgpSignedMsg( mEncodedBody, job->structuringInfo() );
    } else if( !doEncrypt ) {
      // we try calling the *old* build-in code for OpenPGP clearsigning
      Kpgp::Block block;
      block.setText( mEncodedBody );

      // clearsign the message

      // TODO: Async call

      Kpgp::Result result = block.clearsign( mPgpIdentity, mCharset );

      if( result == Kpgp::Ok ) {
        mNewBodyPart->setType( mOldBodyPart.type() );
        mNewBodyPart->setSubtype( mOldBodyPart.subtype() );
        mNewBodyPart->setCharset( mOldBodyPart.charset() );
        mNewBodyPart->setContentTransferEncodingStr( mOldBodyPart.contentTransferEncodingStr() );
        mNewBodyPart->setContentDescription( mOldBodyPart.contentDescription() );
        mNewBodyPart->setContentDisposition( mOldBodyPart.contentDisposition() );
        mNewBodyPart->setBodyEncoded( block.text() );
      } else {
        mRc = false;
        KMessageBox::sorry( mComposeWin,
                            i18n( "<qt><p>This message could not be signed.</p>%1</qt>")
                            .arg( mErrorNoCryptPlugAndNoBuildIn ) );
      }
    }
  }

  if( signJob ) {
    mJobs.push_front( new ContinueComposeMessageJob( this, theMessage, doSign,
                                                     doEncrypt, ignoreBcc ) );
    mJobs.push_front( signJob );
  } else
    // No signing going on, so just continue
    continueComposeMessage( theMessage, doSign, doEncrypt, ignoreBcc );
}

// Do the encryption stuff
void MessageComposer::continueComposeMessage( KMMessage& theMessage,
                                              bool doSign, bool doEncrypt,
                                              bool ignoreBcc )
{
  // determine the list of public recipients
  QString _to = mComposeWin->to().simplifyWhiteSpace();
  if( !mComposeWin->cc().isEmpty() ) {
    if( !_to.endsWith(",") )
      _to += ",";
    _to += mComposeWin->cc().simplifyWhiteSpace();
  }
  QStringList recipientsWithoutBcc = KMMessage::splitEmailAddrList(_to);

  // run encrypting(s) for Bcc recipient(s)
  if( doEncrypt && !ignoreBcc && !theMessage.bcc().isEmpty() ) {
    QStringList bccRecips = KMMessage::splitEmailAddrList( theMessage.bcc() );
    for( QStringList::ConstIterator it = bccRecips.begin();
         it != bccRecips.end(); ++it ) {
      QStringList tmpRecips( recipientsWithoutBcc );
      tmpRecips << *it;
      // Note: This will do the encryptions in reverse order, compared
      // to the old synchronous code. Hopefully, this doesn't matter
      mJobs.push_front( new EncryptMessageJob( theMessage, tmpRecips, doSign,
                                               doEncrypt, mEncodedBody,
                                               mPreviousBoundaryLevel,
                                               mOldBodyPart, mNewBodyPart,
                                               this ) );
    }
    theMessage.setHeaderField( "X-KMail-Recipients",
                               recipientsWithoutBcc.join(",") );
  }

  // Run encrypting for public recipient(s)
  mJobs.push_front( new EncryptMessageJob( mMsg, recipientsWithoutBcc, doSign,
                                           doEncrypt, mEncodedBody,
                                           mPreviousBoundaryLevel,
                                           mOldBodyPart, mNewBodyPart,
                                           this ) );
}

Kpgp::Result MessageComposer::getEncryptionCertificates( const QStringList& recipients,
                                                         QCString& encryptionCertificates )
{
  Kpgp::Result result = Kpgp::Ok;

  // find out whether we are dealing with the OpenPGP or the S/MIME plugin
  if ( -1 != mSelectedCryptPlug->libName().find( "openpgp" ) ) {
    // We are dealing with the OpenPGP plugin. Use Kpgp to determine
    // the encryption keys.
    // get the OpenPGP key ID for the chosen identity
    Kpgp::Module *pgp = Kpgp::Module::getKpgp();
    Kpgp::KeyIDList encryptionKeyIds;

    // temporarily set encrypt_to_self to the value specified in the
    // plugin configuration. this value is used implicitely by the
    // function which determines the encryption keys.
    const bool bEncryptToSelf_Old = pgp->encryptToSelf();
    pgp->setEncryptToSelf( mSelectedCryptPlug->alwaysEncryptToSelf() );
    result = pgp->getEncryptionKeys( encryptionKeyIds, recipients, mPgpIdentity );
    // reset encrypt_to_self to the old value
    pgp->setEncryptToSelf( bEncryptToSelf_Old );

    if ( result == Kpgp::Ok && !encryptionKeyIds.isEmpty() ) {
      // loop over all key IDs
      for ( Kpgp::KeyIDList::ConstIterator it = encryptionKeyIds.begin();
           it != encryptionKeyIds.end(); ++it ) {
        const Kpgp::Key* key = pgp->publicKey( *it );
        if ( key ) {
          QCString certFingerprint = key->primaryFingerprint();
          kdDebug(5006) << "Fingerprint of encryption key: "
                        << certFingerprint << endl;
          // add this key to the list of encryption keys
          if( !encryptionCertificates.isEmpty() )
            encryptionCertificates += '\1';
          encryptionCertificates += certFingerprint;
        }
      }
    }
  } else {
    // S/MIME
    QStringList allRecipients = recipients;
    if ( mSelectedCryptPlug->alwaysEncryptToSelf() )
      allRecipients << mComposeWin->from();
    for ( QStringList::ConstIterator it = allRecipients.begin();
          it != allRecipients.end();
          ++it ) {
      QCString certFingerprint = getEncryptionCertificate( *it );

      if ( certFingerprint.isEmpty() ) {
        // most likely the user canceled the certificate selection
        encryptionCertificates.truncate( 0 );
        return Kpgp::Canceled;
      }

      certFingerprint.remove( 0, certFingerprint.findRev( '(' ) + 1 );
      certFingerprint.truncate( certFingerprint.length() - 1 );
      kdDebug(5006) << "\n\n                    Recipient: " << *it
                    <<   "\nFingerprint of encryption key: "
                    << certFingerprint << "\n\n" << endl;

      const bool certOkay =
        checkForEncryptCertificateExpiry( *it, certFingerprint );
      if( certOkay ) {
        if( !encryptionCertificates.isEmpty() )
          encryptionCertificates += '\1';
        encryptionCertificates += certFingerprint;
      }
      else {
        // ###### This needs to be improved: Tell the user that the certificate
        // ###### expired and let him choose a different one.
        encryptionCertificates.truncate( 0 );
        return Kpgp::Failure;
      }
    }
  }
  return result;
}

void MessageComposer::encryptMessage( KMMessage* msg,
                                      const QStringList& recipients,
                                      bool doSign, bool doEncrypt,
                                      int previousBoundaryLevel,
                                      KMMessagePart newBodyPart )
{
  // This c-string (init empty here) is set by *first* testing of expiring
  // encryption certificate: stops us from repeatedly asking same questions.
  QCString encryptCertFingerprints;

  // determine the encryption certificates in case we need them
  if ( mSelectedCryptPlug ) {
    bool encrypt = doEncrypt;
    if( !encrypt ) {
      // check whether at least one attachment is marked for encryption
      for ( KMAtmListViewItem* atmlvi =
              static_cast<KMAtmListViewItem*>( mComposeWin->mAtmItemList.first() );
            atmlvi;
            atmlvi = static_cast<KMAtmListViewItem*>( mComposeWin->mAtmItemList.next() ) ) {
        if ( atmlvi->isEncrypt() ) {
          encrypt = true;
          break;
        }
      }
    }
    if ( encrypt ) {
      Kpgp::Result result =
        getEncryptionCertificates( recipients, encryptCertFingerprints );
      if ( result != Kpgp::Ok ) {
        mRc = false;
        return;
      }
      if ( encryptCertFingerprints.isEmpty() ) {
        // the user wants to send the message unencrypted
        mComposeWin->setEncryption( false, false );
        doEncrypt = false;
      }
    }
  }

  // encrypt message
  if( doEncrypt ) {
    QCString innerContent;
    if( doSign && mSelectedCryptPlug ) {
      DwBodyPart* dwPart = msg->createDWBodyPart( &newBodyPart );
      dwPart->Assemble();
      innerContent = dwPart->AsString().c_str();
      delete dwPart;
      dwPart = 0;
    } else
      innerContent = mEncodedBody;

    // now do the encrypting:
    {
      if( mSelectedCryptPlug ) {
	// replace simple LFs by CRLFs for all MIME supporting CryptPlugs
	// according to RfC 2633, 3.1.1 Canonicalization
	kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
	innerContent = KMMessage::lf2crlf( innerContent );
	kdDebug(5006) << "                                                       done." << endl;

        StructuringInfoWrapper structuring( mSelectedCryptPlug );

        QByteArray encryptedBody;
        Kpgp::Result result = pgpEncryptedMsg( encryptedBody, innerContent,
                                               structuring,
                                               encryptCertFingerprints );

        if( Kpgp::Ok == result ) {
          mRc = processStructuringInfo( QString::fromUtf8( mSelectedCryptPlug->bugURL() ),
                                        previousBoundaryLevel + doEncrypt ? 2 : 1,
                                        newBodyPart.contentDescription(),
                                        newBodyPart.typeStr(),
                                        newBodyPart.subtypeStr(),
                                        newBodyPart.contentDisposition(),
                                        newBodyPart.contentTransferEncodingStr(),
                                        innerContent,
                                        "encrypted data",
                                        encryptedBody,
                                        structuring,
                                        newBodyPart );
          if ( !mRc )
            KMessageBox::sorry(mComposeWin, mErrorProcessingStructuringInfo);
        } else {
          mRc = false;
          KMessageBox::sorry( mComposeWin,
                              i18n( "<qt><p><b>This message could not be encrypted!</b></p>"
                                    "<p>The Crypto Plug-in '%1' did not return an encoded text "
                                    "block.</p>"
                                    "<p>Probably a recipient's public key was not found or is "
                                    "untrusted.</p></qt>" )
                              .arg( mSelectedCryptPlug->libName() ) );
        }
      } else {
        // we try calling the *old* build-in code for OpenPGP encrypting
        Kpgp::Block block;
        block.setText( innerContent );

        // encrypt the message
        Kpgp::Result result =
          block.encrypt( recipients, mPgpIdentity, doSign, mCharset );

        if( Kpgp::Ok == result ) {
          newBodyPart.setBodyEncodedBinary( block.text() );
          newBodyPart.setCharset( mOldBodyPart.charset() );
        } else {
          mRc = false;
          KMessageBox::sorry(mComposeWin,
            i18n("<qt><p>This message could not be encrypted!</p>%1</qt>")
           .arg( mErrorNoCryptPlugAndNoBuildIn ));
        }
      }
    }
  }

  // process the attachments that are not included into the body
  if( mRc ) {
    const KMMessagePart& ourFineBodyPart( (doSign || doEncrypt)
                                          ? newBodyPart : mOldBodyPart );
    if( !mComposeWin->mAtmList.isEmpty()
        && ( !mEarlyAddAttachments || !mAllAttachmentsAreInBody ) ) {
      // set the content type header
      msg->headers().ContentType().FromString( "Multipart/Mixed" );
      kdDebug(5006) << "MessageComposer::encryptMessage() : set top level Content-Type to Multipart/Mixed" << endl;
      //      msg->setBody( "This message is in MIME format.\n"
      //                    "Since your mail reader does not understand this format,\n"
      //                    "some or all parts of this message may not be legible." );
      // add our Body Part
      msg->addBodyPart( &ourFineBodyPart );

      // add Attachments
      // create additional bodyparts for the attachments (if any)
      int idx;
      KMMessagePart newAttachPart;
      KMMessagePart *attachPart;
      for( idx=0, attachPart = mComposeWin->mAtmList.first();
           attachPart;
           attachPart = mComposeWin->mAtmList.next(), ++idx ) {
        kdDebug(5006) << "                                 processing " << idx << ". attachment" << endl;

        const bool cryptFlagsDifferent = mSelectedCryptPlug
          ? ((mComposeWin->encryptFlagOfAttachment( idx ) != doEncrypt)
             || (mComposeWin->signFlagOfAttachment( idx ) != doSign) )
          : false;
        // CONTROVERSIAL
        const bool encryptThisNow = doEncrypt && ( cryptFlagsDifferent ? mComposeWin->encryptFlagOfAttachment( idx ) : false );
        const bool signThisNow  = doSign && ( cryptFlagsDifferent ? mComposeWin->signFlagOfAttachment( idx ) : false );

        if( cryptFlagsDifferent || !mEarlyAddAttachments ) {

          if( encryptThisNow || signThisNow ) {

            KMMessagePart& rEncryptMessagePart( *attachPart );

            // prepare the attachment's content
            // signed/encrypted body parts must be either QP or base64 encoded
            QCString cte = attachPart->cteStr().lower();
            if( ( "8bit" == cte )
                || ( ( attachPart->type() == DwMime::kTypeText )
                     && ( "7bit" == cte ) ) ) {
              QByteArray body = attachPart->bodyDecodedBinary();
              QValueList<int> dummy;
              attachPart->setBodyAndGuessCte(body, dummy, false, true);
              kdDebug(5006) << "Changed encoding of message part from "
                            << cte << " to " << attachPart->cteStr() << endl;
            }
            DwBodyPart* innerDwPart = msg->createDWBodyPart( attachPart );
            innerDwPart->Assemble();
            QCString encodedAttachment = innerDwPart->AsString().c_str();
            delete innerDwPart;
            innerDwPart = 0;

	    // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
	    // according to RfC 2633, 3.1.1 Canonicalization
	    kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
	    encodedAttachment = KMMessage::lf2crlf( encodedAttachment );
	    kdDebug(5006) << "                                                       done." << endl;

            // sign this attachment
            if( signThisNow ) {
              kdDebug(5006) << "                                 sign " << idx << ". attachment separately" << endl;
              StructuringInfoWrapper structuring( mSelectedCryptPlug );

              pgpSignedMsg( encodedAttachment, structuring );
              QByteArray signature = mSignature;
              mRc = !signature.isEmpty();
              if( mRc ) {
                mRc = processStructuringInfo( QString::fromUtf8( mSelectedCryptPlug->bugURL() ),
                                                 previousBoundaryLevel + 10 + idx,
                                                 attachPart->contentDescription(),
                                                 attachPart->typeStr(),
                                                 attachPart->subtypeStr(),
                                                 attachPart->contentDisposition(),
                                                 attachPart->contentTransferEncodingStr(),
                                                 encodedAttachment,
                                                 "signature",
                                                 signature,
                                                 structuring,
                                                 newAttachPart );
                if( mRc ) {
                  if( encryptThisNow ) {
                    rEncryptMessagePart = newAttachPart;
                    DwBodyPart* dwPart = msg->createDWBodyPart( &newAttachPart );
                    dwPart->Assemble();
                    encodedAttachment = dwPart->AsString().c_str();
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
              kdDebug(5006) << " encrypt " << idx << ". attachment separately" << endl;
              StructuringInfoWrapper structuring( mSelectedCryptPlug );
              QByteArray encryptedBody;
              Kpgp::Result result = pgpEncryptedMsg( encryptedBody,
                                                     encodedAttachment,
                                                     structuring,
                                                     encryptCertFingerprints );

              if( Kpgp::Ok == result ) {
                mRc = processStructuringInfo( QString::fromUtf8( mSelectedCryptPlug->bugURL() ),
                                              previousBoundaryLevel + 11 + idx,
                                              rEncryptMessagePart.contentDescription(),
                                              rEncryptMessagePart.typeStr(),
                                              rEncryptMessagePart.subtypeStr(),
                                              rEncryptMessagePart.contentDisposition(),
                                              rEncryptMessagePart.contentTransferEncodingStr(),
                                              encodedAttachment,
                                              "encrypted data",
                                              encryptedBody,
                                              structuring,
                                              newAttachPart );
                if ( !mRc )
                  KMessageBox::sorry( mComposeWin, mErrorProcessingStructuringInfo );
              } else
                mRc = false;
            }
            msg->addBodyPart( &newAttachPart );
          } else
            msg->addBodyPart( attachPart );

          kdDebug(5006) << "                                 added " << idx << ". attachment to this Multipart/Mixed" << endl;
        } else {
          kdDebug(5006) << "                                 " << idx << ". attachment was part of the BODY already" << endl;
        }
      }
    } else {
      if( ourFineBodyPart.originalContentTypeStr() ) {
        //msg->headers().Assemble();
        //kdDebug(5006) << "\n\n\nMessageComposer::messagecomposer():\n      A.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
        msg->headers().ContentType().FromString( ourFineBodyPart.originalContentTypeStr() );
        //msg->headers().Assemble();
        //kdDebug(5006) << "\n\n\nMessageComposer::messagecomposer():\n      B.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
        msg->headers().ContentType().Parse();
        //msg->headers().Assemble();
        //kdDebug(5006) << "\n\n\nMessageComposer::messagecomposer():\n      C.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
        kdDebug(5006) << "MessageComposer::encryptMessage() : set top level Content-Type from originalContentTypeStr()" << endl;
      } else {
        msg->headers().ContentType().FromString( ourFineBodyPart.typeStr() + "/" + ourFineBodyPart.subtypeStr() );
        kdDebug(5006) << "MessageComposer::encryptMessage() : set top level Content-Type from typeStr()/subtypeStr()" << endl;
      }
      //msg->headers().Assemble();
      //kdDebug(5006) << "\n\n\nMessageComposer::messagecomposer():\n      D.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
      if ( !ourFineBodyPart.charset().isEmpty() )
        msg->setCharset( ourFineBodyPart.charset() );
      //msg->headers().Assemble();
      //kdDebug(5006) << "\n\n\nMessageComposer::messagecomposer():\n      E.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
      msg->setHeaderField( "Content-Transfer-Encoding",
                           ourFineBodyPart.contentTransferEncodingStr() );
      //msg->headers().Assemble();
      //kdDebug(5006) << "\n\n\nMessageComposer::messagecomposer():\n      F.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
      msg->setHeaderField( "Content-Description",
                           ourFineBodyPart.contentDescription() );
      msg->setHeaderField( "Content-Disposition",
                           ourFineBodyPart.contentDisposition() );

      kdDebug(5006) << "MessageComposer::encryptMessage() : top level headers and body adjusted" << endl;

      // set body content
      // msg->setBody( ourFineBodyPart.body() );
      msg->setMultiPartBody( ourFineBodyPart.body() );
      //kdDebug(5006) << "\n\n\n\n\n\n\nMessageComposer::messagecomposer():\n      99.:\n\n\n\n|||" << msg->asString() << "|||\n\n\n\n\n\n" << endl;
      //msg->headers().Assemble();
      //kdDebug(5006) << "\n\n\nMessageComposer::messagecomposer():\n      Z.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
    }
  }
}

//-----------------------------------------------------------------------------
// This method does not call any crypto ops, so it does not need to be async
bool MessageComposer::processStructuringInfo( const QString bugURL,
                                              uint boundaryLevel,
                                              const QString contentDescClear,
                                              const QCString contentTypeClear,
                                              const QCString contentSubtypeClear,
                                              const QCString contentDispClear,
                                              const QCString contentTEncClear,
                                              const QCString& clearCStr,
                                              const QString /*contentDescCiph*/,
                                              const QByteArray& ciphertext,
                                              const StructuringInfoWrapper& structuring,
                                              KMMessagePart& resultingPart )
{
  bool bOk = true;

  if( structuring.data.makeMimeObject ) {
    QCString mainHeader = "Content-Type: ";
    if( structuring.data.contentTypeMain
        && 0 < strlen( structuring.data.contentTypeMain ) )
      mainHeader += structuring.data.contentTypeMain;
    else {
      if( structuring.data.makeMultiMime )
        mainHeader += "text/plain";
      else
        mainHeader += contentTypeClear + '/' + contentSubtypeClear;
    }

    QCString boundaryCStr;  // storing boundary string data
    // add "boundary" parameter
    if( structuring.data.makeMultiMime ) {
      // calculate boundary value
      DwMediaType tmpCT;
      tmpCT.CreateBoundary( boundaryLevel );
      boundaryCStr = tmpCT.Boundary().c_str();
      // remove old "boundary" parameter
      int boundA = mainHeader.find( "boundary=", 0, false );
      int boundZ;
      if( -1 < boundA ) {
        // take into account a leading ";  " string
        while( 0 < boundA && ' ' == mainHeader[ boundA-1 ] )
          --boundA;
        if( 0 < boundA && ';' == mainHeader[ boundA-1 ] )
          --boundA;
        boundZ = mainHeader.find( ';', boundA+1 );
        if( -1 == boundZ )
          mainHeader.truncate( boundA );
        else
          mainHeader.remove( boundA, (1 + boundZ - boundA) );
      }
      // insert new "boundary" parameter
      QCString bStr( "; boundary=\"" );
      bStr += boundaryCStr;
      bStr += "\"";
      mainHeader += bStr;
    }

    if( structuring.data.contentTypeMain
        && 0 < strlen( structuring.data.contentTypeMain ) ) {
      if( structuring.data.contentDispMain
          && 0 < strlen( structuring.data.contentDispMain ) ) {
        mainHeader += "\nContent-Disposition: ";
        mainHeader += structuring.data.contentDispMain;
      }
      if( structuring.data.contentTEncMain
          && 0 < strlen( structuring.data.contentTEncMain ) ) {
        mainHeader += "\nContent-Transfer-Encoding: ";
        mainHeader += structuring.data.contentTEncMain;
      }
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

    DwString mainDwStr;
    mainDwStr = mainHeader + "\n\n";
    DwBodyPart mainDwPa( mainDwStr, 0 );
    mainDwPa.Parse();
    KMMessage::bodyPart( &mainDwPa, &resultingPart );
    if( ! structuring.data.makeMultiMime ) {
      if( structuring.data.includeCleartext ) {
        QCString bodyText( clearCStr );
        bodyText += '\n';
        bodyText += ciphertext;
        resultingPart.setBodyEncoded( bodyText );
      } else
        resultingPart.setBodyEncodedBinary( ciphertext );
    } else { //  OF  if( ! structuring.data.makeMultiMime )
      // Build the encapsulated MIME parts.
      // Build a MIME part holding the version information
      // taking the body contents returned in
      // structuring.data.bodyTextVersion.
      QCString versCStr, codeCStr;
      if( structuring.data.contentTypeVersion
          && 0 < strlen( structuring.data.contentTypeVersion ) ) {
        DwString versStr( "Content-Type: " );
        versStr += structuring.data.contentTypeVersion;
        versStr += "\nContent-Description: version code";

        if( structuring.data.contentDispVersion
            && 0 < strlen( structuring.data.contentDispVersion ) ) {
          versStr += "\nContent-Disposition: ";
          versStr += structuring.data.contentDispVersion;
        }
        if( structuring.data.contentTEncVersion
            && 0 < strlen( structuring.data.contentTEncVersion ) ) {
          versStr += "\nContent-Transfer-Encoding: ";
          versStr += structuring.data.contentTEncVersion;
        }

        DwBodyPart versDwPa( versStr + "\n\n", 0 );
        versDwPa.Parse();
        KMMessagePart versKmPa;
        KMMessage::bodyPart(&versDwPa, &versKmPa);
        versKmPa.setBodyEncoded( structuring.data.bodyTextVersion );
        // store string representation of the cleartext headers
        versCStr = versDwPa.Headers().AsString().c_str();
        // store string representation of encoded cleartext
        versCStr += "\n";
        versCStr += versKmPa.body();
      }

      // Build a MIME part holding the code information
      // taking the body contents returned in ciphertext.
      if( structuring.data.contentTypeCode
          && 0 < strlen( structuring.data.contentTypeCode ) ) {
        DwString codeStr( "Content-Type: " );
        codeStr += structuring.data.contentTypeCode;
        if( structuring.data.contentTEncCode
            && 0 < strlen( structuring.data.contentTEncCode ) ) {
	  codeStr += "\nContent-Transfer-Encoding: ";
          codeStr += structuring.data.contentTEncCode;
	//} else {
        //  codeStr += "\nContent-Transfer-Encoding: ";
	//  codeStr += "base64";
	}
        if( structuring.data.contentDispCode
            && 0 < strlen( structuring.data.contentDispCode ) ) {
          codeStr += "\nContent-Disposition: ";
          codeStr += structuring.data.contentDispCode;
        }

        DwBodyPart codeDwPa( codeStr + "\n\n", 0 );
        codeDwPa.Parse();
        KMMessagePart codeKmPa;
        KMMessage::bodyPart(&codeDwPa, &codeKmPa);
        //if(    structuring.data.contentTEncCode
        //    && 0 < strlen( structuring.data.contentTEncCode ) ) {
        //  codeKmPa.setCteStr( structuring.data.contentTEncCode );
        //} else {
        //  codeKmPa.setCteStr("base64");
        //}
        codeKmPa.setBodyEncodedBinary( ciphertext );
        // store string representation of the cleartext headers
        codeCStr = codeDwPa.Headers().AsString().c_str();
        // store string representation of encoded cleartext
        codeCStr += "\n";
        codeCStr += codeKmPa.body();
      } else {
        // Plugin error!
        KMessageBox::sorry( mComposeWin,
                            i18n("<qt><p>Error: The Crypto Plug-in '%1' returned<br>"
                                 "       \" structuring.makeMultiMime \"<br>"
                                 "but did <b>not</b> specify a Content-Type header "
                                 "for the ciphertext that was generated.</p>"
                                 "<p>Please report this bug:<br>%2</p></qt>")
                            .arg(mSelectedCryptPlug->libName())
                            .arg(bugURL) );
        bOk = false;
      }

      QCString mainStr = "--" + boundaryCStr;
      if( structuring.data.includeCleartext && (0 < clearCStr.length()) )
        mainStr += "\n" + clearCStr + "\n--" + boundaryCStr;
      if( 0 < versCStr.length() )
        mainStr += "\n" + versCStr + "\n\n--" + boundaryCStr;
      if( 0 < codeCStr.length() )
        mainStr += "\n" + codeCStr + "\n--" + boundaryCStr;
      mainStr += "--\n";

      resultingPart.setBodyEncoded( mainStr );
    } //  OF  if( ! structuring.data.makeMultiMime ) .. else
  } else { //  OF  if( structuring.data.makeMimeObject )
    // Build a plain message body
    // based on the values returned in structInf.
    // Note: We do _not_ insert line breaks between the parts since
    //       it is the plugin job to provide us with ready-to-use
    //       texts containing all necessary line breaks.
    resultingPart.setContentDescription( contentDescClear );
    resultingPart.setTypeStr( contentTypeClear );
    resultingPart.setSubtypeStr( contentSubtypeClear );
    resultingPart.setContentDisposition( contentDispClear );
    resultingPart.setContentTransferEncodingStr( contentTEncClear );
    QCString resultingBody;

    if( structuring.data.flatTextPrefix
        && strlen( structuring.data.flatTextPrefix ) )
      resultingBody += structuring.data.flatTextPrefix;
    if( structuring.data.includeCleartext ) {
      if( !clearCStr.isEmpty() )
        resultingBody += clearCStr;
      if( structuring.data.flatTextSeparator
          && strlen( structuring.data.flatTextSeparator ) )
        resultingBody += structuring.data.flatTextSeparator;
    }
    if( ciphertext && strlen( ciphertext ) )
      resultingBody += *ciphertext;
    else {
      // Plugin error!
      KMessageBox::sorry( mComposeWin,
                          i18n( "<qt><p>Error: The Crypto Plug-in '%1' did not return "
                                "any encoded data.</p>"
                                "<p>Please report this bug:<br>%2</p></qt>" )
                          .arg( mSelectedCryptPlug->libName() )
                          .arg( bugURL ) );
      bOk = false;
    }
    if( structuring.data.flatTextPostfix
        && strlen( structuring.data.flatTextPostfix ) )
      resultingBody += structuring.data.flatTextPostfix;

    resultingPart.setBodyEncoded( resultingBody );
  } //  OF  if( structuring.data.makeMimeObject ) .. else

  // No need to free the memory that was allocated for the ciphertext
  // since this memory is freed by it's QCString destructor.

  // Neither do we free the memory that was allocated
  // for our structuring info data's char* members since we are using
  // not the pure cryptplug's StructuringInfo struct
  // but the convenient CryptPlugWrapper's StructuringInfoWrapper class.

  return bOk;
}

//-----------------------------------------------------------------------------
QCString MessageComposer::breakLinesAndApplyCodec()
{
  QString text;
  QCString cText;

  if( mDisableBreaking )
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
                                               i18n("Some characters will be lost"),
                                               i18n("Lose Characters"), i18n("Change Encoding") ) == KMessageBox::Yes );
    if( !anyway ) {
      mComposeWin->mEditor->setText(oldText);
      return QCString();
    }
  }

  return cText;
}


//-----------------------------------------------------------------------------
void MessageComposer::pgpSignedMsg( QCString cText,
                                    StructuringInfoWrapper& structuring )
{
  if( !mSelectedCryptPlug ) {
    KMessageBox::sorry( mComposeWin,
                        i18n( "<qt>No active Crypto Plug-In could be found.<br><br>"
                              "Please activate a Plug-In in the configuration dialog.</qt>" ) );
    mRc = false;
    return;
  }

  QByteArray signature;

  kdDebug(5006) << "\nMessageComposer::pgpSignedMsg calling CRYPTPLUG "
                << mSelectedCryptPlug->libName() << endl;

  bool bSign = true;

  if( mSignCertFingerprint.isEmpty() ) {
    // find out whether we are dealing with the OpenPGP or the S/MIME plugin
    if( mSelectedCryptPlug->protocol() == "openpgp" ) {
      // We are dealing with the OpenPGP plugin. Use Kpgp to determine
      // the signing key.
      if( !mPgpIdentity.isEmpty() ) {
        Kpgp::Module *pgp = Kpgp::Module::getKpgp();

        // TODO: ASync call?

        Kpgp::Key* key = pgp->publicKey( mPgpIdentity );
        if( key ) {

          // TODO: ASync call?

          mSignCertFingerprint = key->primaryFingerprint();
          // kdDebug(5006) << "Signer: " << mComposeWin->from()
          //               << "\nFingerprint of signature key: "
          //               << QString( mSignCertFingerprint ) << endl;
        } else {
          KMessageBox::sorry( mComposeWin,
                              i18n("<qt>This message could not be signed "
                                   "because the OpenPGP key which should be "
                                   "used for signing messages with this "
                                   "identity couldn't be found in your "
                                   "keyring.<br><br>"
                                   "You can change the OpenPGP key "
                                   "which should be used with the current "
                                   "identity in the identity configuration.</qt>"),
                              i18n("Missing Signing Key") );
          bSign = false;
        }
      } else {
        KMessageBox::sorry( mComposeWin,
                            i18n("<qt>This message could not be signed "
                                 "because you didn't define the OpenPGP "
                                 "key which should be used for signing "
                                 "messages with this identity.<br><br>"
                                 "You can define the OpenPGP key "
                                 "which should be used with the current "
                                 "identity in the identity configuration.</qt>"),
                            i18n("Undefined Signing Key") );
        bSign = false;
      }
    } else { // S/MIME

      // TODO (Bo): Why doesn't this use the getEncryptionCertificate
      // method below?

      int certSize = 0;
      QByteArray certificate;
      QString selectedCert;
      KListBoxDialog dialog( selectedCert, "",
                             i18n( "&Select certificate:") );
      dialog.resize( 700, 200 );

      QCString signer = mComposeWin->from().utf8();
      signer.replace('\x001', ' ');

      kdDebug(5006) << "\n\nRetrieving keys for: " << mComposeWin->from()
                    << endl;
      char* certificatePtr = 0;

      // TODO: ASync call?

      bool findCertsOk =
        mSelectedCryptPlug->findCertificates( &(*signer), &certificatePtr,
                                              &certSize, true )
        && (0 < certSize);
      kdDebug(5006) << "keys retrieved ok: " << findCertsOk << endl;

      bool useDialog = false;
      if( findCertsOk ) {
        kdDebug(5006) << "findCertificates() returned " << certificatePtr
                      << endl;
        certificate.assign( certificatePtr, certSize );

        // fill selection dialog listbox
        dialog.entriesLB->clear();
        int iA = 0, iZ = 0;
        while( iZ < certSize ) {
          if( (certificate[iZ] == '\1') || (certificate[iZ] == '\0') ) {
            char c = certificate[iZ];
            if( (c == '\1') && !useDialog ) {
              // set up selection dialog
              useDialog = true;
              dialog.setCaption( i18n("Select Certificate [%1]")
                                 .arg( mComposeWin->from() ) );
            }
            certificate[iZ] = '\0';
            QString s = QString::fromUtf8( &certificate[iA] );
            certificate[iZ] = c;
            if( useDialog )
              dialog.entriesLB->insertItem( s );
            else
              selectedCert = s;
            ++iZ;
            iA = iZ;
          }
          ++iZ;
        }

        // run selection dialog and retrieve user choice
        // OR take the single entry (if only one was found)
        if( useDialog ) {
          dialog.entriesLB->setFocus();
          dialog.entriesLB->setSelected( 0, true );
          bSign = (dialog.exec() == QDialog::Accepted);
        }

        if (bSign) {
          mSignCertFingerprint = selectedCert.utf8();
          mSignCertFingerprint.remove( 0, mSignCertFingerprint.findRev( '(' )+1 );
          mSignCertFingerprint.truncate( mSignCertFingerprint.length()-1 );
          kdDebug(5006) << "Signer: " << mComposeWin->from()
                        << "\nFingerprint of signature key: "
                        << QString( mSignCertFingerprint ) << "\n\n\n";
          if( mSignCertFingerprint.isEmpty() )
            bSign = false;
        }
      }
    }

    // Check for expiry of the signer, CA, and Root certificate.
    // Only do the expiry check if the plugin has this feature
    // and if there in *no* fingerprint in mSignCertFingerprint already.
    if( mSelectedCryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) ){

      // TODO: ASync call?

      int sigDaysLeft = mSelectedCryptPlug->signatureCertificateDaysLeftToExpiry( mSignCertFingerprint );

      // TODO: ASync calls?

      if( mSelectedCryptPlug->signatureCertificateExpiryNearWarning() &&
          sigDaysLeft <
          mSelectedCryptPlug->signatureCertificateExpiryNearInterval() ) {
        QString txt1;
        if( 0 < sigDaysLeft )
          txt1 = i18n( "The certificate you want to use for signing expires in %1 days.<br>This means that after this period, the recipients will not be able to check your signature any longer." ).arg( sigDaysLeft );
        else if( 0 > sigDaysLeft )
          txt1 = i18n( "The certificate you want to use for signing expired %1 days ago.<br>This means that the recipients will not be able to check your signature." ).arg( -sigDaysLeft );
        else
          txt1 = i18n( "The certificate you want to use for signing expires today.<br>This means that, starting from tomorrow, the recipients will not be able to check your signature any longer." );
        int ret = KMessageBox::warningYesNo( mComposeWin,
                                             i18n( "<qt><p>%1</p>"
                                                   "<p>Do you still want to use this "
                                                   "certificate?</p></qt>" )
                                             .arg( txt1 ),
                                             i18n( "Certificate Warning" ),
                                             KGuiItem( i18n("&Use Certificate") ),
                                             KGuiItem( i18n("&Don't Use Certificate") ) );
        if( ret == KMessageBox::No )
          bSign = false;
      }

      if( bSign && ( 0 <= mSelectedCryptPlug->libName().find( "smime", 0, false ) ) ) {

        // TODO: ASync call?

        int rootDaysLeft = mSelectedCryptPlug->rootCertificateDaysLeftToExpiry( mSignCertFingerprint );

        // TODO: ASync calls?

        if( mSelectedCryptPlug->rootCertificateExpiryNearWarning() &&
            rootDaysLeft <
            mSelectedCryptPlug->rootCertificateExpiryNearInterval() ) {
          QString txt1;
          if( 0 < rootDaysLeft )
            txt1 = i18n( "The root certificate of the certificate you want to use for signing expires in %1 days.<br>This means that after this period, the recipients will not be able to check your signature any longer." ).arg( rootDaysLeft );
          else if( 0 > rootDaysLeft )
            txt1 = i18n( "The root certificate of the certificate you want to use for signing expired %1 days ago.<br>This means that the recipients will not be able to check your signature." ).arg( -rootDaysLeft );
          else
            txt1 = i18n( "The root certificate of the certificate you want to use for signing expires today.<br>This means that beginning from tomorrow, the recipients will not be able to check your signature any longer." );
          int ret = KMessageBox::warningYesNo( mComposeWin,
                                               i18n( "<qt><p>%1</p>"
                                                     "<p>Do you still want to use this "
                                                     "certificate?</p></qt>" )
                                               .arg( txt1 ),
                                               i18n( "Certificate Warning" ),
                                               KGuiItem( i18n("&Use Certificate") ),
                                               KGuiItem( i18n("&Don't Use Certificate") ) );
          if( ret == KMessageBox::No )
            bSign = false;
        }
      }

      if( bSign && ( 0 <= mSelectedCryptPlug->libName().find( "smime", 0, false ) ) ) {

        // TODO: ASync call?

        int caDaysLeft = mSelectedCryptPlug->caCertificateDaysLeftToExpiry( mSignCertFingerprint );

        // TODO: ASync calls?

        if( mSelectedCryptPlug->caCertificateExpiryNearWarning() &&
            caDaysLeft <
            mSelectedCryptPlug->caCertificateExpiryNearInterval() ) {
          QString txt1;
          if( 0 < caDaysLeft )
            txt1 = i18n( "The CA certificate of the certificate you want to use for signing expires in %1 days.<br>This means that after this period, the recipients will not be able to check your signature any longer." ).arg( caDaysLeft );
          else if( 0 > caDaysLeft )
            txt1 = i18n( "The CA certificate of the certificate you want to use for signing expired %1 days ago.<br>This means that the recipients will not be able to check your signature." ).arg( -caDaysLeft );
          else
            txt1 = i18n( "The CA certificate of the certificate you want to use for signing expires today.<br>This means that beginning from tomorrow, the recipients will not be able to check your signature any longer." );
          int ret = KMessageBox::warningYesNo( mComposeWin,
                                               i18n( "<qt><p>%1</p>"
                                                     "<p>Do you still want to use this "
                                                     "certificate?</p></qt>" )
                                               .arg( txt1 ),
                                               i18n( "Certificate Warning" ),
                                               KGuiItem( i18n("&Use Certificate") ),
                                               KGuiItem( i18n("&Don't Use Certificate") ) );
          if( ret == KMessageBox::No )
            bSign = false;
        }
      }
    }
    // Check whether the sender address of the signer is contained in
    // the certificate, but only do this if the plugin has this feature.
    if( mSelectedCryptPlug->hasFeature( Feature_WarnSignEmailNotInCertificate ) ) {

      // TODO: ASync calls?

      if( bSign && mSelectedCryptPlug->warnNoCertificate() &&
          !mSelectedCryptPlug->isEmailInCertificate( QString( KMMessage::getEmailAddr( mComposeWin->from() ) ).utf8(), mSignCertFingerprint ) )  {
        QString txt1 = i18n( "The certificate you want to use for signing does not contain your sender email address.<br>This means that it is not possible for the recipients to check whether the email really came from you." );
        int ret = KMessageBox::warningYesNo( mComposeWin,
                                             i18n( "<qt><p>%1</p>"
                                                   "<p>Do you still want to use this "
                                                   "certificate?</p></qt>" )
                                             .arg( txt1 ),
                                             i18n( "Certificate Warning" ),
                                             KGuiItem( i18n("&Use Certificate") ),
                                             KGuiItem( i18n("&Don't Use Certificate") ) );
        if( ret == KMessageBox::No )
          bSign = false;
      }
    }
  } // if( mSignCertFingerprint.isEmpty() )

  // Finally sign the message, but only if the plugin has this feature.
  // TODO (Bo): If this isn't the case, why go through this method at all?
  //   Why not just exit at the beginning?
  if( mSelectedCryptPlug->hasFeature( Feature_SignMessages ) ) {
    size_t cipherLen;

    const char* cleartext = cText;
    char* ciphertext  = 0;

    if( mDebugComposerCrypto ) {
      QFile fileS( "dat_11_sign.input" );
      if( fileS.open( IO_WriteOnly ) ) {
        QDataStream ds( &fileS );
        ds.writeRawBytes( cleartext, strlen( cleartext ) );
        fileS.close();
      }
    }

    if( bSign ) {
      int errId = 0;
      char* errTxt = 0;

      // TODO: ASync call? Likely, yes :-)

      if( mSelectedCryptPlug->signMessage( cleartext,
                                           &ciphertext, &cipherLen,
                                           mSignCertFingerprint,
                                           structuring, &errId, &errTxt ) ) {
        if( mDebugComposerCrypto ) {
          QFile fileD( "dat_12_sign.output" );
          if( fileD.open( IO_WriteOnly ) ) {
            QDataStream ds( &fileD );
            ds.writeRawBytes( ciphertext, cipherLen );
            fileD.close();
          }
        }
        signature.assign( ciphertext, cipherLen );
        kdDebug(5006) << "Assigned signature\n";
      } else if ( errId == /*GPGME_Canceled*/20 ) {
        mRc = false;
        return;
      } else {
        QString error = "#" + QString::number( errId ) + "  :  ";
        if( errTxt )
          error += errTxt;
        else
          error += i18n("[unknown error]");
        KMessageBox::sorry( mComposeWin,
                            i18n( "<qt><p><b>This message could not be signed!</b></p>"
                                  "<p>The Crypto Plug-In '%1' reported the following "
                                  "details:</p>"
                                  "<p><i>%2</i></p>"
                                  "<p>Your configuration might be invalid or the Plug-In "
                                  "damaged.</p>"
                                  "<p><b>Please contact your system "
                                  "administrator.</b></p></qt>" )
                            .arg( mSelectedCryptPlug->libName() )
                            .arg( error ) );
      }
      // we do NOT call a "delete ciphertext" !
      // since "signature" will take care for it (is a QByteArray)
      delete errTxt;
      errTxt = 0;
    }
  }

  // PENDING(khz,kalle) Warn if there was no signature? (because of
  // a problem or because the plugin does not allow signing?

  /* ----------------------------- */
  kdDebug(5006) << "\nMessageComposer::pgpSignedMsg returning from CRYPTPLUG.\n" << endl;

  mSignature = signature;
}

//-----------------------------------------------------------------------------
Kpgp::Result MessageComposer::pgpEncryptedMsg( QByteArray & encryptedBody,
                                               QCString cText,
                                               StructuringInfoWrapper& structuring,
                                               QCString& encryptCertFingerprints )
{
  if( !mSelectedCryptPlug ) {
    KMessageBox::sorry( mComposeWin,
                        i18n( "<qt>No active Crypto Plug-In could be found.<br><br>"
                              "Please activate a Plug-In in the configuration dialog.</qt>" ) );

    // TODO (Bo): Sure about this? (Will stop all further jobs)
    mRc = false;

    // TODO (Bo): The method always returns Ok, so better do it here also.
    // TODO (Bo): Figure out if this should really be the case
    return Kpgp::Ok;
  }

  Kpgp::Result result = Kpgp::Ok;

  // we call the cryptplug
  kdDebug(5006) << "\nMessageComposer::pgpEncryptedMsg: going to call CRYPTPLUG "
                << mSelectedCryptPlug->libName() << endl;

  // PENDING(khz,kalle) Warn if no encryption?

  const char* cleartext  = cText;
  const char* ciphertext = 0;

  // Actually do the encryption, if the plugin supports this
  size_t cipherLen;

  int errId = 0;
  char* errTxt = 0;
  if( mSelectedCryptPlug->hasFeature( Feature_EncryptMessages ) &&

      // TODO: ASync call? Likely, yes :-)

      mSelectedCryptPlug->encryptMessage( cleartext,
                                          &ciphertext, &cipherLen,
                                          encryptCertFingerprints,
                                          structuring, &errId, &errTxt )
      && ciphertext )
    encryptedBody.assign( ciphertext, cipherLen );
  else {
    QString error = "#" + QString::number( errId ) + "  :  ";
    if( errTxt )
      error += errTxt;
    else
      error += i18n("[unknown error]");
    KMessageBox::sorry(mComposeWin,
                       i18n("<qt><p><b>This message could not be encrypted!</b></p>"
                            "<p>The Crypto Plug-In '%1' reported the following "
                            "details:</p>"
                            "<p><i>%2</i></p>"
                            "<p>Your configuration might be invalid or the Plug-In "
                            "damaged.</p>"
                            "<p><b>Please contact your system "
                            "administrator.</b></p></qt>")
                       .arg(mSelectedCryptPlug->libName())
                       .arg( error ) );
  }
  delete errTxt;
  errTxt = 0;

  // we do NOT delete the "ciphertext" !
  // bacause "encoding" will take care for it (is a QByteArray)
  kdDebug(5006) << "\nMessageComposer::pgpEncryptedMsg: returning from CRYPTPLUG.\n" << endl;

  // TODO (Bo): Why does this always return success?
  return result;
}

//-----------------------------------------------------------------------------
QCString MessageComposer::getEncryptionCertificate( const QString& recipient )
{
  bool bEncrypt = true;

  QCString addressee = recipient.utf8();
  addressee.replace('\x001', ' ');
  kdDebug(5006) << "\n\n1st try:  Retrieving keys for: " << recipient << endl;

  QString selectedCert;
  KListBoxDialog dialog( selectedCert, "", i18n( "&Select certificate:" ) );
  dialog.resize( 700, 200 );
  bool useDialog;
  int certSize = 0;
  QByteArray certificateList;

  bool askForDifferentSearchString = false;
  do {
    certSize = 0;
    char* certificatePtr = 0;
    bool findCertsOk;
    if( askForDifferentSearchString )
      findCertsOk = false;
    else {

      // TODO: ASync call?

      findCertsOk = mSelectedCryptPlug->findCertificates( &(*addressee),
                                                          &certificatePtr,
                                                          &certSize,
                                                          false )
        && (0 < certSize);
      kdDebug(5006) << "keys retrieved successfully: " << findCertsOk << "\n" << endl;
      kdDebug(5006) << "findCertificates() 1st try returned " << certificatePtr << endl;
      if( findCertsOk )
        certificateList.assign( certificatePtr, certSize );
    }
    while( !findCertsOk ) {
      bool bOk = false;
      addressee = KInputDialog::getText( askForDifferentSearchString
                                         ? i18n("Look for Other Certificates")
                                         : i18n("No Certificate Found"),
                                         i18n("Enter different address for recipient %1 "
                                              "or enter \" * \" to see all certificates:")
                                         .arg(recipient),
                                         addressee, &bOk, mComposeWin )
        .stripWhiteSpace().utf8();
      askForDifferentSearchString = false;
      if( bOk ) {
        addressee = addressee.simplifyWhiteSpace();
        if( ("\"*\"" == addressee) ||
            ("\" *\"" == addressee) ||
            ("\"* \"" == addressee) ||
            ("\" * \"" == addressee))  // You never know what users type.  :-)
          addressee = "*";
        kdDebug(5006) << "\n\nnext try: Retrieving keys for: " << addressee << endl;
        certSize = 0;
        char* certificatePtr = 0;

        // TODO: ASync call?

        findCertsOk = mSelectedCryptPlug->findCertificates( &(*addressee),
                                                            &certificatePtr,
                                                            &certSize,
                                                            false )
          && (0 < certSize);
        kdDebug(5006) << "keys retrieved successfully: " << findCertsOk << "\n" << endl;
        kdDebug(5006) << "findCertificates() 2nd try returned " << certificatePtr << endl;
        if( findCertsOk )
          certificateList.assign( certificatePtr, certSize );
      } else {
        bEncrypt = false;
        break;
      }
    }
    if( bEncrypt && findCertsOk ) {
      // fill selection dialog listbox
      dialog.entriesLB->clear();
      // show dialog even if only one entry to allow specifying of
      // another search string _instead_of_ the recipients address
      bool bAlwaysShowDialog = true;

      useDialog = false;
      int iA = 0, iZ = 0;
      while( iZ < certSize ) {
        if( (certificateList.at(iZ) == '\1') || (certificateList.at(iZ) == '\0') ) {
          kdDebug(5006) << "iA=" << iA << " iZ=" << iZ << endl;
          char c = certificateList.at(iZ);
          if( (bAlwaysShowDialog || (c == '\1')) && !useDialog ) {
            // set up selection dialog
            useDialog = true;
            dialog.setCaption( i18n( "Select Certificate for Encryption [%1]" )
                              .arg( recipient ) );
            dialog.setLabelAbove(
              i18n( "&Select certificate for recipient %1:" )
              .arg( recipient ) );
          }
          certificateList.at(iZ) = '\0';
          QString s = QString::fromUtf8( &certificateList.at(iA) );
          certificateList.at(iZ) = c;
          if( useDialog )
            dialog.entriesLB->insertItem( s );
          else
            selectedCert = s;
          ++iZ;
          iA = iZ;
        }
        ++iZ;
      }
      // run selection dialog and retrieve user choice
      // OR take the single entry (if only one was found)
      if( useDialog ) {
        dialog.setCommentBelow(
          i18n("(Certificates matching address \"%1\", "
               "press [Cancel] to use different address for recipient %2.)")
          .arg( addressee )
          .arg( recipient ) );
        dialog.entriesLB->setFocus();
        dialog.entriesLB->setSelected( 0, true );
        askForDifferentSearchString = (dialog.exec() != QDialog::Accepted);
      }
    }
  } while ( askForDifferentSearchString );

  if( bEncrypt )
    return selectedCert.utf8();
  else
    return QCString();
}

// TODO: Does this have any crypto stuff that needs to be async?
bool MessageComposer::checkForEncryptCertificateExpiry( const QString& recipient,
                                                        const QCString& certFingerprint )
{
  bool bEncrypt = true;

  // Check for expiry of various certificates, but only if the
  // plugin supports this.
  if( mSelectedCryptPlug->hasFeature( Feature_WarnEncryptCertificateExpiry ) ) {
    QString captionWarn = i18n( "Certificate Warning [%1]" ).arg( recipient );


    // TODO: ASync call?

    int encRecvDaysLeft =
      mSelectedCryptPlug->receiverCertificateDaysLeftToExpiry( certFingerprint );
    if( mSelectedCryptPlug->receiverCertificateExpiryNearWarning() &&
        encRecvDaysLeft <
        mSelectedCryptPlug->receiverCertificateExpiryNearWarningInterval() ) {
      QString txt1;
      if( 0 < encRecvDaysLeft )
        txt1 = i18n( "The certificate of the recipient you want to send this "
                     "message to expires in %1 days.<br>This means that after "
                     "this period, the recipient will not be able to read "
                     "your message any longer." )
               .arg( encRecvDaysLeft );
      else if( 0 > encRecvDaysLeft )
        txt1 = i18n( "The certificate of the recipient you want to send this "
                     "message to expired %1 days ago.<br>This means that the "
                     "recipient will not be able to read your message." )
               .arg( -encRecvDaysLeft );
      else
        txt1 = i18n( "The certificate of the recipient you want to send this "
                     "message to expires today.<br>This means that beginning "
                     "from tomorrow, the recipient will not be able to read "
                     "your message any longer." );
      int ret = KMessageBox::warningYesNo( mComposeWin,
                                           i18n( "<qt><p>%1</p>"
                                                 "<p>Do you still want to use "
                                                 "this certificate?</p></qt>" )
                                           .arg( txt1 ),
                                           captionWarn,
                                           KGuiItem( i18n("&Use Certificate") ),
                                           KGuiItem( i18n("&Don't Use Certificate") ) );
      if( ret == KMessageBox::No )
        bEncrypt = false;
    }

    if( bEncrypt ) {
      int certInChainDaysLeft =
        mSelectedCryptPlug->certificateInChainDaysLeftToExpiry( certFingerprint );
      if( mSelectedCryptPlug->certificateInChainExpiryNearWarning() &&
          certInChainDaysLeft <
          mSelectedCryptPlug->certificateInChainExpiryNearWarningInterval() ) {
        QString txt1;
        if( 0 < certInChainDaysLeft )
          txt1 = i18n( "One of the certificates in the chain of the "
                       "certificate of the recipient you want to send this "
                       "message to expires in %1 days.<br>"
                       "This means that after this period, the recipient "
                       "might not be able to read your message any longer." )
                 .arg( certInChainDaysLeft );
        else if( 0 > certInChainDaysLeft )
          txt1 = i18n( "One of the certificates in the chain of the "
                       "certificate of the recipient you want to send this "
                       "message to expired %1 days ago.<br>"
                       "This means that the recipient might not be able to "
                       "read your message." )
                 .arg( -certInChainDaysLeft );
        else
          txt1 = i18n( "One of the certificates in the chain of the "
                       "certificate of the recipient you want to send this "
                       "message to expires today.<br>This means that "
                       "beginning from tomorrow, the recipient might not be "
                       "able to read your message any longer." );
        int ret = KMessageBox::warningYesNo( mComposeWin,
                                             i18n( "<qt><p>%1</p>"
                                                   "<p>Do you still want to use this "
                                                   "certificate?</p></qt>" )
                                             .arg( txt1 ),
                                             captionWarn,
                                             KGuiItem( i18n("&Use Certificate") ),
                                             KGuiItem( i18n("&Don't Use Certificate") ) );
        if( ret == KMessageBox::No )
          bEncrypt = false;
      }
    }
  }

  return bEncrypt;
}


#include "messagecomposer.moc"
