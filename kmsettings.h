#ifndef __KMSETTINGS
#define __KMSETTINGS

#include <qtabdlg.h>
#include <qwidget.h>
#include <qlined.h>
#include <qbttngrp.h>
#include <qradiobt.h>
#include <qlistbox.h>
#include <qcombo.h>
#include <qpushbt.h>
#include <kapp.h>

class KMAccount;
class KMAccountSettings;

class KMSettings : public QTabDialog
{
  Q_OBJECT

public:
  KMSettings(QWidget *parent=0,const char *name=0);
  ~KMSettings();

protected slots:
  virtual void done(int r);

private slots:
  void accountSelected(int);
  void addAccount();
  void chooseSendmailLocation();
  void chooseSigFile();
  void modifyAccount(int);
  void modifyAccount2();
  void removeAccount();
  void setDefaults();

private:
  QWidget *identityTab,*networkTab;
  QLineEdit *nameEdit,*orgEdit,*emailEdit,*replytoEdit,*sigEdit;
  QLineEdit *smtpServerEdit,*smtpPortEdit,*sendmailLocationEdit;
  QRadioButton *smtpRadio,*sendmailRadio;
  QButtonGroup *incomingGroup,*outgoingGroup;
  QListBox *accountList;
  QPushButton *addButton,*modifyButton,*removeButton;
  KConfig *config;
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
  void typeSelected(int);

private:
  void changeType(int);

  QWidget *local,*remote;
  QLineEdit *nameEdit,*locationEdit,*hostEdit,*portEdit,*mailboxEdit;
  QLineEdit *loginEdit,*passEdit;
  QComboBox *typeList;
  QRadioButton *accessMethod1,*accessMethod2,*accessMethod3;
  KMAccount *account;
};

#endif

