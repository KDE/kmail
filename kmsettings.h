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
class QGridLayout;
class QBoxLayout;
class KTabListBox;

class KMSettings : public QTabDialog
{
  Q_OBJECT

public:
  KMSettings(QWidget *parent=0,const char *name=0);
  ~KMSettings();

protected:
  virtual void createTabIdentity(QWidget*);
  virtual void createTabNetwork(QWidget*);

  // Create an input field with a label and optional detail button ("...")
  // The detail button is not created if detail_return is NULL.
  // The argument 'label' is the label that will be translated via NLS.
  // The argument 'text' is the text that will show up in the entry field.
  // The whole thing is placed in the grid from row/col to the right.
  virtual QLineEdit* createLabeledEntry(QWidget* parent, QGridLayout* grid,
					const char* label, 
					const char* text,
					int row, int col, 
					QPushButton** detail_return=NULL);

  // Create a button in given grid. The name is internationalized.
  virtual QPushButton* createPushButton(QWidget* parent, QGridLayout* grid,
					const char* label, 
					int row, int col);

  // Adds given account to given listbox after given index or at the end
  // if none is specified.
  void tabNetworkAddAcct(KTabListBox*, KMAccount*, int idx=-1);

protected slots:
  virtual void done(int r);

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
  QWidget *identityTab,*networkTab;
  QLineEdit *nameEdit,*orgEdit,*emailEdit,*replytoEdit,*sigEdit;
  QLineEdit *smtpServerEdit,*smtpPortEdit,*sendmailLocationEdit;
  QRadioButton *smtpRadio,*sendmailRadio;
  QButtonGroup *incomingGroup,*outgoingGroup;
  KTabListBox *accountList;
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

