/** KPGP: Pretty good privacy en-/decryption class
 *        This code is under GPL V2.0
 *
 * @author Lars Knoll <knoll@mpi-hd.mpg.de>
 */
#ifndef KPGPBASE_H
#define KPGPBASE_H

#include <qstring.h>
#include <qstrlist.h>

class KpgpBase
{
public:

  enum{ 
    OK = 0,
      CLEARTEXT   =  0,
      RUN_ERR     =  1,
      ERROR       =  1,
      ENCRYPTED   =  2,
      SIGNED      =  4,
      GOODSIG     =  8,
      ERR_SIGNING = 16,
      UNKNOWN_SIG = 32,
      BADPHRASE   = 64,
      BADKEYS     =128,
      NO_SEC_KEY  =256, 
      MISSINGKEY  =512 };
  
  /** virtual class used internally by kpgp */
  KpgpBase();
  virtual ~KpgpBase();
  
  virtual void readConfig();
  virtual void writeConfig(bool sync = false);

  /** set and retrieve the message to handle */
  virtual bool setMessage(const QString mess);
  virtual QString message() const;

  /** manipulating the message */
  virtual int encrypt(const QStrList *, bool = false) { return OK; };
  virtual int sign(const char *) { return OK; };
  virtual int encsign(const QStrList *, const char * = 0,
		      bool = false) { return OK; };
  virtual int decrypt(const char * = 0) { return OK; };
  virtual QStrList pubKeys() { return OK; };
  virtual QString getAsciiPublicKey(QString) { return QString::null; };
  virtual int signKey(const char *, const char *) { return OK; };

  /** various functions to get the status of a message */
  bool isEncrypted() const;
  bool isSigned() const;
  bool isSigGood() const;
  bool unknownSigner() const;
  int getStatus() const;
  QString signedBy() const;
  QString signedByKey() const;
  const QStrList *receivers() const;

  virtual QString lastErrorMessage() const;

  /** functions that modify the behaviour of kpgp */
  void setUser(QString aUser);
  const QString user() const;
  void setEncryptToSelf(bool flag);
  bool encryptToSelf(void) const;

  /// kpgp needs to access this function
  void clearOutput();

protected:
  virtual int run(const char *cmd, const char *passphrase = 0);
  virtual void clear();

  QString addUserId();

  QString input;
  QString output;
  QString info;
  QString errMsg;
  QString pgpUser;
  QString signature;
  QString signatureID;
  QStrList recipients;

  bool flagEncryptToSelf;
  int status;

};

// ---------------------------------------------------------------------------

class KpgpBase2 : public KpgpBase
{

public:
  KpgpBase2();
  virtual ~KpgpBase2();

  virtual int encrypt(const QStrList *recipients,
		      bool ignoreUntrusted = false);
  virtual int sign(const char *passphrase);
  virtual int encsign(const QStrList *recipients, const char *passphrase = 0,
		      bool ingoreUntrusted = false);
  virtual int decrypt(const char *passphrase = 0);
  virtual QStrList pubKeys();
  virtual QString getAsciiPublicKey(QString _person);
  virtual int signKey(const char *key, const char *passphrase);
};


class KpgpBase5 : public KpgpBase
{

public:
  KpgpBase5();
  virtual ~KpgpBase5();

  virtual int encrypt(const QStrList *recipients,
		      bool ignoreUntrusted = false);
  virtual int sign(const char *passphrase);
  virtual int encsign(const QStrList *recipients, const char *passphrase = 0,
		      bool ingoreUntrusted = false);
  virtual int decrypt(const char *passphrase = 0);
  virtual QStrList pubKeys();
  virtual QString getAsciiPublicKey(QString _person);
  virtual int signKey(const char *key, const char *passphrase);
};

// ---------------------------------------------------------------------------
// inlined functions

inline void
KpgpBase::setUser(QString aUser)
{
  pgpUser = aUser;
}

inline const QString
KpgpBase::user() const
{
  return pgpUser;
}

inline void 
KpgpBase::setEncryptToSelf(bool flag)
{
  flagEncryptToSelf = flag;
}

inline bool 
KpgpBase::encryptToSelf(void) const
{
  return flagEncryptToSelf;
}

inline bool 
KpgpBase::isEncrypted() const
{
  if( status & ENCRYPTED )
    return true;
  return false;
}

inline bool 
KpgpBase::isSigned() const
{
  if( status & SIGNED )
    return true;
  return false;
}

inline bool 
KpgpBase::isSigGood() const
{
  if( status & GOODSIG )
    return true;
  return false;
}

inline bool 
KpgpBase::unknownSigner() const
{
  if( status & UNKNOWN_SIG )
    return true;
  return false;
}

inline const QStrList *
KpgpBase::receivers() const
{
  if(recipients.count() <= 0) return 0;
  return &recipients;
}

inline int
KpgpBase::getStatus() const
{
  return status;
}

inline QString 
KpgpBase::signedBy(void) const
{
  return signature;
}

inline QString 
KpgpBase::signedByKey(void) const
{
  return signatureID;
}

#endif
