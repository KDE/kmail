/* KMail Mail Sender
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmsender_h
#define kmsender_h

#include <qstring.h>

class KMMessage;
class KMAcctFolder;
class KMFolderMgr;
class KConfig;
class KProcess;

class KMSender
{
public:
  enum Method { smUnknown=0, smSMTP=1, smMail=2 };

  KMSender(KMFolderMgr*);
  virtual ~KMSender();

  /** Send given message. The message is either queued or sent
    immediately. The default behaviour, as selected with 
    setSendImmediate(), can be overwritten with the parameter
    sendNow (by specifying TRUE or FALSE).
    The sender takes ownership of the given message, so DO NOT
    DELETE OR MODIFY the message further.
    Returns TRUE on success. */
  virtual bool send(KMMessage* msg, short sendNow=-1);

  /** Send all queued messages. Returns TRUE on success. */
  virtual bool sendQueued(void);

  /** Provides direct access to the folder where the messages
    are queued. */
  KMAcctFolder* messageQueue(void) const { return mQueue; }

  /** Method the sender shall use: either SMTP or local mail program */
  Method method(void) const { return mMethod; }
  virtual void setMethod(Method);

  /** Shall messages be sent immediately (TRUE), or shall they be
    queued and sent later upon call of sendQueued() ? */
  bool sendImmediate(void) const { return mSendImmediate; }
  virtual void setSendImmediate(bool);

  /** Name of the mail client (usually "/usr/bin/mail") that
    is used for mailing if the method is smMail */
  const QString& mailer(void) const { return mMailer; }
  virtual void setMailer(const QString&);

  /** Name of the host that is contacted via SMTP if the mailing
    method is smSMTP. */
  const QString& smtpHost(void) const { return mSmtpHost; }
  virtual void setSmtpHost(const QString&);

  /** Port of the SMTP host, usually 110. */
  int smtpPort(void) const { return mSmtpPort; }
  virtual void setSmtpPort(int);

protected:
  // send given message via SMTP.
  virtual bool sendSMTP(KMMessage*);

  // send given message via local mailer
  virtual bool sendMail(KMMessage*);

private:
  Method mMethod;
  bool mSendImmediate;
  KConfig* mCfg;
  KMFolderMgr* mFolderMgr;
  KMAcctFolder* mQueue;
  KProcess* mMailerProc;
  QString mMailer;
  QString mSmtpHost;
  int mSmtpPort;
};

#endif /*kmsender_h*/
