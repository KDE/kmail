/* Select account from given list of account types
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmacctseldlg_h
#define kmacctseldlg_h

#include <qdialog.h>

class QButtonGroup;
class QPushButton;

#define KMAcctSelDlgInherited QDialog

class KMAcctSelDlg: public QDialog
{
  Q_OBJECT

public:
  KMAcctSelDlg(QWidget* parent=0, const char* name=0);

  /** Returns selected button from the account selection group:
    0=local mail, 1=pop3, 2=experimental pop3. */
  int selected(void) const { return mSelBtn; }

protected slots:
  void buttonClicked(int);

protected:
  QButtonGroup* grp;
  QPushButton* ok;
  int mSelBtn;
};

#endif /*kmacctseldlg_h*/
