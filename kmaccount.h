/* KMail Account: Abstract base class for accounts.
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmaccount_h
#define kmaccount_h

#include <qstring.h>
#include <qlist.h>
#include <qtimer.h>
#include <qsignal.h>
#include "kmnewiostatuswdg.h"

// The defualt check interval
#define DEFAULT_CK_INT 5

class KMAcctMgr;
class KMFolder;
class KMAcctFolder;
class KConfig;
class KMMessage;

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

  /** Set intelligent default values to the fields of the account. */
  virtual void init(void) = 0;

  /** There can be exactly one folder that is fed by messages from an
    account. */
  KMFolder* folder(void) const { return ((KMFolder*)mFolder); }
  virtual void setFolder(KMFolder*);

  /** Process new mail for this account if one arrived. Returns TRUE if new
    mail has been found. Whether the mail is automatically loaded to
    an associated folder or not depends on the type of the account. */
  virtual bool processNewMail(KMIOStatus *) = 0;

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

protected slots:
  virtual void mailCheck();

protected:
  KMAccount(KMAcctMgr* owner, const char* accountName);

  /** Does filtering and storing in a folder for the given message.
    Shall be called from within processNewMail() to process the new
    messages. Returns false if failed to add new message. */
  virtual bool processNewMsg(KMMessage* msg);

  /** Send receipt of message back to sender (confirming delivery). */
  virtual void sendReceipt(KMMessage* msg, const QString receiptTo) const;

  /** Install/deinstall automatic new-mail checker timer. */
  virtual void installTimer();
  virtual void deinstallTimer();

protected:
  QString       mName;
  KMAcctMgr*    mOwner;
  KMAcctFolder* mFolder;
  QTimer *mTimer;
  int mInterval;
  bool mExclude;
  bool mCheckingMail;
};


class KMAcctList: public QList<KMAccount>
{
public:
  virtual ~KMAcctList() {}
  short _dummy; // some compilers fail otherwise
};

#endif /*kmaccount_h*/
