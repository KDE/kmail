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
#include <qlineedit.h>
#include <qlayout.h>
#include <qvaluelist.h> // for kab stuff

class KMAddrBook;
class KabBridge;
class QStringList; // for kab stuff
class KabKey; // for kab stuff

#define KMAddrBookSelDlgInherited QDialog
class KMAddrBookSelDlg: public QDialog
{
  Q_OBJECT
public:
  KMAddrBookSelDlg(KMAddrBook* addrBook, const char* caption=NULL);
  virtual ~KMAddrBookSelDlg();

  /** returns selected address(es) or NULL if none was selected or the cancel
    button was pressed. If multiple addresses were selected they
    are returned comma separated. */
  virtual QString address(void) const { return mAddress; }

protected slots:
  void slotOk();
  void slotCancel();

protected:
  KMAddrBook* mAddrBook;
  QGridLayout mGrid;
  QListBox mListBox;
  QPushButton mBtnOk, mBtnCancel;
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
  QLineEdit* mEdtAddress;
  int mIndex;

  //kab specific
  QStringList *mAddresses;
  QValueList<KabKey> *mKeys;
};


#endif /*kmaddrbookdlg_h*/
