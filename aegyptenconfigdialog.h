// -*- c++ -*-
#ifndef _AEGYPTEN_CONFIG_DIALOG_H_
#define _AEGYPTEN_CONFIG_DIALOG_H_

#include <kdialogbase.h>

class CryptPlugWrapper;

class SignatureConfigurationDialogImpl;
class EncryptionConfigurationDialogImpl;
class DirectoryServicesConfigurationDialogImpl;

namespace Aegypten {

  class PluginConfigDialog : public KDialogBase {
    Q_OBJECT
  public:
    PluginConfigDialog( CryptPlugWrapper * wrapper, int plugno,
			const QString & caption,
			QWidget * parent=0, const char * name=0,
			bool modal=true );
    virtual ~PluginConfigDialog() {}

  protected slots:
    void slotOk();
    void slotStartCertManager();

  protected:
    void setPluginInformation();

  protected:
    SignatureConfigurationDialogImpl * mSignatureTab;
    EncryptionConfigurationDialogImpl * mEncryptionTab;
    DirectoryServicesConfigurationDialogImpl * mDirServiceTab;

    CryptPlugWrapper * mWrapper;
    int mPluginNumber;
  };

};


#endif // _AEGYPTEN_CONFIG_DIALOG_H_
