/*******************************************************************************
**
** Filename   : accountwizard.h
** Created on : 07 February, 2005
** Copyright  : (c) 2005 Tobias Koenig
** Email      : tokoe@kde.org
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
*******************************************************************************/

#ifndef KMWIZARD_H
#define KMWIZARD_H

#include <KAssistantDialog>

#include <QLabel>
#include <QList>

class KLineEdit;
class QCheckBox;
class QLabel;
class QPushButton;

class KMAccount;
class KMKernel;
class AccountTypeBox;
class KMTransportInfo;

namespace KPIM {
class ServerTest;
}

class AccountWizard : public KAssistantDialog
{
  Q_OBJECT

  public:
    /**
      Starts the wizard. The wizard is only shown when it has not be
      run successfully before.

      @param kernel The mail kernel the wizard should work on.
      @param parent The parent widget of the dialog.
     */
    static void start( KMKernel *kernel, QWidget *parent = 0 );

    /**
      Reimplemented
     */
    //void setCurrentPage( KPageWidgetItem *page );

  protected:
    AccountWizard( KMKernel *kernel, QWidget *parent );
    ~AccountWizard();

    void setupWelcomePage();
    void setupAccountTypePage();
    void setupAccountInformationPage();
    void setupLoginInformationPage();
    void setupServerInformationPage();

  protected slots:
    void chooseLocation();
    virtual void accept();
    void createTransport();
    void transportCreated();
    void createAccount();
    void accountCreated();
    void finished();

  private slots:
    void slotCurrentPageChanged( KPageWidgetItem *current );

    void popCapabilities( const QStringList&, const QStringList& );
    void imapCapabilities( const QStringList&, const QStringList& );
    void smtpCapabilities( const QStringList&, const QStringList&,
                           const QString&, const QString&, const QString& );

  private:
    QString accountName() const;
    QLabel *createInfoLabel( const QString &msg );

    void checkPopCapabilities( const QString&, int );
    void checkImapCapabilities( const QString&, int );
    void checkSmtpCapabilities( const QString&, int );
    uint popCapabilitiesFromStringList( const QStringList& );
    uint imapCapabilitiesFromStringList( const QStringList& );
    uint authMethodsFromString( const QString& );
    uint authMethodsFromStringList( const QStringList& );

    KPageWidgetItem *mWelcomePage;

    KPageWidgetItem *mAccountTypePage;
    AccountTypeBox *mTypeBox;

    KPageWidgetItem *mAccountInformationPage;
    KLineEdit *mRealName;
    KLineEdit *mEMailAddress;
    KLineEdit *mOrganization;

    KPageWidgetItem *mLoginInformationPage;
    KLineEdit *mLoginName;
    KLineEdit *mPassword;

    KPageWidgetItem *mServerInformationPage;
    QLabel *mIncomingLabel;
    KLineEdit *mIncomingServer;
    QCheckBox *mIncomingUseSSL;
    KLineEdit *mIncomingLocation;

    QPushButton *mChooseLocation;
    KLineEdit *mOutgoingServer;
    QCheckBox *mOutgoingUseSSL;
    QCheckBox *mLocalDelivery;

    QWidget *mIncomingServerWdg;
    QWidget *mIncomingLocationWdg;

    QLabel *mAuthInfoLabel;

    KMKernel *mKernel;
    KMAccount *mAccount;
    KMTransportInfo *mTransportInfo;
    QList<KMTransportInfo*> mTransportInfoList;
    KPIM::ServerTest *mServerTest;
};

#endif
