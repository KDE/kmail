/* KMail Addressbook selection and edit dialog classes
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmaddrbookdlg_h
#define kmaddrbookdlg_h

#include <qdialog.h>
#include <qpushbt.h>
#include <qlistbox.h>
#include <qlined.h>
#include <qlayout.h>

class KMAddrBook;

#define KMAddrBookSelDlgInherited QDialog
class KMAddrBookSelDlg: public QDialog
{
  Q_OBJECT
public:
  KMAddrBookSelDlg(KMAddrBook* addrBook, const char* caption);
  virtual ~KMAddrBookSelDlg();

  /** returns selected address or NULL if none was selected or the cancel
    button was pressed. */
  virtual const QString address(void) const { return mAddress; }

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

#define KMAddrBookEditDlgInherited QDialog
class KMAddrBookEditDlg: public QDialog
{
  Q_OBJECT
public:
  KMAddrBookEditDlg(KMAddrBook* addrBook, const char* caption);
  virtual ~KMAddrBookEditDlg();

protected slots:
  void slotOk();
  void slotCancel();
  void slotAdd();
  void slotRemove();
  void slotLbxHighlighted(const char* item);

protected:
  KMAddrBook* mAddrBook;
  QGridLayout mGrid;
  QListBox mListBox;
  QLineEdit mEdtAddress;
  QPushButton mBtnOk, mBtnCancel, mBtnAdd, mBtnRemove;
  int mIndex;
};


#endif /*kmaddrbookdlg_h*/
