/****************************************************************************
** Form interface generated from reading ui file 'expirypropertiesdialog.ui'
**
** Created: Sat Jan 29 12:59:18 2005
**      by: The User Interface Compiler ($Id$)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef EXPIRYPROPERTIESDIALOG_H
#define EXPIRYPROPERTIESDIALOG_H

#include <qvariant.h>
#include <kdialogbase.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QGroupBox;
class QCheckBox;
class QSpinBox;
class QLabel;
class QRadioButton;
class QButtonGroup;
class KMFolderTree;
class KMFolder;

namespace KMail {

  class FolderRequester;

class ExpiryPropertiesDialog : public KDialogBase
{
    Q_OBJECT

public:
    ExpiryPropertiesDialog( KMFolderTree* tree, KMFolder* folder );
    ~ExpiryPropertiesDialog();

    QCheckBox* expireReadMailCB;
    QSpinBox* expireReadMailSB;
    QLabel* labelDays;
    QCheckBox* expireUnreadMailCB;
    QSpinBox* expireUnreadMailSB;
    QLabel* labelDays2;
    QLabel* expiryActionLabel;
    QRadioButton* moveToRB;
    FolderRequester *folderSelector;
    QRadioButton* deletePermanentlyRB;
    QLabel* note;
    QButtonGroup* actionsGroup;

protected slots:
    void slotOk();
    void slotUpdateControls();

protected:
    QVBoxLayout* globalVBox;
    QHBoxLayout* readHBox;
    QHBoxLayout* unreadHBox;
    QHBoxLayout* expiryActionHBox;
    QVBoxLayout* actionsHBox;
    QHBoxLayout* moveToHBox;
    KMFolder* mFolder;
};
} // namespace
#endif // EXPIRYPROPERTIESDIALOG_H
