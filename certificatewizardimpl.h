#ifndef CERTIFICATEWIZARDIMPL_H
#define CERTIFICATEWIZARDIMPL_H
#include "certificatewizard.h"

class CertificateWizardImpl : public CertificateWizard
{ 
    Q_OBJECT

public:
    CertificateWizardImpl( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~CertificateWizardImpl();

protected slots:
    void slotCreatePSE();

};

#endif // CERTIFICATEWIZARDIMPL_H
