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

#ifndef BUILD_NEW_COMPOSER
#ifndef MESSAGECOMPOSER_H
#define MESSAGECOMPOSER_H

#include "kmmsgpart.h"
#include "keyresolver.h"

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QSharedPointer>

#include <mimelib/mediatyp.h>
#include "kleo/cryptobackend.h"

#include <boost/shared_ptr.hpp>
#include <vector>

class KMMessage;
class KMComposeWin;

class MessageComposerJob;
class EncryptMessageJob;
class SetLastMessageAsUnencryptedVersionOfLastButOne;
class DwBodyPart;

namespace Kleo {
  class KeyResolver;
}

namespace GpgME {
  class Key;
}

namespace KPIM {
  class Identity;
}

namespace KPIMTextEdit {
  struct EmbeddedImage;
}

/**
 * The MessageComposer actually creates and composes the message(s).
 */
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
      case of an error (e.g. if PGP encryption fails). (huh? void)
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

    /**
     * This reads information which we need from the composer window and saves it into
     * member variables.
     * These include the composer text, the attachments, subject, to, cc, the reference message,
     * the charset, the embedded images and so on.
     */
    void readFromComposeWin();

    /**
     * This adjusts the signing/encryption policy for the message text and the attachments,
     * based on various factors.
     * It is called before composing the message with composeMessage().
     */
    void adjustCryptFlags();

    bool encryptWithChiasmus( const Kleo::CryptoBackend::Protocol *chiasmus,
                              const QByteArray &body,
                              QByteArray &resultData ) const;
    void chiasmusEncryptAllAttachments();
    void composeChiasmusMessage( KMMessage &theMessage,
                                 Kleo::CryptoMessageFormat format );

    /**
     * This calls the other composeMessage() for each message that should be created.
     * There can be multiple messages created, for example if we sent the encrypted version
     * but save the unencrypted version.
     */
    void composeMessage();

    // And these two are the parts that should be run after job completions
    void createUnencryptedMessageVersion();

    /**
     * This is the main composing function. It creates the main body part and
     * signs it, then calls continueComposeMessage().
     *
     * It is called from the other composeMessage() and can be called multiple times from there,
     * to allow composing several messages (encrypted and unencrypted) based on the
     * same composer content. That's useful for storing decrypted versions of messages which
     * were sent in encrypted form.
     */
    void composeMessage( KMMessage &theMessage,
                         bool doSign, bool doEncrypt,
                         Kleo::CryptoMessageFormat format );

    /**
     * This creates the EncryptMessageJobs for the message and adds them to the job queue.
     */
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

    /**
     * Builds a MIME object (or a flat text resp.) based upon structuring
     * information returned by a crypto plugin that was called via
     * pgpSignedMsg() (or pgpEncryptedMsg(), resp.).
     *
     * @return: The string representation of the MIME object (or the
     *          flat text, resp.) is returned in resultingPart, so just
     *          use this string as body text of the surrounding MIME object.
     *          This string *is* encoded according to contentTEncClear
     *          and thus should be ready for being sent via SMTP.
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

    /**
     * This function creates the final message.
     * If there are late attachments, they are added to the message, including signing/encryption.
     * This actually adds the body parts we have created before to the main message (@p msg).
     * After this, the message is complete and can be sent or saved.
     */
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
     * Returns the inner body part.
     *
     * The inner body part is either multipart/alternative or text/plain, depending on whether
     * the message is a HTML message.
     *
     * The boundary of the multipart/alternative is saved to mSaveBoundary.
     *
     * @param oldContentType If this is not empty and the message is not HTML, the body part will
     *                       be of this content type, and not text/plain
     * @return the complete inner body part
     */
    boost::shared_ptr<DwBodyPart> innerBodypart( KMMessage &theMessage,
                                                 bool doSign, QString oldContentType = QString() );

    /**
     * Helper function for innerBodypart(). This creates and returns the body of the inner body
     * part, which contains the text/plain and the text/html parts (with boundaries), or only
     * the text/plain part, depending on whether the message is HTML.
     *
     * The boundary seperating the text/plain and text/html parts is saved in mSaveBoundary.
     */
    QByteArray innerBodypartBody( KMMessage &theMessage, bool doSign );

    /**
     * Returns the image body part.
     *
     * If the message has embedded HTML images, this is a multipart/related body part which
     * contains the inner body part and all related images.
     * If the message has no embedded HTML images, this is the same as innerBodyPart.
     *
     * The boundary of the multipart/related is saved to mSaveBoundary.
     *
     * @param innerBodyPart the inner body part created so far
     * @return the complete image body part
     */
    boost::shared_ptr<DwBodyPart> imageBodyPart( KMMessage &theMessage,
                                                 boost::shared_ptr<DwBodyPart> innerBodyPart );

    /**
     * Returns the mixed body part
     *
     * This is a multipart/mixed body part which contains all early attachements,
     * and the imageBodyPart.
     * If there are no early attachments, this is the same as imageBodyPart.
     *
     * The boundary of the multipart/mixed is saved to mSaveBoundary.
     *
     * @param imageBodyPart the image body part created so far
     * @return the complete mixed body part
     */
    boost::shared_ptr<DwBodyPart> mixedBodyPart( KMMessage &theMessage,
                                                 boost::shared_ptr<DwBodyPart> imageBodyPart,
                                                 bool doSignBody, bool doEncryptBody );

    // The composer window. Mainly used in readFromComposeWin() to read the information from it.
    KMComposeWin *mComposeWin;

    MessageComposerJob *mCurrentJob;
    KMMessage *mReferenceMessage;

    // This is the list of messages we composed. Each completed message is added to this list, and
    // the composer window uses the messages from this list for sending and saving them.
    QVector<KMMessage*> mMessageList;

    Kleo::KeyResolver *mKeyResolver;

    QByteArray mSignCertFingerprint;

    // The list of all attachements, including signing/encryption policy
    struct Attachment {
      Attachment( KMMessagePart *p=0, bool s=false, bool e=false )
        : part( p ), sign( s ), encrypt( e ) {}
      KMMessagePart *part;
      bool sign;
      bool encrypt;
    };
    QVector<Attachment> mAttachments;

    // The list of embedded HTML images of the editor
    QList< QSharedPointer<KPIMTextEdit::EmbeddedImage> > mEmbeddedImages;

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

    // If true, we automatically determine the charset of the text part instead of taking
    // the charset that is set in the composer window (mCharset).
    bool mAutoCharset;

    // The charset which will be used for the text part. It is read from the composer window.
    QByteArray mCharset;

    // True if this message is a HTML message. In this case, a multipart/alternative part will
    // be created, with both HTML and plain text subparts.
    bool mIsRichText;

    // The identity UID of the sender, as read from the composer window. Used, for example,
    // for key retrival.
    uint mIdentityUid;

    // Set this to false if something fails during the processes.
    bool mRc;

    // Don't run the next job yet
    bool mHoldJobs;

    // The HTML source and the plain text version of the composer's text, encoded
    QByteArray mHtmlSource;
    QByteArray mPlainText;

    // Convenience variable, this is the same as one of the two variables above,
    // depending on the HTML mode
    QByteArray mBodyText;

    // The old body part contains the main body part, i.e. the body part before doing
    // signing/encryption and before adding late attachments.
    // (Note that I don't know if that also applies for inline OpenPGP and Chiasmus)
    KMMessagePart mOldBodyPart;

    // The new body part, which contains the body part that has all the signing/encryption stuff
    // and late attachments in it.
    // This is the final body part which is set as the body of the final message.
    KMMessagePart *mNewBodyPart;

    QByteArray mSignature;

    // This is the main body part, in the encoded version.
    // It is only used for signing and encrypting and created in composeMessage(), after the main
    // body part has been created.
    QByteArray mEncodedBody;

    // True if at least one attachment will be added to the main body part, i.e. added before
    // signing/encrypting. Opposed to this, there are 'late' attachments, which are added after
    // signing/encrypting, in addBodyAndAttachments().
    bool mEarlyAddAttachments;

    // True if all attachments are added early, see above.
    bool mAllAttachmentsAreInBody;

    int mPreviousBoundaryLevel;

    // The boundary of the body part body which we created in the last step.
    // When creating body part bodies, for example with innerBodypartBody(), we
    // add a boundary seperating the child body parts. However, this boundary needs
    // to appear in the content-type header of the body part header, so we need to remember
    // the boundary and add it to the content-type header when later creating the body part.
    DwString mSaveBoundary;

    // A list of all jobs which are pending execution
    QList<MessageComposerJob*> mJobs;

    bool mEncryptWithChiasmus;
    bool mPerformingSignOperation;
};

#endif /* MESSAGECOMPOSER_H */
#endif // not BUILD_NEW_COMPOSER
