/* -*- mode: C++; c-file-style: "gnu" -*-
 * User identity information
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmidentity_h
#define kmidentity_h

#include <qstring.h>
#include <qcstring.h>
#include <qstringlist.h>

class KMIdentity;
class KConfigBase;
class IdentityList;
class QDataStream;
namespace KMail {
  class SignatureConfigurator;
}

/**
 * @short abstraction of a signature (aka "footer").
 * @author Marc Mutz <mutz@kde.org>
 */
class Signature {
  friend class KMIdentity;
  friend class KMail::SignatureConfigurator;

  friend QDataStream & operator<<( QDataStream & stream, const Signature & sig );
  friend QDataStream & operator>>( QDataStream & stream, Signature & sig );

public:
  /** Type of signature (ie. way to obtain the signature text) */
  enum Type { Disabled = 0, Inlined = 1, FromFile = 2, FromCommand = 3 };

  /** Used for comparison */
  bool operator==( const Signature & other ) const;

  /** Constructor for disabled signature */
  Signature();
  /** Constructor for inline text */
  Signature( const QString & text );
  /** Constructor for text from a file or from output of a command */
  Signature( const QString & url, bool isExecutable );

  /** @return the raw signature text as entered resp. read from file. */
  QString rawText( bool * ok=0 ) const;

  /** @return the signature text with a "-- " separator added, if
      necessary. */
  QString withSeparator( bool * ok=0 ) const;

  /** Set the signature text and mark this signature as being of
      "inline text" type. */
  void setText( const QString & text ) { mText = text; mType = Inlined; }
  /** Set the signature URL and mark this signature as being of
      "from file" resp. "from output of command" type. */
  void setUrl( const QString & url, bool isExecutable=false );

protected:
  void writeConfig( KConfigBase * config ) const;
  void readConfig( const KConfigBase * config );

  Type type() const { return mType; }

  QString url() const { return mUrl; }
  QString text() const { return mText; }

protected:
  QString mUrl;
  QString mText;
  Type    mType;

private:
  QString textFromFile( bool * ok ) const;
  QString textFromCommand( bool * ok ) const;
};

/** User identity information */
class KMIdentity
{
  // only the identity manager should be able to construct and
  // destruct us, but then we get into problems with using
  // QValueList<KMIdentity> and especially qHeapSort().
  friend class IdentityManager;

  friend QDataStream & operator<<( QDataStream & stream, const KMIdentity & ident );
  friend QDataStream & operator>>( QDataStream & stream, KMIdentity & ident );

public:
  typedef QValueList<KMIdentity> List;

  /** used for comparison */
  bool operator==( const KMIdentity & other ) const;

  bool operator!=( const KMIdentity & other ) const {
    return !operator==( other );
  }

  /** used for sorting */
  bool operator<( const KMIdentity & other ) const {
    if ( isDefault() ) return true;
    if ( other.isDefault() ) return false;
    return identityName() < other.identityName();
  }
  bool operator>( const KMIdentity & other ) const {
    if ( isDefault() ) return false;
    if ( other.isDefault() ) return true;
    return identityName() > other.identityName();
  }
  bool operator<=( const KMIdentity & other ) const {
    return !operator>( other );
  }
  bool operator>=( const KMIdentity & other ) const {
    return !operator<( other );
  }

  /** Constructor */
  explicit KMIdentity( const QString & id=QString::null,
		       const QString & realName=QString::null,
		       const QString & emailAddr=QString::null,
		       const QString & organization=QString::null,
		       const QString & replyToAddress=QString::null );

  /** Destructor */
  ~KMIdentity();

protected:
  /** Read configuration from config. Group must be preset (or use
      @ref KConfigGroup). Called from @ref IdentityManager. */
  void readConfig( const KConfigBase * );

  /** Write configuration to config. Group must be preset (or use @ref
      KConfigGroup). Called from @ref IdentityManager. */
  void writeConfig( KConfigBase * ) const;

public:
  /** Tests if there are enough values set to allow mailing */
  bool mailingAllowed() const;

  /** Identity/nickname for this collection */
  QString identityName() const { return mIdentity; }
  void setIdentityName( const QString & name );

  /** @return whether this identity is the default identity */
  bool isDefault() const { return mIsDefault; }

