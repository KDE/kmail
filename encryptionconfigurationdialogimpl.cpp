#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "encryptionconfigurationdialogimpl.h"
#include "cryptplugwrapper.h"

#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qspinbox.h>




#define FULLTEST false




/*
 *  Constructs a EncryptionConfigurationDialogImpl which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
EncryptionConfigurationDialogImpl::EncryptionConfigurationDialogImpl( QWidget* parent,  const char* name, WFlags fl )
    : EncryptionConfigurationDialog( parent, name, fl )
{
}

/*
 *  Destroys the object and frees any allocated resources
 */
EncryptionConfigurationDialogImpl::~EncryptionConfigurationDialogImpl()
{
    // no need to delete child widgets, Qt does it all for us
}


/**
   Enables or disables the widgets in this dialog according to the
   capabilities of the current plugin passed as a parameter.
*/
void EncryptionConfigurationDialogImpl::enableDisable( CryptPlugWrapper* cryptPlug )
{
    // disable the whole page if the plugin does not support encryption
    setEnabled( cryptPlug->hasFeature( Feature_EncryptMessages ) );
    
    // enable/disable individual components depending on the plugin features
    crlBG->setEnabled( cryptPlug->hasFeature( Feature_EncryptionCRLs ) );
    warnReceiverCertificateExpiresCB->setEnabled( cryptPlug->hasFeature( Feature_WarnEncryptCertificateExpiry ) );
    warnReceiverCertificateExpiresSB->setEnabled( cryptPlug->hasFeature( Feature_WarnEncryptCertificateExpiry ) );
    warnChainCertificateExpiresCB->setEnabled( cryptPlug->hasFeature( Feature_WarnEncryptCertificateExpiry ) );
    warnChainCertificateExpiresSB->setEnabled( cryptPlug->hasFeature( Feature_WarnEncryptCertificateExpiry ) );
    warnReceiverNotInCertificateCB->setEnabled( cryptPlug->hasFeature( Feature_WarnEncryptEmailNotInCertificate ) );
    storeEncryptedCB->setEnabled( cryptPlug->hasFeature( Feature_StoreMessagesEncrypted ) );
    certificatePathCheckBG->setEnabled( cryptPlug->hasFeature( Feature_CheckCertificatePath ) );
    
    if( ! FULLTEST ) {
        askEachPartRB                 ->hide(); // We won't implement that.
        
//        encryptionSettingsBG          ->hide(); // Will implement that later.
        
        certBG                         ->hide(); // Will implement that later.

        warnCRLExpireCB               ->hide(); // Will implement that later.
        warnCRLExpireSB               ->hide(); // Will implement that later.
        
//        warnReceiverCertificateExpiresCB->hide();//Will implement that later.
//        warnReceiverCertificateExpiresSB->hide();//Will implement that later.
        
        warnChainCertificateExpiresCB ->hide(); // Will implement that later.
        warnChainCertificateExpiresSB ->hide(); // Will implement that later.
        warnReceiverNotInCertificateCB->hide(); // Will implement that later.
        
        alwaysCheckRootRB             ->hide(); // Will implement that later.
        
        /*enable:*/
        pathMayEndLocallyCB           ->setEnabled( true );
        
    }
}

#include "encryptionconfigurationdialogimpl.moc"
