#include "signatureconfigurationdialogimpl.h"

#include "cryptplugwrapper.h"

#include <q3buttongroup.h>
#include <QRadioButton>
#include <QCheckBox>
#include <QSpinBox>

#define FULLTEST false // if false, hide a bunch of widgets that have
                       // yet to be implemented.

SignatureConfigurationDialogImpl::SignatureConfigurationDialogImpl( QWidget *parent )
  : QWidget( parent )
{
  setupUi( this );
}

void SignatureConfigurationDialogImpl::enableDisable( CryptPlugWrapper *cryptPlug )
{
  // disable the whole page if the plugin does not support
  // signatures (e.g. encryption only)
  setEnabled( cryptPlug->hasFeature( Feature_SignMessages ) );

  // enable/disable various components depending on the availability of
  // the respective feature in the crypto plugin.
  sendCertificatesBG->
    setEnabled( cryptPlug->hasFeature( Feature_SendCertificates ) );

  sigCompoundModeBG->
    setEnabled( cryptPlug->hasFeature( Feature_SendCertificates ) );

  warnSignatureCertificateExpiresCB->
    setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );

  warnSignatureCertificateExpiresSB->
    setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );

  warnCACertificateExpiresCB->
    setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );

  warnCACertificateExpiresSB->
    setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );

  warnRootCertificateExpiresCB->
    setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );

  warnRootCertificateExpiresSB->
    setEnabled( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) );

  warnAddressNotInCertificateCB->
    setEnabled( cryptPlug->hasFeature( Feature_WarnSignEmailNotInCertificate ) );

  pinEntryBG->
    setEnabled( cryptPlug->hasFeature( Feature_PinEntrySettings ) );

  saveSentSigsCB->
    setEnabled( cryptPlug->hasFeature( Feature_StoreMessagesWithSigs ) );

  if ( ! FULLTEST ) {
    askEachPartRB                ->hide(); // We won't implement that.

    sendCertificatesBG           ->hide(); // Will implement that later

    pinEntryBG                   ->hide(); // Will implement that later

    saveSentSigsCB->hide(); // We won't implement that.

    dontSendCertificatesRB       ->hide(); // Will implement that later.
    sendChainWithoutRootRB       ->hide(); // Will implement that later.
    sendChainWithRootRB          ->hide(); // Will implement that later.

    pinOncePerSessionRB          ->hide(); // Will implement that later.
    pinAddCertificatesRB         ->hide(); // Will implement that later.
    pinAlwaysWhenSigningRB       ->hide(); // Will implement that later.
    pinIntervalRB                ->hide(); // Will implement that later.
    pinIntervalSB                ->hide(); // Will implement that later.

    saveSentSigsCB               ->hide(); // We won't implement that.
  }
}

#include "signatureconfigurationdialogimpl.moc"
