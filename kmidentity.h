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
  const char* fullName(void) const { return mFullName; }
  virtual void setFullName(const QString);

  /** The user's organization (optional) */
  const char* organization(void) const { return mOrganization; }
  virtual void setOrganization(const QString);

  /** email address (without the user name - only name@host) */
  const char* emailAddr(void) const { return mEmailAddr; }
  virtual void setEmailAddr(const QString);

  /** email address for the ReplyTo: field */
  const char* replyToAddr(void) const { return mReplyToAddr; }
  virtual void setReplyToAddr(const QString);

  /** name of the signature file (with path) */
  const char* signatureFile(void) const { return mSignatureFile; }
  virtual void setSignatureFile(const QString);

protected:
  QString mFullName, mOrganization, mEmailAddr;
  QString mReplyToAddr, mSignatureFile; 
};

#endif /*kmidentity_h*/
