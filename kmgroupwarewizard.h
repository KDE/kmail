#ifndef KMGROUPWAREWIZARD_H
#define KMGROUPWAREWIZARD_H

#include <qwizard.h>

class KMFolder;
class KMFolderComboBox;
class KMAcctCachedImap;
class NetworkPage;
class KMIdentity;
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

  KMIdentity &identity() const;

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

private:
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


class KMGroupwareWizard : public QWizard {
  Q_OBJECT
public:
  KMGroupwareWizard( QWidget* parent = 0, const char* name = 0, bool modal = FALSE );

  int language() const;
  KMFolder* folder() const;

  bool groupwareEnabled() const { return mGroupwareEnabled; }
  bool useDefaultKolabSettings() const { return mUseDefaultKolabSettings; }

protected slots:
  virtual void back();
  virtual void next();
private slots:
  void slotGroupwareEnabled( int );
  void slotServerSettings( int i );
  void slotUpdateParentFolderName();
private:
  void setAppropriatePages();
  void guessExistingFolderLanguage();
  void setLanguage( int, bool );
  KMIdentity &userIdentity();

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


#endif // KMGROUPWAREWIZARD_H
