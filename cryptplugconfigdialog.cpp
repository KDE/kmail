
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cryptplugconfigdialog.h"

#include "signatureconfigurationdialogimpl.h"
#include "encryptionconfigurationdialogimpl.h"
#include "directoryservicesconfigurationdialogimpl.h"

#include <cryptplugwrapper.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kprocess.h>
#include <kmessagebox.h>

#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qlistview.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qpushbutton.h>

#include <cassert>
#include "kmkernel.h"

CryptPlugConfigDialog::CryptPlugConfigDialog( CryptPlugWrapper * wrapper,
				       int plugno, const QString & caption,
				       QWidget * parent,
				       const char * name, bool modal )
  : KDialogBase( Tabbed, caption, Ok|Cancel|User1, Ok, parent, name, modal,
		 false /* no <hr> */, i18n("Start Certificate &Manager") ),
  mWrapper( wrapper ), mPluginNumber( plugno )
{
  //    addVBoxPage( i18n("Directory Services") )
  mDirServiceTab = new DirectoryServicesConfigurationDialogImpl( this );
  mDirServiceTab->hide();
  mSignatureTab = new SignatureConfigurationDialogImpl( addVBoxPage( i18n("&Signature Configuration") ) );
  mSignatureTab->layout()->setMargin( 0 );
  mEncryptionTab = new EncryptionConfigurationDialogImpl( addVBoxPage( i18n("&Encryption Configuration") ) );
  mEncryptionTab->layout()->setMargin( 0 );

  connect( actionButton( User1 ), SIGNAL(clicked()),
	   SLOT(slotStartCertManager()) );

  setPluginInformation();
}


