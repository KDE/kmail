/* Small dialog window for viewing/setting message part properties.
 * Author: Stefan Taferner <taferner@kde.org>
 */
#ifndef kmmsgpartdlg_h
#define kmmsgpartdlg_h

#include <kcombobox.h>
#include <qdialog.h>
#include <qpixmap.h>

class KMMessagePart;
class QPushButton;
class QComboBox;
class QLabel;
class QLineEdit;

#define KMMsgPartDlgInherited QDialog
class KMMsgPartDlg: public QDialog
{
  Q_OBJECT

public:
  KMMsgPartDlg(const char* caption=NULL, bool readOnly=FALSE);
  virtual ~KMMsgPartDlg();
  
  /** Display this message part. */
  virtual void setMsgPart(KMMessagePart* msgPart);

  /** Returns message part. */
  virtual KMMessagePart* msgPart(void) const { return mMsgPart; }

protected slots:
  virtual void done(int);
  virtual void mimetypeChanged(const QString & name);

protected:
  /** Applies changes from the dialog to the message part. Called
    when the Ok button is pressed. */
  virtual void applyChanges(void);

  KMMessagePart *mMsgPart;
  QLineEdit *mEdtName, *mEdtComment;
  QComboBox *mCbxEncoding;
  KComboBox *mEdtMimetype;
  QLabel *mLblIcon, *mLblSize;
  QPixmap mIconPixmap;
};


#endif /*kmmsgpartdlg_h*/
