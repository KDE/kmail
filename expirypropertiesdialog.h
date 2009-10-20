
#ifndef EXPIRYPROPERTIESDIALOG_H
#define EXPIRYPROPERTIESDIALOG_H

#include <kdialog.h>

class QCheckBox;
class QRadioButton;
class KIntSpinBox;
class FolderCollection;
namespace KMail {

  class FolderRequester;

class ExpiryPropertiesDialog : public KDialog
{
    Q_OBJECT

public:
    ExpiryPropertiesDialog(
      QWidget *tree,
      FolderCollection* folder );
    ~ExpiryPropertiesDialog();

protected slots:
    void accept();
    void slotUpdateControls();

private:
    FolderCollection *mFolder;

    QCheckBox *expireReadMailCB;
    KIntSpinBox *expireReadMailSB;
    QCheckBox *expireUnreadMailCB;
    KIntSpinBox *expireUnreadMailSB;
    QRadioButton *moveToRB;
    FolderRequester *folderSelector;
    QRadioButton *deletePermanentlyRB;
};
} // namespace
#endif // EXPIRYPROPERTIESDIALOG_H