  uint uoid() const { return mUoid; }

protected:
  /** Set whether this identity is the default identity. Since this
      affects all other identites, too (most notably, the old default
      identity), only the @ref IdentityManager can change this.
      You should use
      <pre>
      kmkernel->identityManager()->setAsDefault( name_of_default )
      </pre>
      instead.
  **/
  void setIsDefault( bool flag );

  void setUoid( uint aUoid ) { mUoid = aUoid; }

public:
  /** Full name of the user */
  QString fullName() const { return mFullName; }
  void setFullName(const QString&);

  /** The user's organization (optional) */
  QString organization() const { return mOrganization; }
  void setOrganization(const QString&);

  /** The user's PGP identity */
  QCString pgpIdentity() const { return mPgpIdentity; }
  void setPgpIdentity(const QCString&);

  /** email address (without the user name - only name@host) */
  QString emailAddr() const { return mEmailAddr; }
  void setEmailAddr(const QString&);

  /** vCard to attach to outgoing emails */
  QString vCardFile() const { return mVCardFile; }
  void setVCardFile(const QString&);

  /** email address in the format "username <name@host>" suitable
    for the "From:" field of email messages. */
  QString fullEmailAddr() const;

  /** email address for the ReplyTo: field */
  QString replyToAddr() const { return mReplyToAddr; }
  void setReplyToAddr(const QString&);

  /** email addresses for the BCC: field */
  QString bcc() const { return mBcc; }
  void setBcc(const QString& aBcc) { mBcc = aBcc; }

  void setSignature( const Signature & sig ) { mSignature = sig; }
  Signature & signature() /* _not_ const! */ { return mSignature; }

protected:
  /** @return true if the signature is read from the output of a command */
  bool signatureIsCommand() const { return mSignature.type() == Signature::FromCommand; }
  /** @return true if the signature is read from a text file */
  bool signatureIsPlainFile() const { return mSignature.type() == Signature::FromFile; }
  /** @return true if the signature was specified directly */
  bool signatureIsInline() const { return mSignature.type() == Signature::Inlined; }

  /** name of the signature file (with path) */
  QString signatureFile() const { return mSignature.url(); }
  void setSignatureFile(const QString&);

  /** inline signature */
  QString signatureInlineText() const { return mSignature.text();}
  void setSignatureInlineText(const QString&);

  /** Inline or signature from a file */
  bool useSignatureFile() const { return signatureIsPlainFile() || signatureIsCommand(); }

public:
  /** Returns the signature. This method also takes care of special
    signature files that are shell scripts and handles them
    correct. So use this method to rectreive the contents of the
    signature file. If @p prompt is false, no errors will be displayed
    (useful for retries). */
  QString signatureText( bool * ok=0) const;

  /** The transport that is set for this identity. Used to link a
      transport with an identity. */
  QString transport() const { return mTransport; }
  void setTransport(const QString&);

  /** The folder where sent messages from this identity will be
      stored by default. */
  QString fcc() const { return mFcc; }
  void setFcc(const QString&);

  /** The folder where draft messages from this identity will be
      stored by default. */
  QString drafts() const { return mDrafts; }
  void setDrafts(const QString&);

  /** dictionary which should be used for spell checking */
  QString dictionary() const { return mDictionary; }
  void setDictionary( const QString& );

  static KMIdentity null;
  bool isNull() const;
protected:
  // if you add new members, make sure they have an operator= (or the
  // compiler can synthesize one) and amend KMIdentity::operator==,
  // isNull(), readConfig() and writeConfig() as well as operator<<
  // and operator>> accordingly:
  uint mUoid;
  QString mIdentity, mFullName, mEmailAddr, mOrganization;
  QString mReplyToAddr;
  QString mBcc;
  QString mVCardFile;
  QCString mPgpIdentity;
  QString mFcc, mDrafts, mTransport;
  QString mDictionary;
  Signature mSignature;
  bool      mIsDefault;
};

QDataStream & operator<<( QDataStream & stream, const Signature & sig );
QDataStream & operator>>( QDataStream & stream, Signature & sig );

QDataStream & operator<<( QDataStream & stream, const KMIdentity & ident );
QDataStream & operator>>( QDataStream & stream, KMIdentity & ident );


#endif /*kmidentity_h*/
