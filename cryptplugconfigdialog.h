// -*- c++ -*-
#ifndef _CRYPT_PLUG_CONFIG_DIALOG_H_
#define _CRYPT_PLUG_CONFIG_DIALOG_H_

#include <kdialogbase.h>

class CryptPlugWrapper;

class SignatureConfigurationDialogImpl;
class EncryptionConfigurationDialogImpl;

class CryptPlugConfigDialog : public KDialogBase {
  Q_OBJECT
public:
  CryptPlugConfigDialog( CryptPlugWrapper * wrapper, int plugno,
		      const QString & caption,
		      QWidget * parent=0, const char * name=0,
		      bool modal=true );
  virtual ~CryptPlugConfigDialog() {}

protected slots:
  void slotOk();
  void slotStartCertManager();

protected:
  void setPluginInformation();

protected:
  SignatureConfigurationDialogImpl * mSignatureTab;
  EncryptionConfigurationDialogImpl * mEncryptionTab;

  CryptPlugWrapper * mWrapper;
  int mPluginNumber;
};

#endif // _CRYPT_PLUG_CONFIG_DIALOG_H_
