#ifndef SIGNATURECONFIGURATIONDIALOGIMPL_H
#define SIGNATURECONFIGURATIONDIALOGIMPL_H

#include "ui_signatureconfigurationdialog.h"

class CryptPlugWrapper;

class SignatureConfigurationDialogImpl : public QWidget, Ui::SignatureConfigurationDialog
{
  Q_OBJECT

  public:
    /*
      Constructs a SignatureConfigurationDialogImpl.
    */
    SignatureConfigurationDialogImpl( QWidget *parent = 0 );

    /**
      Enables or disables the widgets in this dialog according to the
      capabilities of the current plugin passed as a parameter.
    */
    void enableDisable( CryptPlugWrapper *cryptPlug );
};

#endif
