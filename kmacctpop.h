/* KMail account for pop mail folders
 *
 */
#ifndef kmacctpop_h
#define kmacctpop_h

#include "kmaccount.h"
#include <kapp.h>
class QLineEdit;
class QPushButton;
class DwPopClient;
class KApplication;

#define KMAcctPopInherited KMAccount

class KMAcctPop: public KMAccount
{
  Q_OBJECT

public:
  virtual ~KMAcctPop();
  virtual void init(void);

  /** Pop user login name */
  const QString& login(void) const { return mLogin; }
  virtual void setLogin(const QString&);

  /** Pop user password */
  const QString& passwd(void) const { return mPasswd; }
  virtual void setPasswd(const QString&, bool storeInConfig=FALSE);

  /** Will the password be stored in the config file ? */
  bool storePasswd(void) const { return mStorePasswd; }

  /** Pop host */
  const QString& host(void) const { return mHost; }
  virtual void setHost(const QString&);

  /** Port on pop host */
  int port(void) { return mPort; }
  virtual void setPort(int);

  /** Pop protocol: shall be 2 or 3 */
  short protocol(void) { return mProtocol; }
  virtual bool setProtocol(short);

  /** Shall messages be left on the server upon retreival (TRUE) 
    or deleted (FALSE). */
  bool leaveOnServer(void) const { return mLeaveOnServer; }
  virtual void setLeaveOnServer(bool);

  /** Retrieve all messages from server (TRUE) or unread messages only. */
  bool retrieveAll(void) const { return mRetrieveAll; }
  virtual void setRetrieveAll(bool);

  /** Inherited methods. */
  virtual const char* type(void) const;
  virtual void readConfig(KConfig&);
  virtual void writeConfig(KConfig&);
  virtual bool processNewMail(KMIOStatus *);

  
protected:
  friend class KMAcctMgr;
  friend class KMPasswdDialog;
  KMAcctPop(KMAcctMgr* owner, const char* accountName);

  /** Very primitive en/de-cryption so that the password is not
      readable in the config file. But still very easy breakable. */
  const QString encryptStr(const QString inStr);
  const QString decryptStr(const QString inStr);

  /** Mail processing main worker method. */
  virtual bool doProcessNewMail(KMIOStatus *);

  /** Display POP error message. Always returns FALSE to simplify the
    code in doProcessNewMail(). */
  virtual bool popError(const QString stage, DwPopClient&) const;

  QString mLogin, mPasswd;
  QString mHost;
  int     mPort;
  short   mProtocol;
  bool    mStorePasswd;
  bool    mLeaveOnServer;
  bool    mRetrieveAll;
  bool    gotMsgs;
};


class KMPasswdDialog : public QDialog
{
  Q_OBJECT

public:
  KMPasswdDialog(QWidget *parent = NULL,const char *name= NULL,
		 KMAcctPop *act=NULL, const char *caption=NULL,
		 const char *login=NULL, QString passwd=NULL);
private:
  QLineEdit *usernameLEdit;
  QLineEdit *passwdLEdit;
  QPushButton *ok;
  QPushButton *cancel;
  KMAcctPop *act;

private slots:
  void slotOkPressed();
  void slotCancelPressed();

protected:
};

#endif /*kmacctpop_h*/
