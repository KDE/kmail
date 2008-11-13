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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef kmsender_h
#define kmsender_h
#include "messagesender.h"

#ifndef KDE_USE_FINAL
# ifndef REALLY_WANT_KMSENDER
#  error Do not include kmsender.h, but messagesender.h.
# endif
#endif

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QObject>

class KMMessage;
class KMFolder;
class KJob;

namespace KPIM {
  class ProgressItem;
}
namespace MailTransport {
  class TransportJob;
}

class KMSender: public QObject, public KMail::MessageSender
{
  Q_OBJECT

public:
  KMSender();
  ~KMSender();

protected:
  /** Send given message. The message is either queued or sent
    immediately. The default behaviour, as selected with
    setSendImmediate(), can be overwritten with the parameter
    sendNow (by specifying true or false).
    The sender takes ownership of the given message on success,
    so DO NOT DELETE OR MODIFY the message further.
    Returns true on success. */
  bool doSend(KMMessage* msg, short sendNow);

  /** Send queued messages, using the specified transport or the
   * default, if none is given.
   */
  bool doSendQueued( const QString& transport=QString() );

private:
  /** Returns true if sending is in progress. */
  bool sending() const { return mSendInProgress; }

public:
  /** Shall messages be sent immediately (true), or shall they be
    queued and sent later upon call of sendQueued() ? */
  bool sendImmediate() const { return mSendImmediate; }
  void setSendImmediate(bool);

  /** Shall messages be sent quoted-printable encoded. No encoding
    happens otherwise. */
  bool sendQuotedPrintable() const { return mSendQuotedPrintable; }
  void setSendQuotedPrintable(bool);

public:
  /** Read configuration from global config. */
  void readConfig();

  /** Write configuration to global config with optional sync() */
  void writeConfig(bool withSync=true);

private:
  /** sets a status msg and emits statusMsg() */
  void setStatusMsg(const QString&);

  /** sets replied/forwarded status in the linked message for @p aMsg. */
  void setStatusByLink(const KMMessage *aMsg);

private slots:
  void slotResult( KJob* job );

  /** This slot should be called when the mail sending progress changes.
      It updates the progressbar. */
  void slotProcessedSize( KJob *job, qulonglong size );

  /** abort sending of the current message */
  void slotAbortSend();

  /** note when a msg gets added to outbox during sending */
  void outboxMsgAdded(int idx);

private:
  /** handle sending of messages */
  void doSendMsg();

  /** cleanup after sending */
  void cleanup();

  /** Test if all required settings are set.
      Reports problems to user via dialogs and returns false.
      Returns true if everything is Ok. */
  bool settingsOk() const;

private:
  bool mSendImmediate;
  bool mSendQuotedPrintable;

  MailTransport::TransportJob* mTransportJob;

  QString mCustomTransport;
  bool mSentOk, mSendAborted;
  QString mErrorMsg;
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

#endif /*kmsender_h*/
