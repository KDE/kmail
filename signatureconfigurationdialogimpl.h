#ifndef SIGNATURECONFIGURATIONDIALOGIMPL_H
#define SIGNATURECONFIGURATIONDIALOGIMPL_H
#include "ui_signatureconfigurationdialog.h"

class CryptPlugWrapper;

class SignatureConfigurationDialogImpl : public QWidget, Ui::SignatureConfigurationDialog
{
    Q_OBJECT

public:
    SignatureConfigurationDialogImpl( QWidget* parent = 0 );

    void enableDisable( CryptPlugWrapper* );
};

#endif // SIGNATURECONFIGURATIONDIALOGIMPL_H
