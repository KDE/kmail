#include <config.h>
#include "signatureconfigurationdialogimpl.h"
#include "cryptplugwrapper.h"

#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qspinbox.h>




#define FULLTEST false




/*
 *  Constructs a SignatureConfigurationDialogImpl which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
SignatureConfigurationDialogImpl::SignatureConfigurationDialogImpl( QWidget* parent,  const char* name, WFlags fl )
    : SignatureConfigurationDialog( parent, name, fl )
{
}

/*
 *  Destroys the object and frees any allocated resources
 */
SignatureConfigurationDialogImpl::~SignatureConfigurationDialogImpl()
{
    // no need to delete child widgets, Qt does it all for us
}


/**
   Enables or disables the widgets in this dialog according to the
   capabilities of the current plugin passed as a parameter.
*/
void SignatureConfigurationDialogImpl::enableDisable( CryptPlugWrapper* cryptPlug )
{
    // disable the whole page if the plugin does not support
    // signatures (e.g. encryption only)
    setEnabled( cryptPlug->hasFeature( Feature_SignMessages ) );

    // enable and disable the various components depending on the
    // availability of a feature in the crypto plugin
    sendCertificatesBG->setEnabled( cryptPlug->hasFeature( Feature_SendCertificates ) );
    sigCompoundModeBG->setEnabled( cryptPlug->hasFeature( Feature_SendCertificates ) );
    warnSignatureCertificateExpiresCB->setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );
    warnSignatureCertificateExpiresSB->setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );
    warnCACertificateExpiresCB->setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );
    warnCACertificateExpiresSB->setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );
    warnRootCertificateExpiresCB->setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );
    warnRootCertificateExpiresSB->setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );
    warnAddressNotInCertificateCB->setEnabled( cryptPlug->hasFeature( Feature_WarnSignEmailNotInCertificate ) );
    pinEntryBG->setEnabled( cryptPlug->hasFeature( Feature_PinEntrySettings ) );
    saveMessagesBG->setEnabled( cryptPlug->hasFeature( Feature_StoreMessagesWithSigs ) );
    
    if( ! FULLTEST ){
        askEachPartRB                ->hide(); // We won't implement that.
        
        dontSendCertificatesRB       ->hide(); // Will implement that later.
        sendChainWithoutRootRB       ->hide(); // Will implement that later.
        sendChainWithRootRB          ->hide(); // Will implement that later.
        
        warnCACertificateExpiresCB   ->hide(); // Will implement that later.
        warnCACertificateExpiresSB   ->hide(); // Will implement that later.
        warnRootCertificateExpiresCB ->hide(); // Will implement that later.
        warnRootCertificateExpiresSB ->hide(); // Will implement that later.
        warnAddressNotInCertificateCB->hide(); // Will implement that later.
        
        sendSigOpaqueRB              ->hide(); // Will implement that later.
        
        pinOncePerSessionRB          ->hide(); // Will implement that later.
        pinAddCertificatesRB         ->hide(); // Will implement that later.
        pinAlwaysWhenSigningRB       ->hide(); // Will implement that later.
        pinIntervalRB                ->hide(); // Will implement that later.
        pinIntervalSB                ->hide(); // Will implement that later.
        
        saveSentSigsCB               ->hide(); // We won't implement that.
        saveMessagesBG               ->hide(); // We won't implement that.
    }
}

#include "signatureconfigurationdialogimpl.moc"
