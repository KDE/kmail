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

class KMAccountSettings : public QDialog {
		Q_OBJECT
	public:
		KMAccountSettings(QWidget *parent=0,const char *name=0,KMAccount *a=NULL);
	private:
		QWidget *local,*remote;
		QLineEdit *nameEdit,*locationEdit,*hostEdit,*portEdit,*mailboxEdit,*loginEdit,*passEdit;
		QComboBox *typeList;
		QRadioButton *accessMethod1,*accessMethod2,*accessMethod3;
		KMAccount *account;
		void changeType(int);
	private slots:
		void chooseLocation();
		void typeSelected(int);
	protected slots:
		void accept();
};

class KMSettings : public QTabDialog {
		Q_OBJECT
	public:
		KMSettings(QWidget *parent=0,const char *name=0);
		~KMSettings();
	private:
		QWidget *identityTab,*networkTab;
		QLineEdit *nameEdit,*orgEdit,*emailEdit,*replytoEdit,*sigEdit;
		QLineEdit *smtpServerEdit,*smtpPortEdit,*sendmailLocationEdit;
		QRadioButton *smtpRadio,*sendmailRadio;
		QButtonGroup *incomingGroup,*outgoingGroup;
		QListBox *accountList;
		QPushButton *addButton,*modifyButton,*removeButton;
		KConfig *config;
	private slots:
		void accountSelected(int);
		void addAccount();
		void chooseSendmailLocation();
		void chooseSigFile();
		void modifyAccount(int);
		void modifyAccount2();
		void removeAccount();
		void setDefaults();
	protected slots:
		virtual void done(int r);
};

#endif

