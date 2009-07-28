
#ifndef EXPIRYPROPERTIESDIALOG_H
#define EXPIRYPROPERTIESDIALOG_H

#include <kdialog.h>

class QCheckBox;
class QRadioButton;
class KIntSpinBox;
class KMFolder;

namespace KMail {

  class FolderRequester;
  class MainFolderView;

class ExpiryPropertiesDialog : public KDialog
{
    Q_OBJECT

public:
    ExpiryPropertiesDialog( MainFolderView* tree, KMFolder* folder );
    ~ExpiryPropertiesDialog();

protected slots:
    void accept();
    void slotUpdateControls();

private:
    KMFolder *mFolder;

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
