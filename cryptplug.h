/* -*- Mode: C -*-

  $Id$

  CRYPTPLUG - an independent cryptography plug-in API

  Copyright (C) 2001 by Klar�lvdalens Datakonsult AB

  CRYPTPLUG is free software; you can redistribute it and/or modify
  it under the terms of GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  CRYPTPLUG is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
*/

#ifndef CRYPTPLUG_H
#define CRYPTPLUG_H

#ifdef __cplusplus
extern "C" {
#else
typedef char bool;
#define true 1
#define false 0
#endif

//#include <stdlib.h>
//#include <string.h>
//#include <ctype.h>


/*! \file cryptplug.h
    \brief Common API header for CRYPTPLUG.

    CRYPTPLUG is an independent cryptography plug-in API
    developed for Sphinx-enabeling KMail and Mutt.

    CRYPTPLUG was designed for the Aegypten project, but it may
    be used by 3rd party developers as well to design pluggable
    crypto backends for the above mentioned MUAs.

    \note All string parameters appearing in this API are to be
    interpreted as UTF-8 encoded.

    \see pgpplugin.c
    \see gpgplugin.c
*/

/*! \defgroup groupGeneral Loading and Unloading the Plugin, General Functionality

    The functions in this section are used for loading and
    unloading plugins. Note that the actual locating of the plugin
    and the loading and unloading of the dynamic library is not
    covered here; this is MUA-specific code for which support code
    might already exist in the programming environments.
*/

/*! \defgroup groupDisplay Graphical Display Functionality

    The functions in this section return stationery that the
    MUAs can use in order to display security functionality
    graphically. This can be toolbar icons, shortcuts, tooltips,
    etc. Not all MUAs will use all this functionality.
*/

/*! \defgroup groupConfig Configuration Support

    The functions in this section provide the necessary
    functionality to configure the security functionality as well
    as to query configuration settings. Since all configuration
    settings will not be saved with the plugin, but rather with
    the MUA, there are also functions to set configuration
    settings programmatically; these will be used on startup of
    the plugin when the MUA transfers the configuration values it
    has read into the plugin. Usually, the functions to query and
    set the configuration values are not needed for anything but
    saving to and restoring from configuration files.
*/


/*! \defgroup groupConfigSign Signature Configuration
    \ingroup groupConfig

    The functions in this section provide the functionality
    to configure signature handling and set and query the
    signature configuration.
*/

/*! \defgroup groupConfigCrypt Encryption Configuration
    \ingroup groupConfig

    The functions in this section provide the functionality
    to configure encryption handling and set and query the
    encryption configuration.

    \note Whenever the term <b> encryption</b> is used here,
    it is supposed to mean both encryption and decryption,
    unless otherwise specified.
*/

/*! \defgroup groupConfigDir Directory Service Configuration
    \ingroup groupConfig

    This section contains messages for configuring the
    directory service.
*/


/*! \defgroup groupCertHand Certificate Handling

    The following methods are used to maintain and query certificates.
*/

/*! \defgroup groupSignAct Signature Actions

    This section describes methods that are used for working
    with signatures.
*/

/*! \defgroup groupCryptAct Encryption and Decryption

    The following methods are used to encrypt and decrypt
    email messages.
*/

/*! \defgroup groupCertAct Certificate Handling Actions

    The functions in this section provide local certificate management.
*/

/*! \defgroup groupCRLAct CRL Handling Actions

    This section describes functions for managing CRLs.
*/





// dummy values:
typedef enum {
  CryptPlugFeat_undef             = 0,

  CryptPlugFeat_SignMessages      = 1,
  CryptPlugFeat_VerifySignatures  = 2,
  CryptPlugFeat_EncryptMessages   = 3,
  CryptPlugFeat_DecryptMessages   = 4   // more to follow ...
} Feature;

// dummy values
typedef enum {
  PinRequest_undef            = 0,

  PinRequest_allways          = 1,
  PinRequest_oncePerMail      = 2,
  PinRequest_oncePerSession   = 3      // may be changed ...
} PinRequests;

// dummy values:
typedef enum {
  SendCert_undef              = 0,

  SendCert_1                  = 1,
  SendCert_2                  = 2,
  SendCert_3                  = 3,     // must be changed !
} SendCertificates;

// dummy values:
typedef enum {
  SignAlg_undef               = 0,

  SignAlg_1                   = 1,
  SignAlg_2                   = 2,
  SignAlg_3                   = 3      // must be changed !
} SignatureAlgorithm;



typedef enum {
  EncryptAlg_undef            = 0,

  EncryptAlg_RSA              = 1,
  EncryptAlg_SHA1             = 2,
  EncryptAlg_TrippleDES       = 3
} EncryptionAlgorithm;

typedef enum {
  SignEmail_undef             = 0,

  SignEmail_auto              = 1,
  SignEmail_confirmOnce       = 2,
  SignEmail_confirmParts      = 3,
  SignEmail_never             = 4
} SignEmail;

typedef enum {
  EncryptEmail_undef          = 0,

  EncryptEmail_auto           = 1,
  EncryptEmail_confirmOnce    = 2,
  EncryptEmail_confirmParts   = 3,
  EncryptEmail_never          = 4
} EncryptEmail;

typedef enum {
  CertSrc_undef               = 0,

  CertSrc_Server              = 1,
  CertSrc_Local               = 2,
  CertSrc_ServerLocal         = 3
} CertificateSource;






/*! \ingroup groupGeneral
    \brief This function sets up all internal structures.

   Plugins that need no initialization should provide an empty
   implementation. The method returns \c true if the initialization was
   successful and \c false otherwise. Before this function is called,
   no other plugin functions should be called; the behavior is
   undefined in this case.

   \note This function <b>must</b> be implemented by each plug-in using
   this API specification.
*/
bool initialize();

/*! \ingroup groupGeneral
    \brief This function frees all internal structures.

    Plugins that do not keep any internal structures should provide an
    empty implementation. After this function has been called,
    no other plugin functions should be called; the behavior is
    undefined in this case.

   \note This function <b>must</b> be implemented by each plug-in using
   this API specification.
*/
void deinitialize();

/*! \ingroup groupGeneral
   \brief This function returns \c true if the
          specified feature is available in the plugin, and
          \c false otherwise.

   Not all plugins will support all features; a complete Sphinx
   implementation will support all features contained in the enum,
   however.

   \note This function <b>must</b> be implemented by each plug-in using
   this API specification.
*/
bool hasFeature( Feature );


/*! \ingroup groupDisplay
   \brief Returns stationery to indicate unsafe emails.
*/
void unsafeStationery( void** pixmap, const char** menutext, char* accel,
          const char** tooltip, const char** statusbartext );

/*! \ingroup groupDisplay
   \brief Returns stationery to indicate signed emails.
*/
void signedStationery( void** pixmap, const char** menutext, char* accel,
          const char** tooltip, const char** statusbartext );

/*! \ingroup groupDisplay
   \brief Returns stationery to indicate encrypted emails.
*/
void encryptedStationery( void** pixmap, const char**
          menutext, char* accel,
          const char** tooltip, const char** statusbartext );

/*! \ingroup groupDisplay
   \brief Returns stationery to indicate signed and encrypted emails.
*/
void signedEncryptedStationery( void** pixmap, const char**
          menutext, char* accel,
          const char** tooltip, const char** statusbartext );

/*! \ingroup groupConfigSign
   \brief This function returns an XML representation of a
            configuration dialog for configuring signature
            handling.
            
   The syntax is that of <filename>.ui</filename>
            files as specified in the <emphasis>Indien</emphasis>
            documentation. This function does not execute or show the
            dialog in any way; this is up to the MUA. Also, what the
            MUA makes of the information provided highly depends on
            the MUA itself. A GUI-based MUA will probably create a
            dialog window (possibly integrated into an existing
            configuration dialog in the application), while a
            terminal-based MUA might generate a series of questions or
            a terminal based menu selection.
*/
const char* signatureConfigurationDialog();

/*! \ingroup groupConfigSign
   \brief This function returns an XML representation of a
            configuration dialog for selecting a signature key.
            
   This will typically be used when the user wants to select a
            signature key for one specific message only; the defaults
            are set in the dialog returned by
            signatureConfigurationDialog().
*/
const char* signatureKeySelectionDialog();

/*! \ingroup groupConfigSign
   \brief This function returns an XML representation of a
            configuration dialog for selecting a signature
            algorithm.

   This will typically be used when the user wants
          to select a signature algorithm for one specific message only; the
          defaults are set in the dialog returned by
            signatureConfigurationDialog().
*/
const char* signatureAlgorithmDialog();

/*! \ingroup groupConfigSign
   \brief This function returns an XML representation of a
            configuration dialog for selecting whether an email
            message and its attachments should be sent with or
            without signatures.

   This will typically be used when the
            user wants to select a signature key for one specific
            message only; the defaults are set in the dialog returned
            by signatureConfigurationDialog().
*/
const char* signatureHandlingDialog();

/*! \ingroup groupConfigSign
   \brief Sets the signature key certificate that identifies the
          role of the signer.
*/
void setSignatureKeyCertificate( const char* certificate );

/*! \ingroup groupConfigSign
   \brief Returns the signature key certificate that identifies
            the role of the signer.
*/
const char* signatureKeyCertificate();

/*! \ingroup groupConfigSign
   \brief Sets the algorithm used for signing.
*/
void setSignatureAlgorithm( SignatureAlgorithm );

/*! \ingroup groupConfigSign
   \brief Returns the algorithm used for signing.
*/
SignatureAlgorithm signatureAlgorithm();

/*! \ingroup groupConfigSign
   \brief Sets which certificates should be sent with the
            message.
*/
void setSendCertificates( SendCertificates );
/*! \ingroup groupConfigSign
   \brief Returns which certificates should be sent with the
            message.
*/
SendCertificates sendCertificates();

/*! \ingroup groupConfigSign
   \brief Specifies whether email should be automatically
            signed, signed after confirmation, signed after
            confirmation for each part or not signed at all.
*/
void setSignEmail( SignEmail );

/*! \ingroup groupConfigSign
   \brief Returns whether email should be automatically
            signed, signed after confirmation, signed after
            confirmation for each part or not signed at all.
*/
SignEmail signEmail();

/*! \ingroup groupConfigSign
   \brief Specifies whether sent email messages should be stored
          with or without their signatures.
*/
void setSaveSentSignatures( bool );

/*! \ingroup groupConfigSign
   \brief Returns whether sent email messages should be stored
            with or without their signatures.
*/
bool saveSentSignatures();

/*! \ingroup groupConfigSign
   \brief Specifies whether a warning should be emitted if any
            of the certificates involved in the signing process
            expires in the near future.
*/
void setCertificateExpiryNearWarning( bool );

/*! \ingroup groupConfigSign
   \brief Returns whether a warning should be emitted if any
            of the certificates involved in the signing process
            expires in the near future.
*/
bool certificateExpiryNearWarning();

/*! \ingroup groupConfigSign
   \brief Specifies whether a warning should be emitted if the
            email address of the sender is not contained in the
            certificate.
*/
void setWarnNoCertificate( bool );

/*! \ingroup groupConfigSign
   \brief Returns whether a warning should be emitted if the
            email address of the sender is not contained in the
            certificate.
*/
bool warnNoCertificate();

/*! \ingroup groupConfigSign
   \brief Specifies how often the PIN is requested when
            accessing the secret signature key.
*/
void setNumPINRequests( PinRequests );

/*! \ingroup groupConfigSign
   \brief Returns how often the PIN is requested when
            accessing the secret signature key.
*/
PinRequests numPINRequests();

/*! \ingroup groupConfigSign
   \brief Specifies whether the certificate path should be
            followed to the root certificate or whether locally stored
            certificates may be used.
*/
void setCheckSignatureCertificatePathToRoot( bool );

/*! \ingroup groupConfigSign
   \brief Returns whether the certificate path should be
            followed to the root certificate or whether locally stored
            certificates may be used.
*/
bool checkSignatureCertificatePathToRoot();

/*! \ingroup groupConfigSign
   \brief Specifies whether certificate revocation lists should
            be used.
*/
void setSignatureUseCRLs( bool );

/*! \ingroup groupConfigSign
   \brief Returns whether certificate revocation lists should
            be used.
*/
bool signatureUseCRLs();

/*! \ingroup groupConfigSign
   \brief Specifies whether a warning should be emitted if any
            of the certificates involved in the signing process
            expires in the near future.
*/
void setSignatureCRLExpiryNearWarning( bool );

/*! \ingroup groupConfigSign
   \brief Returns whether a warning should be emitted if any
            of the certificates involved in the signing process
            expires in the near future.
*/
bool signatureCRLExpiryNearWarning();

/*! \ingroup groupConfigSign
   \brief Specifies the number of days which a certificate must
            be valid before it is considered to expire in the near
            future.
*/
void setSignatureCRLNearExpiryInterval( int );

/*! \ingroup groupConfigSign
   \brief Returns the number of days which a certificate must
            be valid before it is considered to expire in the near
            future.
*/
int signatureCRLNearExpiryInterval();


/*! \ingroup groupConfigCrypt
   \brief This function returns an XML representation of a
            configuration dialog for configuring encryption
            handling.
            
   The syntax is that of <filename>.ui</filename>
            files as specified in the <emphasis>Indien</emphasis>
            documentation. This function does not execute or show the
            dialog in any way; this is up to the MUA. Also, what the
            MUA makes of the information provided highly depends on
            the MUA itself. A GUI-based MUA will probably create a
            dialog window (possibly integrated into an existing
            configuration dialog in the application), while a
            terminal-based MUA might generate a series of questions or
            a terminal based menu selection.
*/
const char* encryptionConfigurationDialog();

/*! \ingroup groupConfigCrypt
   \brief This function returns an XML representation of a
            configuration dialog for selecting an encryption
            algorithm.
            
   This will typically be used when the user wants
          to select an encryption algorithm for one specific message only; the
          defaults are set in the dialog returned by
            encryptionConfigurationDialog().
*/
const char* encryptionAlgorithmDialog();

/*! \ingroup groupConfigCrypt
   \brief This function returns an XML representation of a
            configuration dialog for selecting whether an email
            message and its attachments should be encrypted.

   This will typically be used when the
            user wants to select an encryption key for one specific
            message only; the defaults are set in the dialog returned
            by encryptionConfigurationDialog().
*/
const char* encryptionHandlingDialog();

/*! \ingroup groupConfigCrypt
   \brief This function returns an XML representation of a
            dialog that lets the user select the certificate to use
            for encrypting.
            
   If it was not possible to determine the
            correct certificate from the information in the email
            message, the user is presented with a list of possible
            certificates to choose from. If a unique certificate was
            found, this is presented to the user, who needs to confirm
          the selection of the certificate. This procedure is repeated
          for each recipient of the email message.
*/
const char* encryptionReceiverDialog();

/*! \ingroup groupConfigCrypt
   \brief Sets the algorithm used for encrypting.
*/
void setEncryptionAlgorithm( EncryptionAlgorithm );

/*! \ingroup groupConfigCrypt
   \brief Returns the algorithm used for encrypting.
*/
EncryptionAlgorithm encryptionAlgorithm();

/*! \ingroup groupConfigCrypt
   \brief Specifies whether email should be automatically
            encrypted, encrypted after confirmation, encrypted after
            confirmation for each part or not encrypted at all.
*/
void setEncryptEmail( EncryptEmail );

/*! \ingroup groupConfigCrypt
   \brief Returns whether email should be automatically
            encrypted, encrypted after confirmation, encrypted after
            confirmation for each part or not encrypted at all.
*/
EncryptEmail encryptEmail();

/*! \ingroup groupConfigCrypt
   \brief Specifies whether encrypted email messages should be
            stored encrypted or decrypted.
*/
void setSaveMessagesEncrypted( bool );

/*! \ingroup groupConfigCrypt
   \brief Returns whether encrypted email messages should be stored
            encrypted or decrypted.
*/
bool saveMessagesEncrypted();

/*! \ingroup groupConfigCrypt
   \brief Specifies whether the certificate path should be
            followed to the root certificate or whether locally stored
            certificates may be used.
*/
void setCheckEncryptionCertificatePathToRoot( bool );

/*! \ingroup groupConfigCrypt
   \brief Returns whether the certificate path should be
            followed to the root certificate or whether locally stored
            certificates may be used.
*/
bool checkEncryptionCertificatePathToRoot();

/*! \ingroup groupConfigCrypt
   \brief Specifies whether certificate revocation lists should
            be used.
*/
void setEncryptionUseCRLs( bool );

/*! \ingroup groupConfigCrypt
   \brief Returns whether certificate revocation lists should
            be used.
*/
bool encryptionUseCRLs();

/*! \ingroup groupConfigCrypt
   \brief Specifies whether a warning should be emitted if any
            of the certificates involved in the signing process
            expires in the near future.
*/
void setEncryptionCRLExpiryNearWarning( bool );

/*! \ingroup groupConfigCrypt
   \brief Returns whether a warning should be emitted if any
            of the certificates involved in the signing process
            expires in the near future.
*/
bool encryptionCRLExpiryNearWarning();

/*! \ingroup groupConfigCrypt
   \brief Specifies the number of days which a certificate must
            be valid before it is considered to expire in the near
            future.
*/
void setEncryptionCRLNearExpiryInterval( int );

/*! \ingroup groupConfigCrypt
   \brief Returns the number of days which a certificate must
            be valid before it is considered to expire in the near
            future.
*/
int encryptionCRLNearExpiryInterval();


/*! \ingroup groupConfigDir
   \brief This function returns an XML representation of a
            configuration dialog for selecting a directory
            server.
*/
const char* directoryServiceConfigurationDialog();

/*! \ingroup groupConfigDir
   \brief Appends a directory server to the list of directory
            services searched.

   Will mainly be used for restoring
            configuration data; interactive configuration will be done
            via the configuration dialog returned by
            \c directoryServiceConfigurationDialog().
*/
void appendDirectoryServer( const char* servername, int port );




/*! \ingroup groupConfigDir
    Dummy!!  To be replaced by real structure information...
*/
struct DirectoryServer {
  struct DirectoryServer *prev;
  struct DirectoryServer *next;
  int data;
};

/*! \ingroup groupConfigDir
   \brief Specifies a list of directory servers.

   Will mainly be used for restoring
            configuration data; interactive configuration will be done
            via the configuration dialog returned by
            \c directoryServiceConfigurationDialog().
*/
void setDirectoryServers( struct DirectoryServer );

/*! \ingroup groupConfigDir
   \brief Returns the list of directory servers.

   Will mainly be used for saving configuration data; interactive
            configuration will be done via the configuration dialog
            returned by
            \c directoryServiceConfigurationDialog().
*/
struct DirectoryServer * directoryServers();

/*! \ingroup groupConfigDir
   \brief Specifies whether certificates should be retrieved
            from a directory server, only locally, or both.
*/
void setCertificateSource( CertificateSource );

/*! \ingroup groupConfigDir
   \brief Returns whether certificates should be retrieved
            from a directory server, only locally, or both.
*/
CertificateSource certificateSource();

/*! \ingroup groupConfigDir
   \brief Specifies whether certificates should be retrieved
            from a directory server, only locally, or both.
*/
void setCRLSource( CertificateSource );

/*! \ingroup groupConfigDir
   \brief Returns whether certificates should be retrieved
            from a directory server, only locally, or both.
*/
CertificateSource crlSource();


/*! \ingroup groupCertHand
   \brief Returns \c true if and only if the
          certificates in the certificate chain starting at
          \c certificate are valid.
          
   If \c level is non-null, the parameter contains
          the degree of trust on a backend-specific scale. In an X.509
          implementation, this will either be \c 1
          (valid up to the root certificate) or \c 0
          (not valid up to the root certificate).
*/
bool certificateValidity( const char* certificate, int* level );


/*! \ingroup groupSignAct
   \brief Signs a message \c cleartext and returns
          in \c ciphertext the message including
          signature.

   The signature role is specified by
          \c certificate. If \c certificate is \c NULL,
          the default certificate is used.
*/
bool signMessage( const char* cleartext,
                  const char** ciphertext,
                  const char* certificate );


/*! \ingroup groupSignAct
    Dummy!!  To be replaced by real structure information...
*/
struct SignatureMetaData {
  int data;
};

/*! \ingroup groupSignAct
   \brief Checks whether the signature of a message is
          valid. \c ciphertext specifies the message
          as it was received by the MUA, \c cleartext
          is the message with the signature(s) removed.

   Depending on the configuration, MUAs might not need to use this.
   If \c sigmeta is non-null, the
          \c SignatureMetaData object pointed to will
          contain meta information about the signature after the
          function call.
*/
bool checkMessageSignature( const char* ciphertext,
                            const char** cleartext,
                            struct SignatureMetaData* sigmeta );

/*! \ingroup groupSignAct
   \brief Stores the certificates that follow with the message
          \c ciphertext locally.
*/
bool storeCertificatesFromMessage( const char* ciphertext );


/*! \ingroup groupCryptAct
   \brief Encrypts an email message in
          \c cleartext according to the current
          settings (algorithm, etc.) and returns it in
          \c ciphertext.

   If the message could be encrypted, the function returns
          \c true, otherwise
          \c false.
*/
bool encryptMessage( const char* cleartext, const char** ciphertext );

/*! \ingroup groupCryptAct
   \brief Combines the functionality of
          \c encryptMessage() and
          \c signMessage().

   If \c certificate is \c NULL,
          the default certificate will be used.  If
          \c sigmeta is non-null, the
          \c SignatureMetaData object pointed to will
          contain meta information about the signature after the
          function call.
*/
bool encryptAndSignMessage( const char* cleartext,
                            const char** ciphertext,
                            const char* certificate,
                            struct SignatureMetaData* sigmeta );

/*! \ingroup groupCryptAct
   \brief Tries to decrypt an email message
          \c ciphertext and returns the decrypted
          message in \c cleartext.

   The \c certificate is used for decryption. If
          the message could be decrypted, the function returns
          \c true, otherwise
          \c false.
*/
bool decryptMessage( const char* ciphertext, const
          char** cleartext, const char* certificate );

/*! \ingroup groupCryptAct
   \brief Combines the functionality of
          \c checkMessageSignature() and
          \c decryptMessage().

   If \c certificate is \c NULL,
          the default certificate will be used.  If
          \c sigmeta is non-null, the
          \c SignatureMetaData object pointed to will
          contain meta information about the signature after the
          function call.
*/
bool decryptAndCheckMessage( const char* ciphertext,
                             const char** cleartext,
                             const char* certificate,
                             struct SignatureMetaData* sigmeta );


/*! \ingroup groupCertAct
   \brief This function returns an XML representation of a dialog
          that can be used to fill in the data for requesting a
          certificate (which in turn is done with the function
          \c requestCertificate() described
          next.
*/
const char* requestCertificateDialog();

/*! \ingroup groupCertAct
   \brief Generates a prototype certificate with the data provided
        in the first four parameters and sends it via email to the CA
          specified in \c ca_address.
*/
bool requestDecentralCertificate( const char* name, const char*
          email, const char* organization, const char* department,
          const char* ca_address );

/*! \ingroup groupCertAct
   \brief Requests a certificate in a PSE from the CA
          specified in \c ca_address.
*/
bool requestCentralCertificateAndPSE( const char* name,
          const char* email, const char* organization, const char* department,
          const char* ca_address );

/*! \ingroup groupCertAct
   \brief Creates a local PSE.
*/
bool createPSE();

/*! \ingroup groupCertAct
   \brief Parses and adds a certificate returned by a CA upon
          request with
          \c requestDecentralCertificate() or
          \c requestCentralCertificate().

   If the certificate was requested with
          \c requestCentralCertificate(), the
          certificate returned will come complete with a PSE which is
          also registered with this method.
*/
bool registerCertificate( const char* );

/*! \ingroup groupCertAct
   \brief Requests the prolongation of the certificate
          \c certificate from the CA
          \c ca_address.
*/
bool requestCertificateProlongation( const char*
          certificate, const char* ca_address );

/*! \ingroup groupCertAct
   \brief Returns an HTML 2-formatted string that describes the
          certificate chain of the user's certificate.
          
   Data displayed is at least the issuer of the certificate, the serial number
        of the certificate, the owner of the certificate, the checksum
        of the certificate, the validity duration of the certificate,
          the usage of the certificate, and the contained email
          addresses, if any.
*/
const char* certificateChain();

/*! \ingroup groupCertAct
   \brief Deletes the specified user certificate from the current
          PSE.
*/
bool deleteCertificate( const char* certificate );

/*! \ingroup groupCertAct
   \brief Archives the specified user certificate in the current PSE.

   The certificate cannot be used any longer after this
          operation unless it is unarchived.
*/
bool archiveCertificate( const char* certificate );


/*! \ingroup groupCRLAct
   \brief Returns a HTML 2-formatted string that describes the
          CRL, suitable for display in the MUA.
*/
const char* displayCRL();

/*! \ingroup groupCRLAct
   \brief Manually update the CRL. CRLs will also be automatically
        updated on demand by the backend.
        
   If there is a local version of a CRL saved, it will be overwritten
   with the new CRL from the CA.
*/
void updateCRL();

#ifdef __cplusplus
}
#endif
#endif /*CRYPTPLUG_H*/

