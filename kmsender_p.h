/* KMail Mail Sender
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef __KMAIL_SENDER_P_H__
#define __KMAIL_SENDER_P_H__
#include "kmsender.h"

#include <qcstring.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>
#include <kio/global.h>
#include <kdeversion.h>

class KMMessage;
class KMFolder;
class KMFolderMgr;
class KConfig;
class KProcess;
class KMSendProc;
class QStrList;
class KMTransportInfo;
class KMPrecommand;

namespace KIO {
  class Job;
  class TransferJob;
  class Slave;
}

namespace KMime {
  namespace Types {
    class AddrSpec;
    typedef QValueList<AddrSpec> AddrSpecList;
  }
}


class KMSendProc: public QObject
{
  Q_OBJECT

public:
  KMSendProc(KMSender*);
  virtual ~KMSendProc() {}

  /** Initialize sending of one or more messages. */
  virtual void start(void);

  /** Initializes variables directly before send() is called. */
  virtual void preSendInit(void);

  /** Send given message. May return before message is sent. */
  virtual bool send(KMMessage* msg) = 0;

  /** Cleanup after sending messages. */
  virtual bool finish(bool destructive);

  /** Abort sending the current message. Sets mSending to false */
  virtual void abort() = 0;

  /** Returns TRUE if send was successful, and FALSE otherwise.
      Returns FALSE if sending is still in progress. */
  bool sendOk(void) const { return mSendOk; }

  /** Returns TRUE if sending is still in progress. */
  bool sending(void) const { return mSending; }

  /** Returns error message of last call of failed(). */
  QString message(void) const { return mMsg; }

signals:
  /** Emitted when the current message is sent or an error occurred. */
  void idle();

  /** Emitted when the initialization sequence has finished */
  void started(bool);


protected:
  /** Called to signal a transmission error. The sender then
    calls finish() and terminates sending of messages.
    Sets mSending to FALSE. */
  virtual void failed(const QString &msg);

  /** Informs the user about what is going on. */
  virtual void statusMsg(const QString&);

  /** Called once for the contents of the header fields To, Cc, and Bcc.
    Returns TRUE on success and FALSE on failure.
    Calls addOneRecipient() for each recipient in the list. Aborts and
    returns FALSE if addOneRecipient() returns FALSE. */
  virtual bool addRecipients(const KMime::Types::AddrSpecList & aRecpList);

  /** Called from within addRecipients() once for each recipient in
    the list after all surplus characters have been stripped. E.g.
    for: "Stefan Taferner" <taferner@kde.org>
    addRecpient(taferner@kde.org) is called.
    Returns TRUE on success and FALSE on failure. */
  virtual bool addOneRecipient(const QString& aRecipient) = 0;

protected:
  bool mSendOk, mSending;
  QString mMsg;
  KMSender* mSender;
};


//-----------------------------------------------------------------------------
class KMSendSendmail: public KMSendProc
{
  Q_OBJECT
public:
  KMSendSendmail(KMSender*);
  virtual ~KMSendSendmail();
  virtual void start(void);
  virtual bool send(KMMessage* msg);
  virtual bool finish(bool destructive);
  virtual void abort();

protected slots:
  void receivedStderr(KProcess*,char*,int);
  void wroteStdin(KProcess*);
  void sendmailExited(KProcess*);

protected:
  virtual bool addOneRecipient(const QString& aRecipient);

  QCString mMsgStr;
  char* mMsgPos;
  int mMsgRest;
  KProcess* mMailerProc;
};

//-----------------------------------------------------------------------------
class KMSendSMTP : public KMSendProc
{
Q_OBJECT
public:
  KMSendSMTP(KMSender *sender);
  ~KMSendSMTP();

  virtual bool send(KMMessage *);
  virtual void abort();
  virtual bool finish(bool);

protected:
  virtual bool addOneRecipient(const QString& aRecipient);

private slots:
  void dataReq(KIO::Job *, QByteArray &);
  void result(KIO::Job *);
  void slaveError(KIO::Slave *, int, const QString &);

private:
  QString mQuery;
  QString mQueryField;
  QCString mMessage;
  uint mMessageLength;
  uint mMessageOffset;

  bool mInProcess;

  KIO::TransferJob *mJob;
  KIO::Slave *mSlave;
};

#endif /* __KMAIL_SENDER_P_H__ */
