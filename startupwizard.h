/*
    This file is part of KMail.

    Copyright (c) 2003 - 2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef STARTUPWIZARD_H
#define STARTUPWIZARD_H

#include <qwizard.h>

class KMFolder;
class KMFolderComboBox;
class KMAcctCachedImap;
class NetworkPage;
namespace KPIM { class Identity; }
class KMTransportInfo;

class QLabel;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QButtonGroup;
class QTextBrowser;
class KIntNumInput;

class WizardIdentityPage : public QWidget {
  Q_OBJECT
public:
  WizardIdentityPage( QWidget *parent, const char *name );

  void apply() const;

  KPIM::Identity &identity() const;

private:
  int mIdentity;

  QLineEdit *nameEdit, *orgEdit, *emailEdit;
};


class WizardKolabPage : public QWidget {
  Q_OBJECT
public:
  WizardKolabPage( QWidget * parent, const char * name );

  void apply();
  void init( const QString &userEmail );
  KMFolder *folder() const { return mFolder; }

  QLineEdit    *loginEdit;
  QLineEdit    *passwordEdit;
  QLineEdit    *hostEdit;
  QCheckBox    *storePasswordCheck;
  QCheckBox    *excludeCheck;
  QCheckBox    *intervalCheck;
  QLabel       *intervalLabel;
  KIntNumInput *intervalSpin;

  KMFolder *mFolder;
  KMAcctCachedImap *mAccount;
  KMTransportInfo *mTransport;
};


class StartupWizard : public QWizard {
  Q_OBJECT
public:
  // Call this to execute the thing
  static void run();

private slots:
  virtual void back();
  virtual void next();

  void slotGroupwareEnabled( int );
  void slotServerSettings( int i );
  void slotUpdateParentFolderName();

private:
  StartupWizard( QWidget* parent = 0, const char* name = 0, bool modal = FALSE );

  int language() const;
  KMFolder* folder() const;

  bool groupwareEnabled() const { return mGroupwareEnabled; }
  bool useDefaultKolabSettings() const { return mUseDefaultKolabSettings; }

  QString name() const;
  QString login() const;
  QString host() const;
  QString email() const;
  QString passwd() const;
  bool storePasswd() const;

  void setAppropriatePages();
  void guessExistingFolderLanguage();
  void setLanguage( int, bool );

  // Write the KOrganizer settings
  static void writeKOrganizerConfig( const StartupWizard& );

  // Write the KABC settings
  static void writeKAbcConfig();

  // Write the KAddressbook settings
  static void writeKAddressbookConfig( const StartupWizard& );

  KPIM::Identity& userIdentity();
  const KPIM::Identity& userIdentity() const;

  QWidget* createIntroPage();
  QWidget* createIdentityPage();
  QWidget* createKolabPage();
  QWidget* createAccountPage();
  QWidget* createLanguagePage();
  QWidget* createFolderSelectionPage();
  QWidget* createFolderCreationPage();
  QWidget* createOutroPage();

  QWidget *mIntroPage, *mIdentityPage, *mKolabPage, *mAccountPage, *mLanguagePage,
    *mFolderSelectionPage, *mFolderCreationPage, *mOutroPage;

  QComboBox* mLanguageCombo;
  KMFolderComboBox* mFolderCombo;
  QTextBrowser* mFolderCreationText;
  QLabel* mLanguageLabel;

  WizardIdentityPage* mIdentityWidget;
  WizardKolabPage* mKolabWidget;
  NetworkPage* mAccountWidget;

  QButtonGroup *serverSettings;

  bool mGroupwareEnabled;
  bool mUseDefaultKolabSettings;

  KMFolder *mFolder;
};


#endif // STARTUPWIZARD_H
