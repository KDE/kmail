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

#ifndef MESSAGECOMPOSER_H
#define MESSAGECOMPOSER_H

#include "kmmsgpart.h"
#include "keyresolver.h"

#include <qobject.h>
#include <qvaluevector.h>

#include <mimelib/mediatyp.h>
#include <kpgp.h>

#include <vector>

class KMMessage;
class KMComposeWin;
class CryptPlugWrapper;

class MessageComposerJob;

namespace Kleo {
  class KeyResolver;
}

namespace GpgME {
  class Key;
}

namespace KPIM {
  class Identity;
}

class MessageComposer : public QObject {
  Q_OBJECT
  friend class MessageComposerJob;
  friend class EncryptMessageJob;
  friend class SetLastMessageAsUnencryptedVersionOfLastButOne;

public:
  class KeyResolver;

  MessageComposer( KMComposeWin* win, const char* name=0 );
  ~MessageComposer();

  /**
   * Applies the user changes to the message object of the composer
   * and signs/encrypts the message if activated. Returns FALSE in
   * case of an error (e.g. if PGP encryption fails).
   * If backgroundMode is true then no functions which might require
   * user interaction (like signing/encrypting) are performed
   */
  void applyChanges( bool disableCrypto );

  QString originalBCC() const { return mBcc; }

  void setDisableBreaking( bool b ) { mDisableBreaking = b; }

  const QValueVector<KMMessage*> & composedMessageList() const {
    return mMessageList;
  }

signals:
  void done( bool );

private:
  void readFromComposeWin();

  void adjustCryptFlags();

  // This is the composeMessage method
  void composeMessage();
  // And these two are the parts that should be run after job completions
  void createUnencryptedMessageVersion();

  /**
   * Internal helper function called from applyChanges(void) to allow
   * processing several messages (encrypted or unencrypted) based on
   * the same composer content.
   * That's useful for storing decrypted versions of messages which
   * were sent in encrypted form.                  (khz, 2002/06/24)
   */
  void composeMessage( KMMessage& theMessage,
                       bool doSign, bool doEncrypt,
		       Kleo::CryptoMessageFormat format );
  void continueComposeMessage( KMMessage& theMessage, bool doSign,
                               bool doEncrypt,
			       Kleo::CryptoMessageFormat format );

  /**
   * Called by composeMessage for inline-openpgp messages
   */
  void composeInlineOpenPGPMessage( KMMessage& theMessage,
                                    bool doSign, bool doEncrypt );

  /**
   * Get message ready for sending or saving.
   * This must be done _before_ signing and/or encrypting it.
   */
  QCString breakLinesAndApplyCodec();
  /// Same as above but ensure \n termination
  QCString bodyText();

  /**
   * Create a plain text version of a marked up mail for use as the plain
   * part in a multipart/alternative mail.
   */
  QCString plainTextFromMarkup( const QString& markupText );
  
  /**
   * Get signature for a message (into mMessage).
   * To build nice S/MIME objects signing and encoding must be separated.
   */
  void pgpSignedMsg( const QCString & cText, Kleo::CryptoMessageFormat f );
  /**
   * Get encrypted message.
   * To build nice S/MIME objects signing and encrypting must be separate.
   */
  Kpgp::Result pgpEncryptedMsg( QByteArray& rEncryptedBody,
                                const QCString & cText,
                                const std::vector<GpgME::Key> & encryptionKeys,
				Kleo::CryptoMessageFormat f );

  /**
   * Get signed & encrypted message.
   * To build nice S/MIME objects signing and encrypting must be separate.
   */
  Kpgp::Result pgpSignedAndEncryptedMsg( QByteArray& rEncryptedBody,
					 const QCString & cText,
					 const std::vector<GpgME::Key> & signingKeys,
					 const std::vector<GpgME::Key> & encryptionKeys,
					 Kleo::CryptoMessageFormat f );

