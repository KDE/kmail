/* User identity information
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmidentity_h
#define kmidentity_h

#include <qstring.h>

class KMIdentity
{
public:
  /** Constructor loads config file */
  KMIdentity();

  /** Destructor saves config file */
  virtual ~KMIdentity();

  /** Read configuration from the global config */
  virtual void readConfig(void);

  /** Write configuration to the global config with optional sync */
  virtual void writeConfig(bool withSync=TRUE);

  /** Tests if there are enough values set to allow mailing */
  virtual bool mailingAllowed(void) const;

  /** Identity/nickname fot this collection */
  const QString identity(void) const { return mIdentity; }
  virtual void setIdentity(const QString);

  /** Full name of the user */
  const QString fullName(void) const { return mFullName; }
  virtual void setFullName(const QString);

  /** The user's organization (optional) */
  const QString organization(void) const { return mOrganization; }
  virtual void setOrganization(const QString);

  /** email address (without the user name - only name@host) */
  const QString emailAddr(void) const { return mEmailAddr; }
  virtual void setEmailAddr(const QString);

  /** email address in the format "username <name@host>" suitable
    for the "From:" field of email messages. */
  const QString fullEmailAddr(void) const;

  /** email address for the ReplyTo: field */
  const QString replyToAddr(void) const { return mReplyToAddr; }
  virtual void setReplyToAddr(const QString);

  /** name of the signature file (with path) */
  const QString signatureFile(void) const { return mSignatureFile; }
  virtual void setSignatureFile(const QString);
  
  /** inline signature */
  const QString signatureInlineText(void) const { return mSignatureInlineText;}
  virtual void setSignatureInlineText(const QString);

  /** Inline or signature from a file */
  bool useSignatureFile(void) { return mUseSignatureFile; }
  void setUseSignatureFile(bool);

  /** Returns the signature. This method also takes care of
    special signature files that are shell scripts and handles
    them correct. So use this method to rectreive the contents of
    the signature file. */
  virtual const QString signature(void) const;

protected:
  QString mIdentity, mFullName, mOrganization, mEmailAddr;
  QString mReplyToAddr, mSignatureFile;
  QString mSignatureInlineText;
  bool    mUseSignatureFile;
};

#endif /*kmidentity_h*/