void CryptPlugConfigDialog::slotOk() {
  assert( mWrapper );

  KConfig* config = KMKernel::config();
  KConfigGroupSaver saver(config, "General");

  // Set the right config group
  config->setGroup( QString( "CryptPlug #%1" ).arg( mPluginNumber ) );


/*
            int numEntry = _generalPage->plugList->childCount();
            QListViewItem *item = _generalPage->plugList->firstChild();
            int i = 0;
            int curI = -1;
            for (i = 0; i < numEntry; ++i) {
                QString itemTxt( pluginNameToNumberedItem( item->text(0), 1+i ) );
         mWrapper = _pluginPage->mCryptPlugList->next(), ++i ) {
        if( ! mWrapper->displayName().isEmpty() ) {
            QString item = pluginNameToNumberedItem( mWrapper->displayName(), 1+i );
            _pluginPage->_certificatesPage->plugListBoxCertConf->insertItem( item );
            _pluginPage->_signaturePage->plugListBoxSignConf->insertItem(    item );
            _pluginPage->_encryptionPage->plugListBoxEncryptConf->insertItem( item );
            _pluginPage->_dirservicesPage->plugListBoxDirServConf->insertItem( item );

*/



  // The signature tab - everything here needs to be written both
  // into config and into the crypt plug mWrapper.

  // Sign Messages group box
  if( mSignatureTab->signAllPartsRB->isChecked() ) {
    mWrapper->setSignEmail( SignEmail_SignAll );
    config->writeEntry( "SignEmail", SignEmail_SignAll );
  } else if( mSignatureTab->askEachPartRB->isChecked() ) {
    mWrapper->setSignEmail( SignEmail_Ask );
    config->writeEntry( "SignEmail", SignEmail_Ask );
  } else {
    mWrapper->setSignEmail( SignEmail_DontSign );
    config->writeEntry( "SignEmail", SignEmail_DontSign );
  }
  bool warnSendUnsigned = mSignatureTab->warnUnsignedCB->isChecked();
  mWrapper->setWarnSendUnsigned( warnSendUnsigned );
  config->writeEntry( "WarnSendUnsigned", warnSendUnsigned );

  // Signature Compound Mode group box
  if( mSignatureTab->sendSigOpaqueRB->isChecked() ) {
    mWrapper->setSignatureCompoundMode( SignatureCompoundMode_Opaque );
    config->writeEntry( "SignatureCompoundMode", SignatureCompoundMode_Opaque );
  } else if( mSignatureTab->sendSigMultiPartRB->isChecked() ) {
    mWrapper->setSignatureCompoundMode( SignatureCompoundMode_Detached);
    config->writeEntry( "SignatureCompoundMode", SignatureCompoundMode_Detached );
  } else {
    mWrapper->setSignatureCompoundMode( SignatureCompoundMode_undef );
    config->writeEntry( "SignatureCompoundMode", SignatureCompoundMode_undef );
  }

  // Sending Certificates group box
  if( mSignatureTab->dontSendCertificatesRB->isChecked() ) {
    mWrapper->setSendCertificates( SendCert_DontSend );
    config->writeEntry( "SendCerts", SendCert_DontSend );
  } else if( mSignatureTab->sendYourOwnCertificateRB->isChecked() ) {
    mWrapper->setSendCertificates( SendCert_SendOwn );
    config->writeEntry( "SendCerts", SendCert_SendOwn );
  } else if( mSignatureTab->sendChainWithoutRootRB->isChecked() ) {
    mWrapper->setSendCertificates( SendCert_SendChainWithoutRoot );
    config->writeEntry( "SendCerts", SendCert_SendChainWithoutRoot );
  } else {
    mWrapper->setSendCertificates( SendCert_SendChainWithRoot );
    config->writeEntry( "SendCerts", SendCert_SendChainWithRoot );
  }

  // Signature Settings group box
  QString sigAlgoStr = mSignatureTab->signatureAlgorithmCO->currentText();
  SignatureAlgorithm sigAlgo = SignAlg_SHA1;
  if( sigAlgoStr == "RSA + SHA-1" )
    sigAlgo = SignAlg_SHA1;
  else
    kdDebug(5006) << "Unknown signature algorithm " << sigAlgoStr << endl;
  mWrapper->setSignatureAlgorithm( sigAlgo );
  config->writeEntry( "SigAlgo", sigAlgo );

  bool warnSigCertExp = mSignatureTab->warnSignatureCertificateExpiresCB->isChecked();
  mWrapper->setSignatureCertificateExpiryNearWarning( warnSigCertExp );
  config->writeEntry( "SigCertWarnNearExpire", warnSigCertExp );

  int warnSigCertExpInt = mSignatureTab->warnSignatureCertificateExpiresSB->value();
  mWrapper->setSignatureCertificateExpiryNearInterval( warnSigCertExpInt );
  config->writeEntry( "SigCertWarnNearExpireInt", warnSigCertExpInt );

  bool warnCACertExp = mSignatureTab->warnCACertificateExpiresCB->isChecked();
  mWrapper->setCACertificateExpiryNearWarning( warnCACertExp );
  config->writeEntry( "CACertWarnNearExpire", warnCACertExp );

  int warnCACertExpInt = mSignatureTab->warnCACertificateExpiresSB->value();
  mWrapper->setCACertificateExpiryNearInterval( warnCACertExpInt );
  config->writeEntry( "CACertWarnNearExpireInt", warnCACertExpInt );

  bool warnRootCertExp = mSignatureTab->warnRootCertificateExpiresCB->isChecked();
  mWrapper->setRootCertificateExpiryNearWarning( warnRootCertExp );
  config->writeEntry( "RootCertWarnNearExpire", warnRootCertExp );

  int warnRootCertExpInt = mSignatureTab->warnRootCertificateExpiresSB->value();
  mWrapper->setRootCertificateExpiryNearInterval( warnRootCertExpInt );
  config->writeEntry( "RootCertWarnNearExpireInt", warnRootCertExpInt );

  bool warnNoCertificate = mSignatureTab->warnAddressNotInCertificateCB->isChecked();
  mWrapper->setWarnNoCertificate( warnNoCertificate );
  config->writeEntry( "WarnEmailNotInCert", warnNoCertificate );

  // PIN Entry group box
  if( mSignatureTab->pinOncePerSessionRB->isChecked() ) {
    mWrapper->setNumPINRequests( PinRequest_OncePerSession );
    config->writeEntry( "NumPINRequests", PinRequest_OncePerSession );
  } else if( mSignatureTab->pinAlwaysRB->isChecked() ) {
    mWrapper->setNumPINRequests( PinRequest_Always );
    config->writeEntry( "NumPINRequests", PinRequest_Always );
  } else if( mSignatureTab->pinAddCertificatesRB->isChecked() ) {
    mWrapper->setNumPINRequests( PinRequest_WhenAddingCerts );
    config->writeEntry( "NumPINRequests", PinRequest_WhenAddingCerts );
  } else if( mSignatureTab->pinAlwaysWhenSigningRB->isChecked() ) {
    mWrapper->setNumPINRequests( PinRequest_AlwaysWhenSigning );
    config->writeEntry( "NumPINRequests", PinRequest_AlwaysWhenSigning );
  } else {
    mWrapper->setNumPINRequests( PinRequest_AfterMinutes );
    config->writeEntry( "NumPINRequests", PinRequest_AfterMinutes );
  }
  int pinInt = mSignatureTab->pinIntervalSB->value();
  mWrapper->setNumPINRequestsInterval( pinInt );
  config->writeEntry( "NumPINRequestsInt", pinInt );

  // Save Messages group box
  bool saveSigs = mSignatureTab->saveSentSigsCB->isChecked();
  mWrapper->setSaveSentSignatures( saveSigs );
  config->writeEntry( "SaveSentSigs", saveSigs );

  // The encryption tab - everything here needs to be written both
  // into config and into the crypt plug wrapper.

  // Encrypt Messages group box
  if( mEncryptionTab->encryptAllPartsRB->isChecked() ) {
    mWrapper->setEncryptEmail( EncryptEmail_EncryptAll );
    config->writeEntry( "EncryptEmail", EncryptEmail_EncryptAll );
  } else if( mEncryptionTab->askEachPartRB->isChecked() ) {
    mWrapper->setEncryptEmail( EncryptEmail_Ask );
    config->writeEntry( "EncryptEmail", EncryptEmail_Ask );
  } else {
    mWrapper->setEncryptEmail( EncryptEmail_DontEncrypt );
    config->writeEntry( "EncryptEmail", EncryptEmail_DontEncrypt );
  }
  bool warnSendUnencrypted = mEncryptionTab->warnUnencryptedCB->isChecked();
  mWrapper->setWarnSendUnencrypted( warnSendUnencrypted );
  config->writeEntry( "WarnSendUnencrypted", warnSendUnencrypted );

  bool alwaysEncryptToSelf = mEncryptionTab->alwaysEncryptToSelfCB->isChecked();
  mWrapper->setAlwaysEncryptToSelf( alwaysEncryptToSelf );
  config->writeEntry( "AlwaysEncryptToSelf", alwaysEncryptToSelf );

  // Encryption Settings group box
  QString encAlgoStr = mEncryptionTab->encryptionAlgorithmCO->currentText();
  EncryptionAlgorithm encAlgo = EncryptAlg_RSA;
  if( encAlgoStr == "RSA" )
    encAlgo = EncryptAlg_RSA;
  else if( encAlgoStr == "Triple-DES" )
    encAlgo = EncryptAlg_TripleDES;
  else if( encAlgoStr == "SHA-1" )
    encAlgo = EncryptAlg_SHA1;
  else
    kdDebug(5006) << "Unknown encryption algorithm " << encAlgoStr << endl;
  mWrapper->setEncryptionAlgorithm( encAlgo );
  config->writeEntry( "EncryptAlgo", encAlgo );

  bool recvCertExp = mEncryptionTab->warnReceiverCertificateExpiresCB->isChecked();
  mWrapper->setReceiverCertificateExpiryNearWarning( recvCertExp );
  config->writeEntry( "WarnRecvCertNearExpire", recvCertExp );

  int recvCertExpInt = mEncryptionTab->warnReceiverCertificateExpiresSB->value();
  mWrapper->setReceiverCertificateExpiryNearWarningInterval( recvCertExpInt );
  config->writeEntry( "WarnRecvCertNearExpireInt", recvCertExpInt );

  bool certChainExp = mEncryptionTab->warnChainCertificateExpiresCB->isChecked();
  mWrapper->setCertificateInChainExpiryNearWarning( certChainExp );
  config->writeEntry( "WarnCertInChainNearExpire", certChainExp );

  int certChainExpInt = mEncryptionTab->warnChainCertificateExpiresSB->value();
  mWrapper->setCertificateInChainExpiryNearWarningInterval( certChainExpInt );
  config->writeEntry( "WarnCertInChainNearExpireInt", certChainExpInt );

  bool recvNotInCert = mEncryptionTab->warnReceiverNotInCertificateCB->isChecked();
  mWrapper->setReceiverEmailAddressNotInCertificateWarning( recvNotInCert );
  config->writeEntry( "WarnRecvAddrNotInCert", recvNotInCert );

  // CRL group box
  bool useCRL = mEncryptionTab->useCRLsCB->isChecked();
  mWrapper->setEncryptionUseCRLs( useCRL );
  config->writeEntry( "EncryptUseCRLs", useCRL );

  bool warnCRLExp = mEncryptionTab->warnCRLExpireCB->isChecked();
  mWrapper->setEncryptionCRLExpiryNearWarning( warnCRLExp );
  config->writeEntry( "EncryptCRLWarnNearExpire", warnCRLExp );

  int warnCRLExpInt = mEncryptionTab->warnCRLExpireSB->value();
  mWrapper->setEncryptionCRLNearExpiryInterval( warnCRLExpInt );
  config->writeEntry( "EncryptCRLWarnNearExpireInt", warnCRLExpInt );

  // Save Messages group box
  bool saveEnc = mEncryptionTab->storeEncryptedCB->isChecked();
  mWrapper->setSaveMessagesEncrypted( saveEnc );
  config->writeEntry( "SaveMsgsEncrypted", saveEnc );

  // Certificate Path Check group box
  bool checkPath = mEncryptionTab->checkCertificatePathCB->isChecked();
  mWrapper->setCheckCertificatePath( checkPath );
  config->writeEntry( "CheckCertPath", checkPath );

  bool checkToRoot = mEncryptionTab->alwaysCheckRootRB->isChecked();
  mWrapper->setCheckEncryptionCertificatePathToRoot( checkToRoot );
  config->writeEntry( "CheckEncryptCertToRoot", checkToRoot );

  // The directory services tab - everything here needs to be written both
  // into config and into the crypt plug wrapper.

  uint numDirServers = mDirServiceTab->x500LV->childCount();
  CryptPlugWrapper::DirectoryServer* servers = new CryptPlugWrapper::DirectoryServer[numDirServers];
  config->writeEntry( "NumDirServers", numDirServers );
  QListViewItemIterator lvit( mDirServiceTab->x500LV );
  QListViewItem* current;
  int pos = 0;
  while( ( current = lvit.current() ) ) {
    ++lvit;
    const char* servername = current->text( 0 ).utf8();
    int port = current->text( 1 ).toInt();
    const char* description = current->text( 2 ).utf8();
    servers[pos].servername = new char[strlen( servername )+1];
    strcpy( servers[pos].servername, servername );
    servers[pos].port = port;
    servers[pos].description = new char[strlen( description)+1];
    strcpy( servers[pos].description, description );
    config->writeEntry( QString( "DirServer%1Name" ).arg( pos ),
			current->text( 0 ) );
    config->writeEntry( QString( "DirServer%1Port" ).arg( pos ),
			port );
    config->writeEntry( QString( "DirServer%1Descr" ).arg( pos ),
			current->text( 2 ) );
    pos++;
  }
  mWrapper->setDirectoryServers( servers, numDirServers );
  for( uint i = 0; i < numDirServers; i++ ) {
    delete[] servers[i].servername;
    delete[] servers[i].description;
  }
  delete[] servers;

  // Local/Remote Certificates group box
  if( mDirServiceTab->firstLocalThenDSCertRB->isChecked() ) {
    mWrapper->setCertificateSource( CertSrc_ServerLocal );
    config->writeEntry( "CertSource", CertSrc_ServerLocal );
  } else if( mDirServiceTab->localOnlyCertRB->isChecked() ) {
    mWrapper->setCertificateSource( CertSrc_Local );
    config->writeEntry( "CertSource", CertSrc_Local );
  } else {
    mWrapper->setCertificateSource( CertSrc_Server );
    config->writeEntry( "CertSource", CertSrc_Server );
  }

  // Local/Remote CRLs group box
  if( mDirServiceTab->firstLocalThenDSCRLRB->isChecked() ) {
    mWrapper->setCRLSource( CertSrc_ServerLocal );
    config->writeEntry( "CRLSource", CertSrc_ServerLocal );
  } else if( mDirServiceTab->localOnlyCRLRB->isChecked() ) {
    mWrapper->setCRLSource( CertSrc_Local );
    config->writeEntry( "CRLSource", CertSrc_Local );
  } else {
    mWrapper->setCRLSource( CertSrc_Server );
    config->writeEntry( "CRLSource", CertSrc_Server );
  }

  accept();
}


