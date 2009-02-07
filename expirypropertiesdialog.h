
#ifndef EXPIRYPROPERTIESDIALOG_H
#define EXPIRYPROPERTIESDIALOG_H

#include <kdialog.h>

class QCheckBox;
class QSpinBox;
class QRadioButton;
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
    void updateSpinBoxSuffix();

private:
    KMFolder *mFolder;

    QCheckBox *expireReadMailCB;
    QSpinBox *expireReadMailSB;
    QCheckBox *expireUnreadMailCB;
    QSpinBox *expireUnreadMailSB;
    QRadioButton *moveToRB;
    FolderRequester *folderSelector;
    QRadioButton *deletePermanentlyRB;
};
} // namespace
#endif // EXPIRYPROPERTIESDIALOG_H
