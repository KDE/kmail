#ifndef SIGNATURECONFIGURATIONDIALOGIMPL_H
#define SIGNATURECONFIGURATIONDIALOGIMPL_H
#include "signatureconfigurationdialog.h"

class CryptPlugWrapper;

class SignatureConfigurationDialogImpl : public SignatureConfigurationDialog
{
    Q_OBJECT

public:
    SignatureConfigurationDialogImpl( QWidget* parent = 0, 
                                      const char* name = 0, WFlags fl = 0 );
    ~SignatureConfigurationDialogImpl();

    void enableDisable( CryptPlugWrapper* );
};

#endif // SIGNATURECONFIGURATIONDIALOGIMPL_H
