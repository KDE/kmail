/*
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

#ifndef MESSAGECOMPOSER_H
#define MESSAGECOMPOSER_H

#include "kmmsgpart.h"
#include "keyresolver.h"

#include <QObject>

#include <QList>
#include <QByteArray>

#include <mimelib/mediatyp.h>
#include "kleo/cryptobackend.h"

#include <vector>

class KMMessage;
class KMComposeWin;

class MessageComposerJob;
class EncryptMessageJob;
class SetLastMessageAsUnencryptedVersionOfLastButOne;

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
  friend class ::MessageComposerJob;
  friend class ::EncryptMessageJob;
  friend class ::SetLastMessageAsUnencryptedVersionOfLastButOne;

  public:
    class KeyResolver;

    MessageComposer( KMComposeWin *win );
    ~MessageComposer();

    /*
      Applies the user changes to the message object of the composer
      and signs/encrypts the message if activated. Returns false in
      case of an error (e.g. if PGP encryption fails).
      If backgroundMode is true then no functions which might require
      user interaction (like signing/encrypting) are performed
    */
    void applyChanges( bool disableCrypto );

    QString originalBCC() const { return mBcc; }

    void setDisableBreaking( bool b ) { mDisableBreaking = b; }

    const QVector<KMMessage*> &composedMessageList() const { return mMessageList; }

    bool isPerformingSignOperation() const { return mPerformingSignOperation; }

  signals:
    void done( bool );

  private:
    void readFromComposeWin();

    void adjustCryptFlags();

    bool encryptWithChiasmus( const Kleo::CryptoBackend::Protocol *chiasmus,
                              const QByteArray &body,
                              QByteArray &resultData ) const;
    void chiasmusEncryptAllAttachments();
    void composeChiasmusMessage( KMMessage &theMessage,
                                 Kleo::CryptoMessageFormat format );

    // This is the composeMessage method
    void composeMessage();
    // And these two are the parts that should be run after job completions
    void createUnencryptedMessageVersion();

    /*
      Internal helper function called from applyChanges(void) to allow
      processing several messages (encrypted or unencrypted) based on
      the same composer content.
      That's useful for storing decrypted versions of messages which
      were sent in encrypted form.                  (khz, 2002/06/24)
    */
    void composeMessage( KMMessage &theMessage,
                         bool doSign, bool doEncrypt,
                         Kleo::CryptoMessageFormat format );
    void continueComposeMessage( KMMessage &theMessage, bool doSign,
                                 bool doEncrypt,
                                 Kleo::CryptoMessageFormat format );

    /*
      Called by composeMessage for inline-openpgp messages
    */
    void composeInlineOpenPGPMessage( KMMessage &theMessage,
                                      bool doSign, bool doEncrypt );

    /**
     * Reads the plain text version and the HTML code from the edit widget,
     * applies linebreaks and encodes those with the current charset.
     *
     * @return false if not all chars could be encoded with the current charset.
     */
    bool getSourceText( QByteArray &plainTextEncoded, QByteArray &htmlSourceEncoded ) const;

    /**
     * This sets the member variables mHtmlSource and mPlainText and mBodyText.
     * The text is taken from the composer (with proper wordwrapping) and
     * then encoded with the correct codec.
     * The user is warned if the codec can't encode all characters.
     *
     * @return false if any error occurred, for example if the text of the body
     *         couldn't be encoded.
     */
    bool breakLinesAndApplyCodec();

    /**
     * This will automatically detect the charset needed to encode the text of
     * the composer.
     * mCharset will be set to the detected character set, and the charset of
     * the composer window will be set as well.
     *
     * @return true if the detection was successful, false if no suitable
     *         charset was found.
     */
    bool autoDetectCharset();

    /*
      Gets the signature for a message (into mMessage).
      To build nice S/MIME objects signing and encoding must be separated.
    */
    void pgpSignedMsg( const QByteArray &cText, Kleo::CryptoMessageFormat f );

    /*
      Gets the encrypted message.
      To build nice S/MIME objects signing and encrypting must be separate.
    */
    Kpgp::Result pgpEncryptedMsg( QByteArray &rEncryptedBody,
                                  const QByteArray &cText,
                                  const std::vector<GpgME::Key> &encryptionKeys,
                                  Kleo::CryptoMessageFormat f ) const;

    /*
      Gets the signed & encrypted message.
      To build nice S/MIME objects signing and encrypting must be separate.
    */
    Kpgp::Result pgpSignedAndEncryptedMsg( QByteArray &rEncryptedBody,
                                           const QByteArray &cText,
                                           const std::vector<GpgME::Key> &signingKeys,
                                           const std::vector<GpgME::Key> &encryptionKeys,
                                           Kleo::CryptoMessageFormat f ) const;

    /*
      Builds a MIME object (or a flat text resp.) based upon structuring
      information returned by a crypto plugin that was called via
      pgpSignedMsg() (or pgpEncryptedMsg(), resp.).

      NOTE: The c string representation of the MIME object (or the
            flat text, resp.) is returned in resultingPart, so just
            use this string as body text of the surrounding MIME object.
            This string *is* encoded according to contentTEncClear
            and thus should be ready for being sent via SMTP.
    */
    bool processStructuringInfo( const QString bugURL,
                                 const QString contentDescriptionClear,
                                 const QByteArray contentTypeClear,
                                 const QByteArray contentSubtypeClear,
                                 const QByteArray contentDispClear,
                                 const QByteArray contentTEncClear,
                                 const QByteArray &bodytext,
                                 const QString contentDescriptionCiph,
                                 const QByteArray &ciphertext,
                                 KMMessagePart &resultingPart,
                                 bool signing, Kleo::CryptoMessageFormat format ) const;

    void encryptMessage( KMMessage *msg,
                         const Kleo::KeyResolver::SplitInfo &si,
                         bool doSign, bool doEncrypt,
                         KMMessagePart newBodyPart,
                         Kleo::CryptoMessageFormat format );

    void addBodyAndAttachments( KMMessage *msg,
                                const Kleo::KeyResolver::SplitInfo &si,
                                bool doSign, bool doEncrypt,
                                const KMMessagePart &ourFineBodyPart,
                                Kleo::CryptoMessageFormat format );

  private slots:
    void slotDoNextJob();

  private:
    void doNextJob();
    void emitDone( bool );

    bool determineWhetherToSign( bool doSignCompletely );
    bool determineWhetherToEncrypt( bool doEncryptCompletely );
    void markAllAttachmentsForSigning( bool sign );
    void markAllAttachmentsForEncryption( bool enc );

    /**
     * Loads the embedded images of the composer in a list
     */
    void loadImages();

    /**
     * Converts the img tags to tags with content-id's
     */
    void convertImageTags();

    /**
     * Prepares the body for multipart/related and adds the embedded images
     */
    void addEmbeddedImages( KMMessage &theMessage, QByteArray &body );

    /**
     * Prepares the body for adding attachments or embedded images
     *  An inner bodypart is needed when we have embedded images, or non-signed/non-encryped attachments
     *  An MPA body is assembled when using html. The body ends with a new boundary.
     *
     */
    void assembleInnerBodypart( KMMessage &theMessage, bool doSign, QByteArray &body, QString oldContentType="" );

    KMComposeWin *mComposeWin;
    MessageComposerJob *mCurrentJob;
    KMMessage *mReferenceMessage;
    QVector<KMMessage*> mMessageList;
    QList<QByteArray> mImages; // list with base64 encoded images
    QStringList mContentIDs; // content id's of the embedded images
    QStringList mImageNames; // names of the images as they are available as resource in the editor

    Kleo::KeyResolver *mKeyResolver;

    QByteArray mSignCertFingerprint;

    struct Attachment {
      Attachment( KMMessagePart *p=0, bool s=false, bool e=false )
        : part( p ), sign( s ), encrypt( e ) {}
      KMMessagePart *part;
      bool sign;
      bool encrypt;
    };
    QVector<Attachment> mAttachments;

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
    QByteArray mCharset;
    bool mIsRichText;
    uint mIdentityUid;
    bool mRc; // Set this to false, if something fails during the processes
    bool mHoldJobs; // Don't run the next job yet

    // The HTML source and the plain text version of the composer's text, encoded
    QByteArray mHtmlSource;
    QByteArray mPlainText;

    // Convenience variable, this is the same as one of the two variables above,
    // depending on the HTML mode
    QByteArray mBodyText; 

    // These are the variables of the big composeMessage(X,Y,Z) message
    KMMessagePart *mNewBodyPart;
    QByteArray mSignature;

    QByteArray mEncodedBody; // Only needed if signing and/or encrypting
    bool mEarlyAddAttachments; // Can attachments be added to the messagebody before signing/encrypting?
    bool  mAllAttachmentsAreInBody;
    KMMessagePart mOldBodyPart;
    int mPreviousBoundaryLevel;

    // The boundary is saved for later addition in the header
    DwString  mSaveBoundary;

    QList<MessageComposerJob*> mJobs;
    bool mEncryptWithChiasmus;
    bool mPerformingSignOperation;
};

#endif /* MESSAGECOMPOSER_H */
