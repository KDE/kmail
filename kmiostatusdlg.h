/* I/O Status Dialog, e.g. for sending and receiving mails
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmiostatusdlg_h
#define kmiostatusdlg_h

#include <qdialog.h>

class QLabel;
class QPushButton;

#define KMIOStatusDlgInherited QDialog
class KMIOStatusDlg: public QDialog
{
  Q_OBJECT
public:
  KMIOStatusDlg(const char* caption);

  virtual void setTask(const QString msg);
  virtual void setStatus(const QString msg);

  // return from exec() loop with result 0
  virtual void done(void);

protected:
  QLabel *mLblTask, *mLblStatus;
  QPushButton *mBtnAbort;
};

#endif /*kmiostatusdlg_h*/
