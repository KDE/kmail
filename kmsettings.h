#ifndef __KMSETTINGS
#define __KMSETTINGS

#include <qtabdlg.h>
#include <qstring.h>
#include <qchkbox.h>

class KMAccount;
class KMAccountSettings;
class QGridLayout;
class QBoxLayout;
class KTabListBox;
class QLineEdit;
class QButtonGroup;
class QRadioButton;
class QPushButton;
class QComboBox;
class QFileDialog;
class Kpgp;

class KMSettings : public QTabDialog
{
  Q_OBJECT

public:
  KMSettings(QWidget *parent=0,const char *name=0);
  ~KMSettings();

protected:
  virtual void createTabIdentity(QWidget*);
  virtual void createTabNetwork(QWidget*);
  virtual void createTabComposer(QWidget*);

  // Create a button in given grid. The name is internationalized.
  virtual QPushButton* createPushButton(QWidget* parent, QGridLayout* grid,
					const char* label, 
					int row, int col);

  // Returns a string suitable for account listbox
  const QString tabNetworkAcctStr(const KMAccount* acct) const;

protected slots:
  void doApply();
  void doCancel();

private slots:
  void accountSelected(int,int);
  void addAccount();
  void chooseSendmailLocation();
  void chooseSigFile();
  void modifyAccount(int,int);
  void modifyAccount2();
  void removeAccount();
  void setDefaults();

private:
  QLineEdit *nameEdit,*orgEdit,*emailEdit,*replytoEdit,*sigEdit;
  QLineEdit *smtpServerEdit,*smtpPortEdit,*sendmailLocationEdit;
  QLineEdit *phraseReplyEdit, *phraseReplyAllEdit, *phraseForwardEdit;
  QLineEdit *indentPrefixEdit, *wrapColumnEdit, *pgpUserEdit;
  QCheckBox *autoAppSignFile, *wordWrap, *monospFont, *pgpAutoSign;
  QRadioButton *smtpRadio,*sendmailRadio;
  QButtonGroup *incomingGroup,*outgoingGroup;
  KTabListBox *accountList;
  QPushButton *addButton,*modifyButton,*removeButton;
  Kpgp* pgp;
};


//-----------------------------------------------------------------------------
class KMAccountSettings : public QDialog
{
  Q_OBJECT

public:
  KMAccountSettings(QWidget *parent=0,const char *name=0,KMAccount *a=NULL);

protected slots:
 void accept();

private slots:
  void chooseLocation();

private:
  QLineEdit *mEdtName, *mEdtLocation, *mEdtLogin, *mEdtPasswd, *mEdtHost;
  QLineEdit *mEdtPort;
  QComboBox *mFolders;
  KMAccount *mAcct;
  QCheckBox *chk;
};

#endif

