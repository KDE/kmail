/* KMail account for experimental pop mail account
 *
 */
#ifndef KMAcctExpPop_h
#define KMAcctExpPop_h

#include "kmaccount.h"
#include <kapp.h>
#include <qdialog.h>
#include "kio/job.h"
#include "kio/global.h"
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qcstring.h>
#include <qtimer.h>

class QLineEdit;
class QPushButton;
class DwPopClient;
class KApplication;
class DwPopClient;
class KMMessage;
class QTimer;
class KURL::List;
class QDataStream;

#define KMAcctExpPopInherited KMAccount

class KMAcctExpPop: public KMAccount
{
  Q_OBJECT

public:
  virtual ~KMAcctExpPop();
  virtual void init(void);

  /** Pop user login name */
  QString login(void) const { return mLogin; }
  virtual void setLogin(const QString&);

  /** Pop user password */
  QString passwd(void) const;
  virtual void setPasswd(const QString&, bool storeInConfig=FALSE);

  /** Set the password to "" (empty string) */
  virtual void clearPasswd();

  /** Use SSL? */
  bool useSSL(void) const { return mUseSSL; }
  virtual void setUseSSL(bool);

  /** Use TLS? */
  bool useTLS(void) const { return mUseTLS; }
  virtual void setUseTLS(bool);

  /** Authentification method */
  QString auth(void) const { return mAuth; }
  virtual void setAuth(const QString &);

  /** Will the password be stored in the config file ? */
  bool storePasswd(void) const { return mStorePasswd; }
  virtual void setStorePasswd(bool);

  /** Pop host */
  const QString& host(void) const { return mHost; }
  virtual void setHost(const QString&);

  /** Port on pop host */
  unsigned short int port(void) { return mPort; }
  virtual void setPort(unsigned short int);

  /** Pop protocol: shall be 2 or 3 */
  short protocol(void) { return mProtocol; }
  virtual bool setProtocol(short);

  /** Shall messages be left on the server upon retreival (TRUE)
    or deleted (FALSE). */
  bool leaveOnServer(void) const { return mLeaveOnServer; }
  virtual void setLeaveOnServer(bool);

  /** Inherited methods. */
  virtual const char* type(void) const;
  virtual void readConfig(KConfig&);
  virtual void writeConfig(KConfig&);
  virtual void processNewMail(bool _interactive);
  virtual void pseudoAssign(KMAccount*);

protected:
  enum Stage { Idle, List, Uidl, Retr, Dele, Quit };
  friend class KMAcctMgr;
  friend class KMPasswdDialog;
  KMAcctExpPop(KMAcctMgr* owner, const QString& accountName);

  /** Very primitive en/de-cryption so that the password is not
      readable in the config file. But still very easy breakable. */
  QString encryptStr(const QString& inStr) const;
  QString decryptStr(const QString& inStr) const;

  /** Start a KIO Job to get a list of messages on the pop server */
  void startJob();

  /** Connect up the standard signals/slots for the KIO Jobs */
  void connectJob();

  /** Get an URL for the account */
  KURL getUrl();

  /** Process any queued messages and save the list of seen uids
      for this user/server */
  void processRemainingQueuedMessagesAndSaveUidList();

  QString mLogin, mPasswd;
  QString mHost;
  unsigned short int mPort;
  short   mProtocol;
  bool    mUseSSL;
  bool    mUseTLS;
  QString mAuth;
  bool    mStorePasswd;
  bool    mLeaveOnServer;
  bool    gotMsgs;

  KIO::SimpleJob *job;
  KIO::Slave *slave;
  KIO::MetaData mSlaveConfig;
  QStringList idsOfMsgsPendingDownload;
  QValueList<int> lensOfMsgsPendingDownload;

  QStringList idsOfMsgs;
  QValueList<int> lensOfMsgs;
  QStringList uidsOfMsgs;
  QStringList uidsOfSeenMsgs;
  QStringList uidsOfNextSeenMsgs;
  KURL::List idsOfMsgsToDelete;
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
  /** Messages are downloaded in the background and then once every x seconds
      a batch of messages are processed. Messages are processed in batches to
      reduce flicker (multiple refreshes of the qlistview of messages headers
      in a single second causes flicker) when using a fast pop server such as
      one on a lan.

      Processing a message means applying KMAccount::processNewMsg to it and
      adding its UID to the list of seen UIDs */
  void slotProcessPendingMsgs();

  /** If there are more messages to be downloaded then start a new kio job
      to get the message whose id is at the head of the queue */
  void slotGetNextMsg();

  /** New data has arrived append it to the end of the current message */
  void slotData( KIO::Job*, const QByteArray &);

  /** Finished downloading the current kio job, either due to an error
      or because the job has been cancelled or because the complete message
      has been downloaded */
  void slotResult( KIO::Job* );

  /** Cleans up after a user cancels the current job */
  void slotCancel();

  /** Kills the job if still stage == List */
  void slotAbortRequested();

  /* Called when a job is finished. Basically a finite state machine for
   cycling through the Idle, List, Uidl, Retr, Quit stages */
  void slotJobFinished();

  /** Slave error handling */
  void slotSlaveError(KIO::Slave *, int, const QString &);
};


//-----------------------------------------------------------------------------
class KMExpPasswdDialog : public QDialog
{
  Q_OBJECT

public:
  KMExpPasswdDialog(QWidget *parent = 0,const char *name= 0,
	  	    KMAcctExpPop *act=0, const QString &caption=QString::null,
		    const QString &login=QString::null,
                    const QString &passwd=QString::null);

private:
  QLineEdit *usernameLEdit;
  QLineEdit *passwdLEdit;
  QPushButton *ok;
  QPushButton *cancel;
  KMAcctExpPop *act;

private slots:
  void slotOkPressed();
  void slotCancelPressed();

};
#endif /*KMAcctExpPop_h*/
