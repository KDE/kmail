/* KMail Addressbook selection and edit dialog classes
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmaddrbookdlg_h
#define kmaddrbookdlg_h

#include <kdialogbase.h>

#include <qdialog.h>
#include <qpushbutton.h>
#include <qlistbox.h>
#include <qlayout.h>
#include <qvaluelist.h> // for kab stuff

class KMAddrBook;
class KabBridge;
class KLineEdit;
class QCheckBox;
class QStringList; // for kab stuff
class KabKey; // for kab stuff

#define KMAddrBookSelDlgInherited QDialog
class KMAddrBookSelDlg: public QDialog
{
  Q_OBJECT
public:
  enum { AddressBookAddresses = 1, RecentAddresses };

  KMAddrBookSelDlg(KMAddrBook* addrBook, const QString& caption=QString::null);
  virtual ~KMAddrBookSelDlg();

  /** returns selected address(es) or NULL if none was selected or the cancel
    button was pressed. If multiple addresses were selected they
    are returned comma separated. */
  virtual QString address(void) const { return mAddress; }

protected slots:
  void slotOk();
  void slotCancel();

  void toggleShowRecent( bool );
  void readConfig();
  void saveConfig();
  /** you can OR AddressBookAddresses and RecentAddresses as @p addressTypes */

protected:
  void showAddresses( int addressTypes );

  KMAddrBook* mAddrBook;
  QGridLayout mGrid;
  QListBox mListBox;
  QPushButton mBtnOk, mBtnCancel;
  QCheckBox *mCheckBox;
  QString mAddress;
};


//-----------------------------------------------------------------------------


class KMAddrBookEditDlg: public KDialogBase
{
  Q_OBJECT

public:
  KMAddrBookEditDlg( KMAddrBook* aAddrBook, QWidget *parent=0,
		     const char *name=0, bool modal=true );

  virtual ~KMAddrBookEditDlg();

protected slots:
  void slotOk();
  void slotCancel();
  void slotEnableAdd();
  void slotAdd();
  void slotEnableRemove();
  void slotRemove();
  void slotLbxHighlighted(const QString& item);

protected:
  KMAddrBook* mAddrBook;
  QListBox* mListBox;
  KLineEdit* mEdtAddress;
  int mIndex;

  //kab specific
  QValueList<KabKey> *mKeys;
};


#endif /*kmaddrbookdlg_h*/