  /**
   * Check for expiry of various certificates.
   */
  bool checkForEncryptCertificateExpiry( const QString& recipient,
                                         const QCString& certFingerprint );

  /**
   * Build a MIME object (or a flat text resp.) based upon
   * structuring information returned by a crypto plugin that was
   * called via pgpSignedMsg() (or pgpEncryptedMsg(), resp.).
   *
   * NOTE: The c string representation of the MIME object (or the
   *       flat text, resp.) is returned in resultingPart, so just
   *       use this string as body text of the surrounding MIME object.
   *       This string *is* encoded according to contentTEncClear
   *       and thus should be ready for being sent via SMTP.
   */
  bool processStructuringInfo( const QString bugURL,
                               const QString contentDescriptionClear,
                               const QCString contentTypeClear,
                               const QCString contentSubtypeClear,
                               const QCString contentDispClear,
                               const QCString contentTEncClear,
                               const QCString& bodytext,
                               const QString contentDescriptionCiph,
                               const QByteArray& ciphertext,
                               KMMessagePart& resultingPart,
			       bool signing, Kleo::CryptoMessageFormat format );

  void encryptMessage( KMMessage* msg, const Kleo::KeyResolver::SplitInfo & si,
                       bool doSign, bool doEncrypt,
                       KMMessagePart newBodyPart,
		       Kleo::CryptoMessageFormat format );

  void addBodyAndAttachments( KMMessage* msg, const Kleo::KeyResolver::SplitInfo & si,
                              bool doSign, bool doEncrypt,
                              const KMMessagePart& ourFineBodyPart,
                              Kleo::CryptoMessageFormat format );

private slots:
  void slotDoNextJob();

private:
  void doNextJob();

  int encryptionPossible( const QStringList & recipients, bool openPGP );
  bool determineWhetherToSign( bool doSignCompletely );
  bool determineWhetherToEncrypt( bool doEncryptCompletely );
  void markAllAttachmentsForSigning( bool sign );
  void markAllAttachmentsForEncryption( bool enc );

  KMComposeWin* mComposeWin;
  MessageComposerJob * mCurrentJob;
  KMMessage* mReferenceMessage;
  QValueVector<KMMessage*> mMessageList;

  Kleo::KeyResolver * mKeyResolver;

  QCString mSignCertFingerprint;

  struct Attachment {
    Attachment( KMMessagePart * p=0, bool s=false, bool e=false )
      : part( p ), sign( s ), encrypt( e ) {}
    KMMessagePart * part;
    bool sign;
    bool encrypt;
  };
  QValueVector<Attachment> mAttachments;

  QString mPGPSigningKey, mSMIMESigningKey;
  bool mUseOpportunisticEncryption;
  bool mSignBody, mEncryptBody;
  bool mSigningRequested, mEncryptionRequested;
  bool mDoSign, mDoEncrypt;
  unsigned int mAllowedCryptoMessageFormats;
  bool mDisableCrypto;
  bool mDisableBreaking;
  QString mBcc;
  QStringList mTo, mCc, mBccList;
  bool mDebugComposerCrypto;
  bool mAutoCharset;
  QCString mCharset;
  bool mIsRichText;
  uint mIdentityUid;
  bool mRc; // Set this to false, if something fails during the processes
  bool mHoldJobs; // Don't run the next job yet

  QCString mText; // textual representation of the message text, encoded
  unsigned int mLineBreakColumn; // used for line breaking

  // These are the variables of the big composeMessage(X,Y,Z) message
  KMMessagePart* mNewBodyPart;
  QByteArray mSignature;

  QCString mEncodedBody; // Only needed if signing and/or encrypting
  bool mEarlyAddAttachments, mAllAttachmentsAreInBody;
  KMMessagePart mOldBodyPart;
  int mPreviousBoundaryLevel;

  // The boundary is saved for later addition into mp/a body
  DwString  mSaveBoundary;

  QValueList<MessageComposerJob*> mJobs;
};

#endif /* MESSAGECOMPOSER_H */
