// -*- c++ -*-
#ifndef KMAcctExpPop_h
#define KMAcctExpPop_h

#include "networkaccount.h"

#include <qvaluelist.h>
#include <qstringlist.h>
#include <qtimer.h>

class KMPopHeaders;
class KMMessage;
class QDataStream;
namespace KIO {
  class MetaData;
  class Slave;
  class SimpleJob;
  class Job;
}

/** KMail account for experimental pop mail account
 *
 */
class KMAcctExpPop: public KMail::NetworkAccount {
  Q_OBJECT

public:
  typedef KMail::NetworkAccount base;

  virtual ~KMAcctExpPop();
  virtual void init(void);

  virtual KIO::MetaData slaveConfig() const;

  /** A weak assignment operator */
  virtual void pseudoAssign( const KMAccount * a );

  virtual QString protocol() const;
  virtual unsigned short int defaultPort() const;

  /**
   * Sending of several commands at once
   */
  bool usePipelining(void) const { return mUsePipelining; }
  virtual void setUsePipelining(bool);

  /**
   * Shall messages be left on the server upon retreival (TRUE)
   * or deleted (FALSE).
   */
  bool leaveOnServer(void) const { return mLeaveOnServer; }
  virtual void setLeaveOnServer(bool);

  /**
   * Shall messages be filter on the server (TRUE)
   * or not (FALSE).
   */
  bool filterOnServer(void) const { return mFilterOnServer; }
  virtual void setFilterOnServer(bool);

  /**
   * Size of messages which should be check on the
   * pop server before download
   */
  unsigned int filterOnServerCheckSize(void) const { return mFilterOnServerCheckSize; }
  virtual void setFilterOnServerCheckSize(unsigned int);

  /**
   * Inherited methods.
   */
  virtual QString type(void) const;
  virtual void readConfig(KConfig&);
  virtual void writeConfig(KConfig&);
  virtual void processNewMail(bool _interactive);

  virtual void killAllJobs( bool disconnectSlave=false ); // NOOP currently

protected:
  enum Stage { Idle, List, Uidl, Head, Retr, Dele, Quit };
  friend class KMAcctMgr;
  friend class KMPasswdDialog;
  KMAcctExpPop(KMAcctMgr* owner, const QString& accountName);

  /**
   * Start a KIO Job to get a list of messages on the pop server
   */
  void startJob();

  /**
   * Connect up the standard signals/slots for the KIO Jobs
   */
  void connectJob();

  /**
   * Process any queued messages and save the list of seen uids
   * for this user/server
   */
  void processRemainingQueuedMessagesAndSaveUidList();

  bool    mUsePipelining;
  bool    mLeaveOnServer;
  bool    gotMsgs;
  bool    mFilterOnServer;
  unsigned int mFilterOnServerCheckSize;

  KIO::SimpleJob *job;
  QStringList idsOfMsgsPendingDownload; //ID of messages which should be downloaded
  QValueList<int> lensOfMsgsPendingDownload; //length of messages which should be downloaded

  QPtrList<KMPopHeaders> headersOnServer;
  QPtrListIterator<KMPopHeaders> headerIt;
  bool headers;
  QStringList headerDeleteUids;
  QStringList headerDownUids;
  QStringList headerLaterUids;


  QStringList idsOfMsgs; //used for ids and for count
  QValueList<int> lensOfMsgs;
  QStringList uidsOfMsgs;
  QStringList uidsOfSeenMsgs; //UIDS of seen messages, saved in config
  QStringList uidsOfNextSeenMsgs; //UIDS of new seen messages, for writing in the config file
  QStringList idsOfMsgsToDelete;
  int indexOfCurrentMsg;

  QValueList<KMMessage*> msgsAwaitingProcessing;
  QStringList msgIdsAwaitingProcessing;
  QStringList msgUidsAwaitingProcessing;

  QByteArray curMsgData;
  QDataStream *curMsgStrm;

  int curMsgLen;
  int stage;
  QTimer processMsgsTimer;
  QTimer *ss;
  int processingDelay;
  int numMsgs, numBytes, numBytesToRead, numBytesRead, numMsgBytesRead;
  bool interactive;
  bool mProcessing;
  bool mUidlFinished;
  int dataCounter;

protected slots:
  /**
   * Messages are downloaded in the background and then once every x seconds
   * a batch of messages are processed. Messages are processed in batches to
   * reduce flicker (multiple refreshes of the qlistview of messages headers
   * in a single second causes flicker) when using a fast pop server such as
   * one on a lan.
   *
   * Processing a message means applying KMAccount::processNewMsg to it and
   * adding its UID to the list of seen UIDs
   */
  void slotProcessPendingMsgs();

  /**
   * If there are more messages to be downloaded then start a new kio job
   * to get the message whose id is at the head of the queue
   */
  void slotGetNextMsg();

  /**
   * A messages has been retrieved successfully. The next data belongs to the
   * next message.
   */
  void slotMsgRetrieved(KIO::Job*, const QString &);

  /**
   * New data has arrived append it to the end of the current message
   */
  void slotData( KIO::Job*, const QByteArray &);

  /**
   * Finished downloading the current kio job, either due to an error
   * or because the job has been cancelled or because the complete message
   * has been downloaded
   */
  void slotResult( KIO::Job* );

  /**
   * Cleans up after a user cancels the current job
   */
  void slotCancel();

  /**
   * Kills the job if still stage == List
   */
  void slotAbortRequested();

  /**
   * Called when a job is finished. Basically a finite state machine for
   * cycling through the Idle, List, Uidl, Retr, Quit stages
   */
  void slotJobFinished();

  /**
   * Slave error handling
   */
  void slotSlaveError(KIO::Slave *, int, const QString &);

  /**
   * If there are more headers to be downloaded then start a new kio job
   * to get the next header
   */
  void slotGetNextHdr();
};



#endif /*KMAcctExpPop_h*/
