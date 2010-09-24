
#ifndef EXPIRYPROPERTIESDIALOG_H
#define EXPIRYPROPERTIESDIALOG_H

#include <kdialog.h>
#include <QSharedPointer>

class QCheckBox;
class QRadioButton;
class KIntSpinBox;
class FolderCollection;

class MailCommon;

namespace KMail {

  class FolderRequester;

class ExpiryPropertiesDialog : public KDialog
{
    Q_OBJECT

public:
    ExpiryPropertiesDialog(
      QWidget *tree,
      const QSharedPointer<FolderCollection> &folder,
      MailCommon *mailCommon );
    ~ExpiryPropertiesDialog();

protected slots:
    void accept();
    void slotUpdateControls();

private:
    QSharedPointer<FolderCollection> mFolder;

    QCheckBox *expireReadMailCB;
    KIntSpinBox *expireReadMailSB;
    QCheckBox *expireUnreadMailCB;
    KIntSpinBox *expireUnreadMailSB;
    QRadioButton *moveToRB;
    FolderRequester *folderSelector;
    QRadioButton *deletePermanentlyRB;
    MailCommon *mMailCommon;
};
} // namespace
#endif // EXPIRYPROPERTIESDIALOG_H
