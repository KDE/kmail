/* KMail Addressbook selection and edit dialog classes
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmaddrbookdlg_h
#define kmaddrbookdlg_h

#include <kdialogbase.h>

#include <qlistbox.h>
#include <qvaluelist.h> // for kab stuff

class KMAddrBook;
class KabBridge;
class KLineEdit;
class QCheckBox;
class QStringList; // for kab stuff
class KabKey; // for kab stuff

class KMAddrBookSelDlg: public KDialogBase
{
  Q_OBJECT
public:
  enum { AddressBookAddresses = 1, RecentAddresses };

  KMAddrBookSelDlg(QWidget *parent, const QString& caption=QString::null);
  virtual ~KMAddrBookSelDlg();

  /** returns selected address(es) or 0 if none was selected or the cancel
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

  QListBox   *mListBox;
  QCheckBox  *mCheckBox;
  QString     mAddress;
};


#endif /*kmaddrbookdlg_h*/
