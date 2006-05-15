/* KMail Mail Sender
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmsender_h
#define kmsender_h
#include "messagesender.h"

#ifndef KDE_USE_FINAL
# ifndef REALLY_WANT_KMSENDER
#  error Do not include kmsender.h, but messagesender.h.
# endif
#endif

#include <qcstring.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>
#include <kdeversion.h>

class KMMessage;
class KMFolder;
class KMFolderMgr;
class KConfig;
class KProcess;
class KMSendProc;
class KMSendSendmail;
class KMSendSMTP;
class QStrList;
class KMTransportInfo;
class KMPrecommand;

namespace KPIM {
  class ProgressItem;
}

class KMSender: public QObject, public KMail::MessageSender
{
  Q_OBJECT
  friend class KMSendProc;
  friend class KMSendSendmail;
  friend class KMSendSMTP;

public:
  KMSender();
  ~KMSender();

protected:
  /** Send given message. The message is either queued or sent
    immediately. The default behaviour, as selected with
    setSendImmediate(), can be overwritten with the parameter
    sendNow (by specifying TRUE or FALSE).
    The sender takes ownership of the given message on success,
    so DO NOT DELETE OR MODIFY the message further.
    Returns TRUE on success. */
  bool doSend(KMMessage* msg, short sendNow);

public:
  /** Start sending all queued messages. Returns TRUE on success. */
  bool sendQueued(); // used by KMMainWidget

private:
  /** Returns TRUE if sending is in progress. */
  bool sending(void) const { return mSendInProgress; }

public:
  /** Shall messages be sent immediately (TRUE), or shall they be
    queued and sent later upon call of sendQueued() ? */
  bool sendImmediate(void) const { return mSendImmediate; }
  void setSendImmediate(bool);

  /** Shall messages be sent quoted-printable encoded. No encoding
    happens otherwise. */
  bool sendQuotedPrintable(void) const { return mSendQuotedPrintable; }
  void setSendQuotedPrintable(bool);

private:
  /** Get the transport information */
  KMTransportInfo * transportInfo() { return mTransportInfo; }

public:
  /** Read configuration from global config. */
  void readConfig(void);

  /** Write configuration to global config with optional sync() */
  void writeConfig(bool withSync=TRUE);

private:
  /** sets a status msg and emits statusMsg() */
  void setStatusMsg(const QString&);

  /** sets replied/forwarded status in the linked message for @p aMsg. */
  void setStatusByLink(const KMMessage *aMsg);

  /** Emit progress info - calculates a percent value based on the amount of bytes sent */
  void emitProgressInfo( int currentFileProgress );

private slots:
  /** Start sending */
  void slotPrecommandFinished(bool);

  void slotIdle();

  /** abort sending of the current message */
  void slotAbortSend();

  /** initialization sequence has finised */
  void sendProcStarted(bool success);

  /** note when a msg gets added to outbox during sending */
  void outboxMsgAdded(int idx);

private:
  /** handle sending of messages */
  void doSendMsg();

  /** second part of handling sending of messages */
  void doSendMsgAux();

  /** cleanup after sending */
  void cleanup(void);

  /** Test if all required settings are set.
      Reports problems to user via dialogs and returns FALSE.
      Returns TRUE if everything is Ok. */
  bool settingsOk(void) const;

  /** Parse protocol '://' (host port? | mailer) string and
      set transport settings */
  KMSendProc* createSendProcFromString(QString transport);

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

#endif /*kmsender_h*/
