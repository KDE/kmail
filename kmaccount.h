/* KMail Account: Abstract base class for accounts.
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmaccount_h
#define kmaccount_h

#include <qstring.h>
#include <qlist.h>
#include <qvaluelist.h>
#include <qtimer.h>
#include <qsignal.h>
#include <qguardedptr.h>
#include "kmnewiostatuswdg.h"

// The defualt check interval
#define DEFAULT_CK_INT 5

class KMAcctMgr;
class KMFolder;
class KMAcctFolder;
class KConfig;
class KMMessage;
class KProcess;

class KMAccount: public QObject
{
  Q_OBJECT
  friend class KMAcctMgr;

public:
  virtual ~KMAccount();

  /** Returns type of the account */
  virtual const char* type(void) const = 0;

  /** Returns account name */
  const QString& name(void) const { return mName; }
  virtual void setName(const QString&);

  /** Set password to "" (empty string) */
  virtual void clearPasswd();

  /** Set intelligent default values to the fields of the account. */
  virtual void init(void) = 0;

  // A weak assignment operator
  virtual void pseudoAssign(KMAccount*) = 0;

  /** There can be exactly one folder that is fed by messages from an
    account. */
  KMFolder* folder(void) const { return ((KMFolder*)((KMAcctFolder*)mFolder)); }
  virtual void setFolder(KMFolder*);

  /** Process new mail for this account if one arrived. Returns TRUE if new
    mail has been found. Whether the mail is automatically loaded to
    an associated folder or not depends on the type of the account. */
  virtual void processNewMail(bool interactive) = 0;

  /** Read config file entries. This method is called by the account
    manager when a new account is created. */
  virtual void readConfig(KConfig& config);

  /** Write all account information to given config file. The config group
    is already properly set by the caller. */
  virtual void writeConfig(KConfig& config);

  /** Set/get interval for checking if new mail arrived (in minutes).
    An interval of zero (or less) disables the automatic checking. */
  virtual void setCheckInterval(int aInterval);
  int checkInterval(void) const { return mInterval; }

  // This can be used to provide a more complex calculation later if we want
  inline const int defaultCheckInterval(void) const { return DEFAULT_CK_INT; }

  /** Set/get whether account should be part of the accounts checked
    with "Check Mail". */
  virtual void setCheckExclude(bool aExclude);
  int checkExclude(void) const { return mExclude; }

   /** Pre command */
  const QString& precommand(void) const { return mPrecommand; }
  virtual void setPrecommand(const QString &cmd) { mPrecommand = cmd; }

  /** Runs the precommand. If the precommand is empty, the method
   * will just return success and not actually do anything
   *
   * @return True if successful, false otherwise*/
  static bool runPrecommand(const QString &precommand);

signals:
  virtual void finishedCheck(bool newMail);

protected slots:
  virtual void mailCheck();
  virtual void sendReceipts();

protected:
  KMAccount(KMAcctMgr* owner, const QString& accountName);

  /** Does filtering and storing in a folder for the given message.
    Shall be called from within processNewMail() to process the new
    messages. Returns false if failed to add new message. */
  virtual bool processNewMsg(KMMessage* msg);

  /** Send receipt of message back to sender (confirming
      delivery). Checks the config settings, calls
      @ref KMMessage::createDeliveryReceipt and queues the resulting
      message in @p mReceipts. */
  virtual void sendReceipt(KMMessage* msg);

  /** Install/deinstall automatic new-mail checker timer. */
  virtual void installTimer();
  virtual void deinstallTimer();

protected:
  QString       mName;
  QString       mPrecommand;
  KMAcctMgr*    mOwner;
  QGuardedPtr<KMAcctFolder> mFolder;
  QTimer *mTimer, mReceiptTimer;
  int mInterval;
  bool mExclude;
  bool mCheckingMail;
  QValueList<KMMessage*> mReceipts;

private:
    // avoid compiler warning about hidden virtual
    virtual void setName( const char *name ) { QObject::setName( name ); }
};


class KMAcctList: public QList<KMAccount>
{
public:
  virtual ~KMAcctList() {}
  short _dummy; // some compilers fail otherwise
};

class KMAccountPrivate : public QObject
{
    Q_OBJECT

public:
    KMAccountPrivate( QObject *parent = 0);

public slots:
    void precommandExited(KProcess *);
};

#endif /*kmaccount_h*/
