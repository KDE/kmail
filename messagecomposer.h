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

#ifndef MESSAGECOMPOSER_H
#define MESSAGECOMPOSER_H

#include "kmmsgpart.h"

#include <qobject.h>

#include <mimelib/mediatyp.h>
#include <kpgp.h>

class KMMessage;
class KMComposeWin;
class CryptPlugWrapper;
class StructuringInfoWrapper;

class MessageComposerJob;


class MessageComposer : public QObject {
  Q_OBJECT
  friend class MessageComposerJob;
  friend class EncryptMessageJob;
  friend class SignSMimeJob;

public:
  MessageComposer( KMComposeWin* win, const char* name=0 );
  ~MessageComposer();

  /**
   * Applies the user changes to the message object of the composer
   * and signs/encrypts the message if activated. Returns FALSE in
   * case of an error (e.g. if PGP encryption fails).
   * If backgroundMode is true then no functions which might require
   * user interaction (like signing/encrypting) are performed
   */
  void applyChanges( bool dontSign, bool dontEncrypt );

  QString originalBCC() const { return mBcc; }

  void setDisableBreaking( bool b ) { mDisableBreaking = b; }

signals:
  void done( bool );

private:
  void readFromComposeWin();

  void adjustCryptFlags();

  // This is the composeMessage method
  void composeMessage();
  // And these two are the parts that should be run after job completions
  void composeMessage2();
  void composeMessage3();

  /**
   * Internal helper function called from applyChanges(void) to allow
   * processing several messages (encrypted or unencrypted) based on
   * the same composer content.
   * That's useful for storing decrypted versions of messages which
   * were sent in encrypted form.                  (khz, 2002/06/24)
   */
  void composeMessage( KMMessage& theMessage,
                       bool doSign, bool doEncrypt, bool ignoreBcc );
  void continueComposeMessage( KMMessage& theMessage, bool doSign,
                               bool doEncrypt, bool ignoreBcc );

  /**
   * Get message ready for sending or saving.
   * This must be done _before_ signing and/or encrypting it.
   */
  QCString breakLinesAndApplyCodec();

  /**
   * Get signature for a message.
   * To build nice S/MIME objects signing and encoding must be separeted.
   */
  void pgpSignedMsg( QCString cText, StructuringInfoWrapper& structuring );
  /**
   * Get encrypted message.
   * To build nice S/MIME objects signing and encrypting must be separat.
   */
  Kpgp::Result pgpEncryptedMsg( QByteArray& rEncryptedBody,
                                QCString cText,
                                StructuringInfoWrapper& structuring,
                                QCString& encryptCertFingerprints );

  /**
   * Get encryption certificate for a recipient (the Aegypten way).
   */
  QCString getEncryptionCertificate( const QString& recipient );

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
   *       flat text, resp.) is returned in resultingData, so just
   *       use this string as body text of the surrounding MIME object.
   *       This string *is* encoded according to contentTEncClear
   *       and thus should be ready for being sended via SMTP.
   */
  bool processStructuringInfo( const QString bugURL, uint boundaryLevel,
                               const QString contentDescriptionClear,
                               const QCString contentTypeClear,
                               const QCString contentSubtypeClear,
                               const QCString contentDispClear,
                               const QCString contentTEncClear,
                               const QCString& bodytext,
                               const QString contentDescriptionCiph,
                               const QByteArray& ciphertext,
                               const StructuringInfoWrapper& structuring,
                               KMMessagePart& resultingPart );

  Kpgp::Result getEncryptionCertificates( const QStringList& recipients,
                                          QCString& encryptionCertificates );

  void encryptMessage( KMMessage* msg, const QStringList& recipients,
                       bool doSign, bool doEncrypt,
                       int previousBoundaryLevel, KMMessagePart newBodyPart );

private slots:
  void slotDoNextJob();

private:
  void doNextJob();

  void handleBCCMessage( KMMessage* );

  KMComposeWin* mComposeWin;
  MessageComposerJob * mCurrentJob;
  KMMessage* mMsg;
  KMMessage* mExtraMessage;
  CryptPlugWrapper* mSelectedCryptPlug;

  QCString mSignCertFingerprint;

  QCString mPgpIdentity;
  bool mDontSign, mDontEncrypt, mAutoPgpEncrypt;
  bool mDisableBreaking;
  QString mBcc;
  bool mDebugComposerCrypto;
  bool mAutoCharset;
  QCString mCharset;
  bool mRc; // Set this to false, if something fails during the processes
  bool mHoldJobs; // Don't run the next job yet

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
