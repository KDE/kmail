/*  -*- c++ -*-
    keyresolver.h

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2004 Klarälvdalens Datakonsult AB

    Based on kpgp.h
    Copyright (C) 2001,2002 the KPGP authors
    See file libkdenetwork/AUTHORS.kpgp for details

    Libkleopatra is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    Libkleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef __KLEO_KEYRESOLVER_H__
#define __KLEO_KEYRESOLVER_H__

#include <ui/keyapprovaldialog.h>

#include <kleo/enum.h>

#include <kpgp.h> // for Kpgp::Result
#include <gpgmepp/key.h>

#include <vector>

class QStringList;

namespace Kleo {


  /**
     \short A class to resolve signing/encryption keys w.r.t. per-recipient preferences

     \sect Step 1: Set the information needed

     The constructor takes some basic options as arguments, such as
     whether or not encryption was actually requested. Recipient and
     sender information is then set by using \c
     setEncryptToSelfKeys(), \c setSigningKeys(), \c
     setPrimaryRecipients() (To/Cc) and \c setSecondaryRecipients()
     (Bcc).

     \sect Step 2: Lookup and check per-recipient crypto preferences / Opportunistic Encryption

     First, \c checkSigningPreferences() goes through all recipient's
     signing perferences, to determine whether or not to sign. It also
     takes into account the available signing keys and whether or not
     the user explicitly requested signing.

     \c checkEncryptionPreferences() does the same for encryption
     preferences. If opportunistic encryption is enabled, recipients
     without encryption preferences set are treated as if they had a
     preference of \c AskWheneverPossible.

     In both cases an Action code is returned, with the following
     meanings:

     <dl><dt>Conflict</dt><dd>A conflict was detected. E.g. one
     recipient's preference was set to "always encrypt", while another
     one's preference was set to "never encrypt". You should ask the
     user what to do.</dd></dt>

     <dt>DoIt, DontDoIt</dt><dd>Do/Don't sign/encrypt</dd>

     <dt>Ask</dt><dd>(Some) crypto preferences request to prompt the
     user, so do it.</dd>

     <dt>Impossible</dt><dd>Signing or encryption is impossible,
     e.g. due to missing keys or unsupported formats.</dd> </dl>

     \sect Step 3: Resolve all keys.

     In case signing or encryption was implicitly or explicitly
     requested by the user, \c resolveAllKeys() tries to find signing
     keys for each required format, as well as encryption keys for all
     recipients (incl. the sender, if encrypt-to-self is set).

     \sect Step 4: Get signing keys.

     If, after key resolving, signing is still requested and
     apparently possible, you can get the result of all this by
     iterating over the available message formats and retrieving the
     set of signing keys to use with a call to \c signingKeys().

     \sect Step 5: Get encrytion key sets.

     If after key resolving, encryption is still requested and
     apparently possible, you can get the result of all this by
     calling \c encryptionItems() with the current message format at
     hand as its argument.

     This will return a list of recipient-list/key-list pairs that
     each describe a copy of the (possibly signed) message to be
     encrypted independantly.

     Note that it's only necessary to sign the message once for each
     message format, although it might be necessary to create more
     than one message when encrypting. This is because encryption
     allows the recipients to learn about the other recipients the
     message was encrypted to, so each secondary (BCC) recipient need
     a copy of it's own to hide the other secondary recipients.
    */

  class KeyResolver {
  public:
    KeyResolver( bool encToSelf, bool showApproval, bool oppEncryption,
		 unsigned int format,
		 int encrKeyNearExpiryThresholdDays,
		 int signKeyNearExpiryThresholdDays,
		 int encrRootCertNearExpiryThresholdDays,
		 int signRootCertNearExpiryThresholdDays,
		 int encrChainCertNearExpiryThresholdDays,
		 int signChainCertNearExpiryThresholdDays );

    ~KeyResolver();

    struct Item : public KeyApprovalDialog::Item {
      Item()
	: KeyApprovalDialog::Item(),
	  signPref( UnknownSigningPreference ),
	  format( AutoFormat ),
	  needKeys( true ) {}
      Item( const QString & a,
	    EncryptionPreference e, SigningPreference s,
	    CryptoMessageFormat f )
	: KeyApprovalDialog::Item( a, std::vector<GpgME::Key>(), e ),
	  signPref( s ), format( f ), needKeys( true ) {}
      Item( const QString & a, const std::vector<GpgME::Key> & k,
	    EncryptionPreference e, SigningPreference s,
	    CryptoMessageFormat f )
	: KeyApprovalDialog::Item( a, k, e ),
	  signPref( s ), format( f ), needKeys( false ) {}

      SigningPreference signPref;
      CryptoMessageFormat format;
      bool needKeys;
    };


    /**
       Set the fingerprints of keys to be used for encrypting to
       self. Also looks them up and complains if they're not usable or
       found.
    */
    Kpgp::Result setEncryptToSelfKeys( const QStringList & fingerprints );
    /**
	Set the fingerprints of keys to be used for signing. Also
	looks them up and complains if they're not usable or found.
    */
    Kpgp::Result setSigningKeys( const QStringList & fingerprints );
    /**
       Set the list of primary (To/CC) recipient addresses. Also looks
       up possible keys, but doesn't interact with the user.
    */
    void setPrimaryRecipients( const QStringList & addresses );
    /**
       Set the list of secondary (BCC) recipient addresses. Also looks
       up possible keys, but doesn't interact with the user.
    */
    void setSecondaryRecipients( const QStringList & addresses );


    /**
       Determine whether to sign or not, depending on the
       per-recipient signing preferences, as well as the availability
       of usable signing keys.
    */
    Action checkSigningPreferences( bool signingRequested ) const;
    /**
       Determine whether to encrypt or not, depending on the
       per-recipient encryption preferences, as well as the availability
       of usable encryption keys.
    */
    Action checkEncryptionPreferences( bool encryptionRequested ) const;

    /**
       Queries the user for missing keys and displays a key approval
       dialog if needed.
    */
    Kpgp::Result resolveAllKeys( bool signingRequested, bool encryptionRequested );

    /**
       @return the signing keys to use (if any) for the given message
       format.
    */
    std::vector<GpgME::Key> signingKeys( CryptoMessageFormat f ) const;

    struct SplitInfo {
      SplitInfo() {}
      SplitInfo( const QStringList & r ) : recipients( r ) {}
      SplitInfo( const QStringList & r, const std::vector<GpgME::Key> & k )
	: recipients( r ), keys( k ) {}
      QStringList recipients;
      std::vector<GpgME::Key> keys;
    };
    /** @return the found distinct sets of items for format \a f.  The
	returned vector will contain more than one item only if
	secondary recipients have been specified.
    */
    std::vector<SplitInfo> encryptionItems( CryptoMessageFormat f ) const;

  private:
    void dump() const;
    std::vector<Item> getEncryptionItems( const QStringList & recipients );
    std::vector<GpgME::Key> getEncryptionKeys( const QString & recipient, bool quiet ) const;

    Kpgp::Result showKeyApprovalDialog();

    bool encryptionPossible() const;
    bool signingPossible() const;
    Kpgp::Result resolveEncryptionKeys( bool signingRequested );
    Kpgp::Result resolveSigningKeysForEncryption();
    Kpgp::Result resolveSigningKeysForSigningOnly();
    Kpgp::Result checkKeyNearExpiry( const GpgME::Key & key,
				     const char * dontAskAgainName, bool mine,
				     bool sign, bool ca=false, int recurse_limit=100,
				     const GpgME::Key & orig_key=GpgME::Key::null ) const;
    void collapseAllSplitInfos();
    void addToAllSplitInfos( const std::vector<GpgME::Key> & keys, unsigned int formats );
    void addKeys( const std::vector<Item> & items, CryptoMessageFormat f );
    void addKeys( const std::vector<Item> & items );
    QStringList allRecipients() const;
    std::vector<GpgME::Key> signingKeysFor( CryptoMessageFormat f ) const;
    std::vector<GpgME::Key> encryptToSelfKeysFor( CryptoMessageFormat f ) const;

    std::vector<GpgME::Key> lookup( const QStringList & patterns, bool secret=false ) const;

    bool haveTrustedEncryptionKey( const QString & person ) const;

    std::vector<GpgME::Key> selectKeys( const QString & person, const QString & msg,
					const std::vector<GpgME::Key> & selectedKeys=std::vector<GpgME::Key>() ) const;

    QStringList keysForAddress( const QString & address ) const;
    void setKeysForAddress( const QString & address, const QStringList& pgpKeyFingerprints, const QStringList& smimeCertFingerprints ) const;

    bool encryptToSelf() const { return mEncryptToSelf; }
    bool showApprovalDialog() const { return mShowApprovalDialog; }

    int encryptKeyNearExpiryWarningThresholdInDays() const {
      return mEncryptKeyNearExpiryWarningThreshold;
    }
    int signingKeyNearExpiryWarningThresholdInDays() const {
      return mSigningKeyNearExpiryWarningThreshold;
    }

    int encryptRootCertNearExpiryWarningThresholdInDays() const {
      return mEncryptRootCertNearExpiryWarningThreshold;
    }
    int signingRootCertNearExpiryWarningThresholdInDays() const {
      return mSigningRootCertNearExpiryWarningThreshold;
    }

    int encryptChainCertNearExpiryWarningThresholdInDays() const {
      return mEncryptChainCertNearExpiryWarningThreshold;
    }
    int signingChainCertNearExpiryWarningThresholdInDays() const {
      return mSigningChainCertNearExpiryWarningThreshold;
    }

    struct ContactPreferences {
      ContactPreferences();
      Kleo::EncryptionPreference encryptionPreference;
      Kleo::SigningPreference signingPreference;
      Kleo::CryptoMessageFormat cryptoMessageFormat;
      QStringList pgpKeyFingerprints;
      QStringList smimeCertFingerprints;
    };

    ContactPreferences& lookupContactPreferences( const QString& address ) const;
    void saveContactPreference( const QString& email, const ContactPreferences& pref ) const;

  private:
    class EncryptionPreferenceCounter;
    friend class EncryptionPreferenceCounter;
    class SigningPreferenceCounter;
    friend class SigningPreferenceCounter;

    class Private;
    Private * d;

    bool mEncryptToSelf;
    const bool mShowApprovalDialog : 1;
    const bool mOpportunisticEncyption : 1;
    const unsigned int mCryptoMessageFormats;

    const int mEncryptKeyNearExpiryWarningThreshold;
    const int mSigningKeyNearExpiryWarningThreshold;
    const int mEncryptRootCertNearExpiryWarningThreshold;
    const int mSigningRootCertNearExpiryWarningThreshold;
    const int mEncryptChainCertNearExpiryWarningThreshold;
    const int mSigningChainCertNearExpiryWarningThreshold;
  };

} // namespace Kleo

#endif // __KLEO_KEYRESOLVER_H__
