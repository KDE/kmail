#ifndef ENCRYPTIONCONFIGURATIONDIALOGIMPL_H
#define ENCRYPTIONCONFIGURATIONDIALOGIMPL_H
#include "encryptionconfigurationdialog.h"

class CryptPlugWrapper;

class EncryptionConfigurationDialogImpl : public EncryptionConfigurationDialog
{
    Q_OBJECT

public:
    EncryptionConfigurationDialogImpl( QWidget* parent = 0, 
                                       const char* name = 0, WFlags fl = 0 );
    ~EncryptionConfigurationDialogImpl();

    void enableDisable( CryptPlugWrapper* wrapper );
};

#endif // ENCRYPTIONCONFIGURATIONDIALOGIMPL_H