void CryptPlugConfigDialog::setPluginInformation() {
  assert( mWrapper );

  // Enable/disable the fields in the dialog pages
  // according to the plugin capabilities.
  mSignatureTab->enableDisable( mWrapper );
  mEncryptionTab->enableDisable( mWrapper );
  mDirServiceTab->enableDisable( mWrapper );

  // Fill the fields of the tab pages with the data from
  // the current plugin.

  // Signature tab

  // Sign Messages group box
  switch( mWrapper->signEmail() ) {
  case SignEmail_SignAll:
    mSignatureTab->signAllPartsRB->setChecked( true );
    break;
  case SignEmail_Ask:
    mSignatureTab->askEachPartRB->setChecked( true );
    break;
  case SignEmail_DontSign:
    mSignatureTab->dontSignRB->setChecked( true );
    break;
  default:
    kdDebug( 5006 ) << "Unknown email sign setting" << endl;
  };
  mSignatureTab->warnUnsignedCB->setChecked( mWrapper->warnSendUnsigned() );

  // Compound Mode group box
  switch( mWrapper->signatureCompoundMode() ) {
  case SignatureCompoundMode_Opaque:
    mSignatureTab->sendSigOpaqueRB->setChecked( true );
    break;
  case SignatureCompoundMode_Detached:
    mSignatureTab->sendSigMultiPartRB->setChecked( true );
    break;
  default:
    mSignatureTab->sendSigMultiPartRB->setChecked( true );
    kdDebug( 5006 ) << "Unknown signature compound mode setting, default set to multipart/signed" << endl;
  };

  // Sending Certificates group box
  switch( mWrapper->sendCertificates() ) {
  case SendCert_DontSend:
    mSignatureTab->dontSendCertificatesRB->setChecked( true );
    break;
  case SendCert_SendOwn:
    mSignatureTab->sendYourOwnCertificateRB->setChecked( true );
    break;
  case SendCert_SendChainWithoutRoot:
    mSignatureTab->sendChainWithoutRootRB->setChecked( true );
    break;
  case SendCert_SendChainWithRoot:
    mSignatureTab->sendChainWithRootRB->setChecked( true );
    break;
  default:
    kdDebug( 5006 ) << "Unknown send certificate setting" << endl;
  }

  // Signature Settings group box
  SignatureAlgorithm sigAlgo = mWrapper->signatureAlgorithm();
  QString sigAlgoStr;
  switch( sigAlgo ) {
  case SignAlg_SHA1:
    sigAlgoStr = "SHA1";
    break;
  default:
    kdDebug( 5006 ) << "Unknown signature algorithm" << endl;
  };

  for( int i = 0; i < mSignatureTab->signatureAlgorithmCO->count(); ++i )
    if( mSignatureTab->signatureAlgorithmCO->text( i ) ==
	sigAlgoStr ) {
      mSignatureTab->signatureAlgorithmCO->setCurrentItem( i );
      break;
    }

  mSignatureTab->warnSignatureCertificateExpiresCB->setChecked( mWrapper->signatureCertificateExpiryNearWarning() );
  mSignatureTab->warnSignatureCertificateExpiresSB->setValue( mWrapper->signatureCertificateExpiryNearInterval() );
  mSignatureTab->warnCACertificateExpiresCB->setChecked( mWrapper->caCertificateExpiryNearWarning() );
  mSignatureTab->warnCACertificateExpiresSB->setValue( mWrapper->caCertificateExpiryNearInterval() );
  mSignatureTab->warnRootCertificateExpiresCB->setChecked( mWrapper->rootCertificateExpiryNearWarning() );
  mSignatureTab->warnRootCertificateExpiresSB->setValue( mWrapper->rootCertificateExpiryNearInterval() );

  mSignatureTab->warnAddressNotInCertificateCB->setChecked( mWrapper->warnNoCertificate() );

  // PIN Entry group box
  switch( mWrapper->numPINRequests() ) {
  case PinRequest_OncePerSession:
    mSignatureTab->pinOncePerSessionRB->setChecked( true );
    break;
  case PinRequest_Always:
    mSignatureTab->pinAlwaysRB->setChecked( true );
    break;
  case PinRequest_WhenAddingCerts:
    mSignatureTab->pinAddCertificatesRB->setChecked( true );
    break;
  case PinRequest_AlwaysWhenSigning:
    mSignatureTab->pinAlwaysWhenSigningRB->setChecked( true );
    break;
  case PinRequest_AfterMinutes:
    mSignatureTab->pinIntervalRB->setChecked( true );
    break;
  default:
    kdDebug( 5006 ) << "Unknown pin request setting" << endl;
  };

  mSignatureTab->pinIntervalSB->setValue( mWrapper->numPINRequestsInterval() );

  // Save Messages group box
  mSignatureTab->saveSentSigsCB->setChecked( mWrapper->saveSentSignatures() );

  // The Encryption tab

  // Encrypt Messages group box
  switch( mWrapper->encryptEmail() ) {
  case EncryptEmail_EncryptAll:
    mEncryptionTab->encryptAllPartsRB->setChecked( true );
    break;
  case EncryptEmail_Ask:
    mEncryptionTab->askEachPartRB->setChecked( true );
    break;
  case EncryptEmail_DontEncrypt:
    mEncryptionTab->dontEncryptRB->setChecked( true );
    break;
  default:
    kdDebug( 5006 ) << "Unknown email encryption setting" << endl;
  };
  mEncryptionTab->warnUnencryptedCB->setChecked( mWrapper->warnSendUnencrypted() );
  mEncryptionTab->alwaysEncryptToSelfCB->setChecked( mWrapper->alwaysEncryptToSelf() );

  // Encryption Settings group box
  QString encAlgoStr;
  switch( mWrapper->encryptionAlgorithm() ) {
  case EncryptAlg_RSA:
    encAlgoStr = "RSA";
    break;
  case EncryptAlg_TripleDES:
    encAlgoStr = "Triple-DES";
    break;
  case EncryptAlg_SHA1:
    encAlgoStr = "SHA-1";
    break;
  default:
    kdDebug( 5006 ) << "Unknown encryption algorithm" << endl;
  };

  for( int i = 0; i < mEncryptionTab->encryptionAlgorithmCO->count(); ++i )
    if( mEncryptionTab->encryptionAlgorithmCO->text( i ) == encAlgoStr ) {
      mEncryptionTab->encryptionAlgorithmCO->setCurrentItem( i );
      break;
    }

  mEncryptionTab->warnReceiverCertificateExpiresCB->setChecked( mWrapper->receiverCertificateExpiryNearWarning() );
  mEncryptionTab->warnReceiverCertificateExpiresSB->setValue( mWrapper->receiverCertificateExpiryNearWarningInterval() );
  mEncryptionTab->warnChainCertificateExpiresCB->setChecked( mWrapper->certificateInChainExpiryNearWarning() );
  mEncryptionTab->warnChainCertificateExpiresSB->setValue( mWrapper->certificateInChainExpiryNearWarningInterval() );
  mEncryptionTab->warnReceiverNotInCertificateCB->setChecked( mWrapper->receiverEmailAddressNotInCertificateWarning() );

  // CRL group box
  mEncryptionTab->useCRLsCB->setChecked( mWrapper->encryptionUseCRLs() );
  mEncryptionTab->warnCRLExpireCB->setChecked( mWrapper->encryptionCRLExpiryNearWarning() );
  mEncryptionTab->warnCRLExpireSB->setValue( mWrapper->encryptionCRLNearExpiryInterval() );

  // Save Messages group box
  mEncryptionTab->storeEncryptedCB->setChecked( mWrapper->saveMessagesEncrypted() );

  // Certificate Path Check group box
  mEncryptionTab->checkCertificatePathCB->setChecked( mWrapper->checkCertificatePath() );
  if( mWrapper->checkEncryptionCertificatePathToRoot() )
    mEncryptionTab->alwaysCheckRootRB->setChecked( true );
  else
    mEncryptionTab->pathMayEndLocallyCB->setChecked( true );

  // Directory Services tab page

  int numServers;
  CryptPlugWrapper::DirectoryServer* servers = mWrapper->directoryServers( &numServers );
  if( servers ) {
    QListViewItem* previous = 0;
    for( int i = 0; i < numServers; i++ ) {
      previous = new QListViewItem( mDirServiceTab->x500LV,
				    previous,
				    QString::fromUtf8( servers[i].servername ),
				    QString::number( servers[i].port ),
				    QString::fromUtf8( servers[i].description ) );
    }
  }

  // Local/Remote Certificates group box
  switch( mWrapper->certificateSource() ) {
  case CertSrc_ServerLocal:
    mDirServiceTab->firstLocalThenDSCertRB->setChecked( true );
    break;
  case CertSrc_Local:
    mDirServiceTab->localOnlyCertRB->setChecked( true );
    break;
  case CertSrc_Server:
    mDirServiceTab->dsOnlyCertRB->setChecked( true );
    break;
  default:
    kdDebug( 5006 ) << "Unknown certificate source" << endl;
  }

  // Local/Remote CRL group box
  switch( mWrapper->crlSource() ) {
  case CertSrc_ServerLocal:
    mDirServiceTab->firstLocalThenDSCRLRB->setChecked( true );
    break;
  case CertSrc_Local:
    mDirServiceTab->localOnlyCRLRB->setChecked( true );
    break;
  case CertSrc_Server:
    mDirServiceTab->dsOnlyCRLRB->setChecked( true );
    break;
  default:
    kdDebug( 5006 ) << "Unknown certificate source" << endl;
  }
}

void CryptPlugConfigDialog::slotStartCertManager() {
  assert( mWrapper );

  KProcess certManagerProc; // save to create on the heap, since
                            // there is no parent
  certManagerProc << "kgpgcertmanager";
  certManagerProc << mWrapper->displayName();
  certManagerProc << mWrapper->libName();

  if( !certManagerProc.start( KProcess::DontCare ) )
    KMessageBox::error( this, i18n( "Could not start certificate manager. Please check your installation!" ),
			i18n( "KMail Error" ) );
  else
    kdDebug(5006) << "\nCertificatesPage::slotStartCertManager(): certificate manager started.\n" << endl;
  // process continues to run even after the KProcess object goes
  // out of scope here, since it is started in DontCare run mode.
}


#include "cryptplugconfigdialog.moc"
