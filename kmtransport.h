/**
 * kmtransport.h
 *
 * Copyright (c) 2001-2002 Michael Haeckel <haeckel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _KMTRANSPORT_H_
#define _KMTRANSPORT_H_
 
#include <kdialogbase.h>

class QCheckBox;
class QLabel;
class QLineEdit;
class QRadioButton;
class KMServerTest;
class QButtonGroup;

class KMTransportInfo : public QObject
{
public:
  KMTransportInfo();
  virtual ~KMTransportInfo();
  void readConfig(int id);
  void writeConfig(int id);
  static int findTransport(const QString &name);
  static QStringList availableTransports();
  QString type, name, host, port, user, pass, precommand, encryption, authType;
  QString localHostname;
  bool auth, storePass, specifyHostname;
};

class KMTransportSelDlg : public KDialogBase
{
  Q_OBJECT
 
public:
  KMTransportSelDlg( QWidget *parent=0, const char *name=0, bool modal=TRUE );
  int selected() const;
 
private slots:
  void buttonClicked( int id );
 
private:
  int mSelectedButton;
};

class KMTransportDialog : public KDialogBase
{
  Q_OBJECT

public:
  KMTransportDialog( const QString & caption, KMTransportInfo *transportInfo,
		     QWidget *parent=0, const char *name=0, bool modal=TRUE );
  virtual ~KMTransportDialog();

private slots:
  virtual void slotOk();
  void slotSendmailChooser();
  void slotRequiresAuthClicked();
  void slotSmtpEncryptionChanged(int);
  void slotCheckSmtpCapabilities();
  void slotSmtpCapabilities(const QStringList &);
  void slotSendmailEditPath(const QString &);
private:
  struct SendmailWidgets
  {
    QLabel       *titleLabel;
    QLineEdit    *nameEdit;
    QLineEdit    *locationEdit;
    QPushButton  *chooseButton;
  };
  struct SmtpWidgets
  {
    QLabel       *titleLabel;
    QLineEdit    *nameEdit;
    QLineEdit    *hostEdit;
    QLineEdit    *portEdit;
    QCheckBox    *authCheck;
    QLabel       *loginLabel;
    QLineEdit    *loginEdit;
    QLabel       *passwordLabel;
    QLineEdit    *passwordEdit;
    QLineEdit    *precommand;
    QButtonGroup *encryptionGroup;
    QRadioButton *encryptionNone;
    QRadioButton *encryptionSSL;
    QRadioButton *encryptionTLS;
    QButtonGroup *authGroup;
    QRadioButton *authPlain;
    QRadioButton *authLogin;
    QRadioButton *authCramMd5;
    QRadioButton *authDigestMd5;
    QPushButton  *checkCapabilities;
    QCheckBox    *storePasswordCheck;
    QCheckBox    *specifyHostnameCheck;
    QLineEdit    *localHostnameEdit;
  };

  void makeSendmailPage();
  void makeSmtpPage();
  void setupSettings();
  void saveSettings();
  void checkHighest(QButtonGroup *);

  KMServerTest    *mServerTest;
  SmtpWidgets     mSmtp;
  SendmailWidgets mSendmail;
  KMTransportInfo *mTransportInfo;
};


#endif
