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

protected:
  QString mFullName, mOrganization, mEmailAddr;
  QString mReplyToAddr, mSignatureFile; 
};

#endif /*kmidentity_h*/
