#ifndef __KMSETTINGS
#define __KMSETTINGS

#include <qtabdialog.h>
#include <qstring.h>
#include <qcheckbox.h>

class KMAccount;
class KMAccountSettings;
class KTabListBox;
class KColorButton;
class QGridLayout;
class QBoxLayout;
class QLineEdit;
class QButtonGroup;
class QRadioButton;
class QPushButton;
class QComboBox;
class QFileDialog;
class QLabel;
class KpgpConfig;

#define DEFAULT_EDITOR_STR       "kedit %f"

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
  virtual void createTabMisc(QWidget*);
  virtual void createTabAppearance(QWidget*);
  virtual void createTabPgp(QWidget *parent);

  // Create a button in given grid. The name is internationalized.
  virtual QPushButton* createPushButton(QWidget* parent, QGridLayout* grid,
					const QString& label,
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
  void slotSendNow();
  void slotSendLater();
  void slotAllow8Bit();
  void slotQuotedPrintable();
  void slotDefaultFontSelect();
  void slotBodyFontSelect();
  void slotListFontSelect();
  void slotFolderlistFontSelect();
  void slotDefaultColorSelect();
  void slotSigModify();

private:
  QLineEdit *nameEdit,*orgEdit,*emailEdit,*replytoEdit,*sigEdit;
  QLineEdit *smtpServerEdit,*smtpPortEdit,*sendmailLocationEdit;
  QLineEdit *phraseReplyEdit, *phraseReplyAllEdit, *phraseForwardEdit;
  QLineEdit *indentPrefixEdit, *wrapColumnEdit, *extEditorEdit;
  QCheckBox *autoAppSignFile, *wordWrap, *monospFont, *pgpAutoSign, *smartQuote;
  QCheckBox *emptyTrashOnExit, *sendOnCheck, *longFolderList, *sendReceipts,
    *compactOnExit, *useExternalEditor;
  QRadioButton *smtpRadio, *sendmailRadio, *sendNow, *sendLater;
  QRadioButton *allow8Bit, *quotedPrintable;
  QButtonGroup *incomingGroup,*outgoingGroup;
  KTabListBox *accountList;
  QPushButton *addButton,*modifyButton,*removeButton;
  QCheckBox *defaultFonts, *defaultColors;
  QLabel *bodyFontLabel, *listFontLabel, *folderListFontLabel;
  QLabel *bodyFontLabel2, *listFontLabel2, *folderListFontLabel2;
  QPushButton *bodyFontButton, *listFontButton, *folderListFontButton;
  QPushButton *sigModify;
  QColor cFore, cBack, cNew, cUnread;
  KColorButton *foregroundColorBtn, *backgroundColorBtn, *newColorBtn, 
    *unreadColorBtn;
  QLabel *foregroundColorLbl, *backgroundColorLbl, *newColorLbl, 
    *unreadColorLbl;
  KpgpConfig *pgpConfig;
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
  void slotIntervalChange();

private:
  QLineEdit *mEdtName, *mEdtLocation, *mEdtLogin, *mEdtPasswd, *mEdtHost;
  QLineEdit *mEdtPort, *mChkInt;
  QComboBox *mFolders;
  KMAccount *mAcct;
  QCheckBox *mStorePasswd, *mChkDelete, *mChkInterval, *mChkExclude;
  QCheckBox *mChkRetrieveAll;
};

#endif

