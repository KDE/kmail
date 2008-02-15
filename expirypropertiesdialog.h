
#ifndef EXPIRYPROPERTIESDIALOG_H
#define EXPIRYPROPERTIESDIALOG_H

#include <kdialog.h>

class QCheckBox;
class QSpinBox;
class QRadioButton;
class KMFolderTree;
class KMFolder;

namespace KMail {

  class FolderRequester;

class ExpiryPropertiesDialog : public KDialog
{
    Q_OBJECT

public:
    ExpiryPropertiesDialog( KMFolderTree* tree, KMFolder* folder );
    ~ExpiryPropertiesDialog();

protected slots:
    void accept();
    void slotUpdateControls();

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
