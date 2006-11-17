#ifndef ENCRYPTIONCONFIGURATIONDIALOGIMPL_H
#define ENCRYPTIONCONFIGURATIONDIALOGIMPL_H
#include "ui_encryptionconfigurationdialog.h"

class CryptPlugWrapper;

class EncryptionConfigurationDialog : public QWidget, public Ui::EncryptionConfigurationDialog
{
public:
  EncryptionConfigurationDialog( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

class EncryptionConfigurationDialogImpl : public EncryptionConfigurationDialog
{
    Q_OBJECT

public:
    EncryptionConfigurationDialogImpl( QWidget* parent = 0); 
    ~EncryptionConfigurationDialogImpl();

    void enableDisable( CryptPlugWrapper* wrapper );
};

#endif // ENCRYPTIONCONFIGURATIONDIALOGIMPL_H
