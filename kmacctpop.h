/* KMail account for pop mail folders
 *
 */
#ifndef kmacctpop_h
#define kmacctpop_h

#include "kmaccount.h"

class QLineEdit;
class QPushButton;

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
  virtual void setProtocol(short);

  virtual const char* type(void) const;
  virtual void readConfig(void);
  virtual void writeConfig(void);
  virtual bool processNewMail(void);

  
protected:
  KMAcctPop(KMAcctMgr* owner, const char* accountName);
  friend class KMAcctMgr;
  friend class KMPasswdDialog;

  QString mLogin, mPasswd;
  QString mHost;
  int     mPort;
  short   mProtocol;
  bool    mStorePasswd;
};


class KMPasswdDialog : public QDialog
{
  Q_OBJECT

public:
  KMPasswdDialog(QWidget *p=0, const char *n=0, const char *m="",
		 const char *login="", const char *pass = "");
private:
  QLineEdit *usernameLEdit;
  QLineEdit *passwdLEdit;
  QPushButton *ok;
  QPushButton *cancel;

private slots:
  void slotOkPressed();
  void slotCancelPressed();

protected:
};

#endif /*kmacctpop_h*/
