/*
  KMail Mail Sender

  Copyright (c) 1997-1998 Stefan Taferner <taferner@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef __KMAIL_SENDER_P_H__
#define __KMAIL_SENDER_P_H__
#include "kmsender.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QObject>
#include <kio/global.h>
#include <kdeversion.h>

class K3Process;

namespace KIO {
  class Job;
  class TransferJob;
  class Slave;
}

class KJob;
class KMSendProc: public QObject
{
  Q_OBJECT

public:
  KMSendProc(KMSender*);
  virtual ~KMSendProc() {}

  /** Initialize sending of one or more messages. */
  void start() { emit started( doStart() ); }

  /** Send given message. May return before message is sent. */
  bool send( const QString & sender, const QStringList & to, const QStringList & cc, const QStringList & bcc, const QByteArray & message ) {
    reset(); return doSend( sender, to, cc, bcc, message );
  }

  /** Cleanup after sending messages. */
  void finish() { doFinish(); deleteLater(); }

  /** Abort sending the current message. Sets mSending to false */
  virtual void abort() = 0;

  /** Returns true if send was successful, and false otherwise.
      Returns false if sending is still in progress. */
  bool sendOk() const { return mSendOk; }

  /** Returns error message of last call of failed(). */
  QString lastErrorMessage() const { return mLastErrorMessage; }

signals:
  /** Emitted when the current message is sent or an error occurred. */
  void idle();

  /** Emitted when the initialization sequence has finished */
  void started(bool);


protected:
  /** Called to signal a transmission error. The sender then
    calls finish() and terminates sending of messages.
    Sets mSending to false. */
  void failed(const QString &msg);

  /** Informs the user about what is going on. */
  void statusMsg(const QString&);

private:
  void reset();

private:
  virtual void doFinish() = 0;
  virtual bool doSend( const QString & sender, const QStringList & to, const QStringList & cc, const QStringList & bcc, const QByteArray & message ) = 0;
  virtual bool doStart() = 0;

protected:
  KMSender* mSender;
  QString mLastErrorMessage;
  bool mSendOk : 1;
  bool mSending : 1;
};


//-----------------------------------------------------------------------------
class KMSendSendmail: public KMSendProc
{
  Q_OBJECT
public:
  KMSendSendmail(KMSender*);
  ~KMSendSendmail();
  void start();
  void abort();

protected slots:
  void receivedStderr(K3Process*,char*,int);
  void wroteStdin(K3Process*);
  void sendmailExited(K3Process*);

private:
  /** implemented from KMSendProc */
  void doFinish();
  /** implemented from KMSendProc */
  bool doSend( const QString & sender, const QStringList & to, const QStringList & cc, const QStringList & bcc, const QByteArray & message );
  /** implemented from KMSendProc */
  bool doStart();

private:
  QByteArray mMsgStr;
  char* mMsgPos;
  int mMsgRest;
  K3Process* mMailerProc;
};

//-----------------------------------------------------------------------------
class KMSendSMTP : public KMSendProc
{
Q_OBJECT
public:
  KMSendSMTP(KMSender *sender);
  ~KMSendSMTP();

  void abort();

private slots:
  void dataReq(KIO::Job *, QByteArray &);
  void result(KJob *);
  void slaveError(KIO::Slave *, int, const QString &);

private:
  /** implemented from KMSendProc */
  void doFinish();
  /** implemented from KMSendProc */
  bool doSend( const QString & sender, const QStringList & to, const QStringList & cc, const QStringList & bcc, const QByteArray & message );
  /** implemented from KMSendProc */
  bool doStart() { return true; }

  void cleanup();

private:
  QByteArray mMessage;
  uint mMessageLength;
  uint mMessageOffset;

  bool mInProcess;

  KIO::TransferJob *mJob;
  KIO::Slave *mSlave;
};

#endif /* __KMAIL_SENDER_P_H__ */
