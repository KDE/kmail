#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "certificatewizardimpl.h"
#include <kdebug.h>
/*
 *  Constructs a CertificateWizardImpl which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The wizard will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal wizard.
 */
CertificateWizardImpl::CertificateWizardImpl( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : CertificateWizard( parent, name, modal, fl )
{
}

/*
 *  Destroys the object and frees any allocated resources
 */
CertificateWizardImpl::~CertificateWizardImpl()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 * protected slot
 */
void CertificateWizardImpl::slotCreatePSE()
{
    kdWarning() << "CertificateWizardImpl::slotCreatePSE() not yet implemented!" << endl;
}


#include "certificatewizardimpl.moc"
