/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#ifndef kmsender_h
#define kmsender_h
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

namespace KPIM {
  class ProgressItem;
}

class KMSender: public QObject
{
  Q_OBJECT
  friend class KMSendProc;

public:
  KMSender();
  virtual ~KMSender();

  /** Send given message. The message is either queued or sent
    immediately. The default behaviour, as selected with
    setSendImmediate(), can be overwritten with the parameter
    sendNow (by specifying TRUE or FALSE).
    The sender takes ownership of the given message on success,
    so DO NOT DELETE OR MODIFY the message further.
    Returns TRUE on success. */
  virtual bool send(KMMessage* msg, short sendNow=-1);

  /** Start sending all queued messages. Returns TRUE on success. */
  virtual bool sendQueued();

  /** Returns TRUE if sending is in progress. */
  bool sending(void) const { return mSendInProgress; }

  /** Shall messages be sent immediately (TRUE), or shall they be
    queued and sent later upon call of sendQueued() ? */
  bool sendImmediate(void) const { return mSendImmediate; }
  virtual void setSendImmediate(bool);

  /** Shall messages be sent quoted-printable encoded. No encoding
    happens otherwise. */
  bool sendQuotedPrintable(void) const { return mSendQuotedPrintable; }
  virtual void setSendQuotedPrintable(bool);

  /** Get the transport information */
  KMTransportInfo * transportInfo() { return mTransportInfo; }

  /** Read configuration from global config. */
  virtual void readConfig(void);

  /** Write configuration to global config with optional sync() */
  virtual void writeConfig(bool withSync=TRUE);

  /** sets a status msg and emits statusMsg() */
  void setStatusMsg(const QString&);

  /** sets replied/forwarded status in the linked message for @p aMsg. */
  void setStatusByLink(const KMMessage *aMsg);

  /** Emit progress info - calculates a percent value based on the amount of bytes sent */
  void emitProgressInfo( int currentFileProgress );

protected slots:
  /** Start sending */
  virtual void slotPrecommandFinished(bool);

  virtual void slotIdle();

  /** abort sending of the current message */
  virtual void slotAbortSend();

  /** initialization sequence has finised */
  virtual void sendProcStarted(bool success);

  /** note when a msg gets added to outbox during sending */
  void outboxMsgAdded(int idx);

protected:
  /** handle sending of messages */
  virtual void doSendMsg();

  /** second part of handling sending of messages */
  virtual void doSendMsgAux();

  /** cleanup after sending */
  virtual void cleanup(void);

  /** Test if all required settings are set.
      Reports problems to user via dialogs and returns FALSE.
      Returns TRUE if everything is Ok. */
  virtual bool settingsOk(void) const;

  /** Parse protocol '://' (host port? | mailer) string and
      set transport settings */
  virtual KMSendProc* createSendProcFromString(QString transport);

private:
  bool mSendImmediate;
  bool mSendQuotedPrintable;
  KMTransportInfo *mTransportInfo;
  KMPrecommand *mPrecommand;

  bool mSentOk, mSendAborted;
  QString mErrorMsg;
  KMSendProc *mSendProc;
  QString mMethodStr;
  bool mSendProcStarted;
  bool mSendInProgress;
  KMFolder *mOutboxFolder;
  KMFolder *mSentFolder;
  KMMessage * mCurrentMsg;
  KPIM::ProgressItem* mProgressItem;
  int mSentMessages, mTotalMessages;
  int mSentBytes, mTotalBytes;
  int mFailedMessages;
};


//-----------------------------------------------------------------------------
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

#endif /*kmsender_h*/
