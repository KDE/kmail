/**
 * cryptplugwrapper.h
 *
 * Copyright (c) 2001 Karl-Heinz Zimmer, Klaraelvdalens Datakonsult AB
 *
 * This CRYPTPLUG wrapper interface is based on cryptplug.h by
 * Karl-Heinz Zimmer which is based on 'The Aegypten Plugin API' as
 * specified by Matthias Kalle Dalheimer, Klaraelvdalens Datakonsult AB,
 * see file mua-integration.sgml located on Aegypten CVS:
 *          http://www.gnupg.org/aegypten/development.en.html
 *
 * purpose: Wrap up all Aegypten Plugin API functions in one C++ class
 *          for usage by KDE programs, e.g. KMail (or KMime, resp.)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef cryptplugwrapper_h
#define cryptplugwrapper_h



/*
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                        *
 *  This file's source comments are optimized for processing by Doxygen.  *
 *                                                                        *
 *  To obtain best results please get an updated version of Doxygen,      *
 *  for sources and binaries goto http://www.doxygen.org/index.html       *
 *                                                                        *
  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                                                                          *
                                                                          */


#include <kapplication.h>
#include <kdebug.h>
#include "cryptplug.h"




/*! \file cryptplugwrapper.h
    \brief C++ wrapper for the CRYPTPLUG library API.

    This CRYPTPLUG wrapper interface is based on cryptplug.h by
    Karl-Heinz Zimmer which is based on 'The Aegypten Plugin API' as
    specified by Matthias Kalle Dalheimer, Klaraelvdalens Datakonsult AB,
    see file mua-integration.sgml located on Aegypten CVS:
             http://www.gnupg.org/aegypten/development.en.html

    purpose: Wrap up all Aegypten Plugin API functions in one C++ class
             for usage by KDE programs, e.g. KMail (or KMime, resp.)

    CRYPTPLUG is an independent cryptography plug-in API
    developed for Sphinx-enabeling KMail and Mutt.

    CRYPTPLUG was designed for the Aegypten project, but it may
    be used by 3rd party developers as well to design pluggable
    crypto backends for the above mentioned MUAs.

    \note All string parameters appearing in this API are to be
    interpreted as UTF-8 encoded.

    \see cryptplugwrapper.cpp
*/

/*! \defgroup groupAdmin Constructor, destructor and setting of 'active' flag

    The functions in this section are used for general administration of
    this CRYPTPLUG wrapper class and for maintaining a separate \c active flag
    for environments using more than one CRYPTPLUG library simultaneously.
*/

/*! \defgroup groupGeneral Loading and Unloading the Plugin, General Functionality

    The functions in this section are used for loading and
    unloading the respective CRYPTPLUG library, for (re)setting
    it's internal data structures and for retrieving information
    on the implementation state of all functions covered by the CRYPTPLUG API.
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


/*! \defgroup groupSignCryptAct Signing and Encrypting Actions

    This section describes methods and structures
    used for signing and/or encrypting your mails.
*/


/*! \defgroup groupSignAct Signature Actions
    \ingroup groupSignCryptAct

    This section describes methods that are used for working
    with signatures.
*/

/*! \defgroup groupCryptAct Encryption and Decryption
    \ingroup groupSignCryptAct

    The following methods are used to encrypt and decrypt
    email messages.
*/

/*! \defgroup groupCertAct Certificate Handling Actions

    The functions in this section provide local certificate management.
*/

/*! \defgroup groupCRLAct CRL Handling Actions

    This section describes functions for managing CRLs.
*/

/*! \defgroup groupAdUsoInterno Important functions to be used by plugin implementors ONLY.

    This section describes functions that have to be used by
    plugin implementors but should not be used by plugin users
    directly.

    If you are not planning to write your own cryptography
    plugin <b>you should ignore this</b> section!
*/



class CryptPlugWrapper;
/*! \ingroup groupSignCryptAct
  \brief This class provides C++ access to the StructuringInfo helper
         struct that is specified in cryptplug.h to hold information
         returned by signing and by encrypting functions.

  Use this information to compose a MIME object containing signed
  and/or encrypted content (or to build a text frame around your
  flat non-MIME message body, resp.)

  \note This class is different from the respective cryptplug.h class
  because this one takes care for freeing the char** members' memory
  automatically. You must <b>not</b> call the \c free function for
  any of it's members - just ignore the advise given in the
  cryptplug.h documentation!

  <b>If</b> value returned in \c makeMimeObject is <b>TRUE</b> the
  text strings returned in \c contentTypeMain and \c contentDispMain
  and \c contentTEncMain (and, if required, \c content[..]Version and
  \c bodyTextVersion and \c content[..]Sig) should be used to compose
  a respective MIME object.<br>
  If <b>FALSE</b> the texts returned in \c flatTextPrefix and
  \c flatTextSeparator and \c flatTextPostfix are to be used instead.<br>
  Allways <b>either</b> the \c content[..] and \c bodyTextVersion
  parameters <b>or</b> the \c flatText[..] parameters are holding
  valid data - never both of them may be used simultaneously
  as plugins will just ignore the parameters not matching their
  \c makeMimeObject setting.

  When creating your MIME object please observe these common rules:
  \li Parameters named \c contentType[..] and \c contentDisp[..] and
  \c contentTEnc[..] will return the values for the respective MIME
  headers 'Content-Type' and 'Content-Disposition' and
  'Content-Transfer-Encoding'. The following applies to these parameters:
  \li The relevant MIME part may <b>only</b> be created if the respective
  \c contentType[..] parameter is holding a non-zero-length string. If the
  \c contentType[..] parameter value is invalid or holding an empty string
  the respective \c contentDisp[..] and \c contentTEnc[..] parameters
  should be ignored.
  \li If the respective \c contentDisp[..] or \c contentTEnc[..] parameter
  is NULL or holding a zero-length string it is up to you whether you want
  to add the relevant MIME header yourself, but since it in in the
  responsibility of the plugin implementors to provide you with all
  neccessary 'Content-[..]' header information you should <b>not need</b>
  to define them if they are not returned by the signing or encrypting
  function - otherwise this may be considered as a bug in the plugin and
  you could report the missing MIME header information to the address
  returned by the \c bugURL() function.

  If \c makeMultiMime returns FALSE the \c contentTypeMain returned must
  not be altered but used to specify a single part mime object holding the
  code bloc, e.g. this is used for 'enveloped-data' single part MIME
  objects. In this case you should ignore both the \c content[..]Version
  and \c content[..]Code parameters.

  If \c makeMultiMime returns TRUE also the following rules apply:
  \li If \c includeCleartext is TRUE you should include the cleartext
  as first part of our multipart MIME object, typically this is TRUE
  when signing mails but FALSE when encrypting.
  \li The \c contentTypeMain returned typically starts with
  "multipart/" while providing a "protocol" and a "micalg" parameter: just
  add an appropriate \c "; boundary=[your \c boundary \c string]" to get
  the complete Content-Type value to be used for the MIME object embedding
  both the signed part and the signature part (or - in case of
  encrypting - the version part and the code part, resp.).
  \li If \c contentTypeVersion is holding a non-zero-length string an
  additional MIME part must added immediately before the code part, this
  version part's MIME headers must have the unaltered values of
  \c contentTypeVersion and (if they are holding non-zero-length strings)
  \c contentDispVersion and \c contentTEncVersion, the unaltered contents
  of \c bodyTextVersion must be it's body.
  \li The value returned in \c contentTypeCode is specifying the complete
  Content-Type to be used for this multipart MIME object's signature part
  (or - in case of encrypting - for the code part following after the
  version part, resp.), you should not add/change/remove anything here
  but just use it's unaltered value for specifying the Content-Type header
  of the respective MIME part.
  \li The same applies to the \c contentDispCode value: just use it's
  unaltered value to specify the Content-Disposition header entry of
  the respective MIME part.
  \li The same applies to the \c contentTEncCode value: just use it's
  unaltered value to specify the Content-Transfer-Encoding header of
  the respective MIME part.

  <b>If</b> value returned in \c makeMimeObject is <b>FALSE</b> the
  text strings returned in \c flatTextPrefix and \c flatTextPostfix
  should be used to build a frame around the cleartext and the code
  bloc holding the signature (or - in case of encrypting - the encoded
  data bloc, resp.).<br>
  If \c includeCleartext is TRUE this frame should also include the
  cleartext as first bloc, this bloc should be divided from the code bloc
  by the contents of \c flatTextSeparator - typically this is used for
  signing but not when encrypting.<br>
  If \c includeCleartext is FALSE you should ignore both the cleartext
  and the \c flatTextSeparator parameter.

  <b>How to use StructuringInfoWrapper data in your program:</b>
  \li To compose a signed message please act as described below.
  \li For constructing an encrypted message just replace the
  \c signMessage() call by the respective \c encryptMessage() call
  and then proceed exactly the same way.
  \li In any case make <b>sure</b> to free your \c ciphertext when
  you are done with processing the data returned by the signing
  (or encrypting, resp.) function.

\verbatim

    char* ciphertext;
    StructuringInfoWrapper structInf;

    if( ! signMessage( cleartext, &ciphertext, certificate,
                      structInf ) ) {

        myErrorDialog( "Error: could not sign the message!" );

    } else {
      if( structInf.data.makeMimeObject ) {

        // Build the main MIME object.
        // This is done by
        // using the header values returned in
        // structInf.data.contentTypeMain and in
        // structInf.data.contentDispMain and in
        // structInf.data.contentTEncMain.
        ..

        if( ! structInf.data.makeMultiMime ) {

          // Build the main MIME object's body.
          // This is done by
          // using the code bloc returned in
          // ciphertext.
          ..

        } else {

          // Build the encapsulated MIME parts.
          if( structInf.data.includeCleartext ) {

            // Build a MIME part holding the cleartext.
            // This is done by
            // using the original cleartext's headers and by
            // taking it's original body text.
            ..

          }
          if(    structInf.data.contentTypeVersion
              && 0 < strlen( structInf.data.contentTypeVersion ) ) {

            // Build a MIME part holding the version information.
            // This is done by
            // using the header values returned in
            // structInf.data.contentTypeVersion and
            // structInf.data.contentDispVersion and
            // structInf.data.contentTEncVersion and by
            // taking the body contents returned in
            // structInf.data.bodyTextVersion.
            ..

          }
          if(    structInf.data.contentTypeCode
              && 0 < strlen( structInf.data.contentTypeCode ) ) {

            // Build a MIME part holding the code information.
            // This is done by
            // using the header values returned in
            // structInf.data.contentTypeCode and
            // structInf.data.contentDispCode and
            // structInf.data.contentTEncCode and by
            // taking the body contents returned in
            // ciphertext.
            ..

          } else {

            // Plugin error!
            myErrorDialog( "Error: Cryptography plugin returned a main"
                          "Content-Type=Multipart/.. but did not "
                          "specify the code bloc's Content-Type header."
                          "\nYou may report this bug:"
                          "\n" + cryptplug.bugURL() );
          }
        }
      } else  {

        // Build a plain message body
        // based on the values returned in structInf.
        // Note: We do _not_ insert line breaks between the parts since
        //       it is the plugin job to provide us with ready-to-use
        //       texts containing all neccessary line breaks.
        strcpy( myMessageBody, structInf.data.plainTextPrefix );
        if( structInf.data.includeCleartext ) {
          strcat( myMessageBody, cleartext );
          strcat( myMessageBody, structInf.data.plainTextSeparator );
        }
        strcat( myMessageBody, *ciphertext );
        strcat( myMessageBody, structInf.data.plainTextPostfix );
      }

      // free the memory that was allocated
      // for the ciphertext
      free( ciphertext );
    }

\endverbatim

  \see signMessage, encryptMessage, encryptAndSignMessage
*/
class StructuringInfoWrapper {
public:
  StructuringInfoWrapper( CryptPlugWrapper* wrapper );
  virtual ~StructuringInfoWrapper();
  virtual void reset();
  StructuringInfo data;
private:
  virtual void initMe();
  virtual void freeMe();
  bool _initDone;
  CryptPlugWrapper* _wrapper;
};




/*!
    \brief This class provides C++ access to the CRYPTPLUG API.
*/
class CryptPlugWrapper : public QObject
{
    Q_OBJECT

public:

    /*! \ingroup groupGeneral

        \brief Current initialization state.

        This flag holding status of previous call of initialize function.
        If initialize was not called before return value will be
        \c CryptPlugInit_undef.

        \sa initStatus, initialize
    */
    typedef enum {
        InitStatus_undef         = 0,

        InitStatus_Ok            = 1,
        InitStatus_NoLibName     = 2,
        InitStatus_LoadError     = 0x1000,
        InitStatus_InitError     = 0x2000
    } InitStatus;


    /*! \ingroup groupAdmin
        \brief Constructor of CRYPTPLUG wrapper class.

        This constructor does <b>not</b> call the initialize() method
        but just stores some information for later use.

        \note Since more than one crypto plug-in might be specified (using
              multiple instances of the warpper class) it is neccessary to
              set \c active at least one them. Only wrappers that have been
              activated may be initialized or configured or used to perform
              crypto actions.

        \param parent  The parent widget to be used for displaying dialogs.
                       If this parameter is NULL the desktop is used as
                       the dialogs' parent widget.
        \param name    The external name that is visible in lists, messages,
                       etc.
        \param libName Complete path+name of CRYPTPLUG library that is to
                       be used by this instance of CryptPlugWrapper.
        \param update  the URL from where updates can be downloaded
        \param active  Specify whether the relative library is to be used
                       or not.

        \sa ~CryptPlugWrapper, setActive, active, initialize, deinitialize
        \sa initStatus
    */
    CryptPlugWrapper( QWidget*       parent,
                      const QString& name,
                      const QString& libName,
                      const QString& update,
                      bool           active = false );

    /*! \ingroup groupAdmin
        \brief Destructor of CRYPTPLUG wrapper class.

        This destructor <b>does</b> call the deinitialize() method in case
        this was not done by explicitely calling it before.

        \sa deinitialize, initialize, CryptPlugWrapper(), setActive, active
        \sa
    */
    virtual ~CryptPlugWrapper();

    /*! \ingroup groupAdmin
        \brief Set this CRYPTPLUG wrapper's internal \c active flag.

        Since more than one crypto plug-in might be specified (using
        multiple instances of the warpper class) it is neccessary to
        set \c active at least one them. Only wrappers that have been
        activated may be initialized or configured or used to perform
        crypto actions.

        This flag may be set in the constructor or by calling setActive().

        \note Deactivating does <b>not</b> mean resetting the internal
              structures - if just prevents the normal functions from
              being called erroneously. When deactivated only the following
              functions are operational: constructor , destructor ,
              setActive , active, setLibName , libName , initStatus;
              calling other functions will be ignored and their return
              values will be undefined.

        \param active  Specify whether the relative library is to be used
                       or not.

        \sa active, CryptPlugWrapper(), ~CryptPlugWrapper
        \sa deinitialize, initialize, initStatus
    */
    void setActive( bool active );

    /*! \ingroup groupAdmin
        \brief Returns this CRYPTPLUG wrapper's internal \c active flag.

        \return  whether the relative library is to be used or not.

        \sa setActive
    */
    bool active() const;


    /*! \ingroup groupAdmin
        \brief Set the CRYPTPLUG library name.

        Complete path+name of CRYPTPLUG library that is to
                       be used by this instance of CryptPlugWrapper.

        This name may be set in the constructor or by calling setLibName().

        \note Setting/changing the library name may only be done when
              the initStatus() is <b>not</b> \c InitStatus_Ok.
              If you want to change the name of the library after
              successfully having called initialize() please make
              sure to unload it by calling the deinitialize() function.

        \param libName libName Complete path+name of CRYPTPLUG library
                       that is to be used by this CryptPlugWrapper.

        \return whether the library name could be changed; library name
                can only be changed when library is not initialized - see
                above 'note'.

        \sa libName, CryptPlugWrapper(), ~CryptPlugWrapper
        \sa deinitialize, initialize, initStatus
    */
    bool setLibName( const QString& libName );

    /*! \ingroup groupAdmin
        \brief Returns the CRYPTPLUG library name.

        \return  the complete path+name of CRYPTPLUG library that is to
                 be used by this instance of CryptPlugWrapper.

        \sa setLibName
    */
    QString libName() const;


    /*! \ingroup groupAdmin
      \brief Returns the external name.
      \return the external name used for display purposes
    */
    QString displayName() const;


    /*! \ingroup groupAdmin
      \brief Returns the update URL.
      \return the update URL
    */
    QString updateURL() const;



    /*! \ingroup groupGeneral
    \brief This function does two things: (a) load the lib and (b) set up all internal structures.

        The method tries to load the CRYPTPLUG library specified
        in the constructor and returns \c true if the both <b>loading
        and initializing</b> the internal data structures was successful
        and \c false otherwise. Before this function is called,
        no other plugin functions should be called; the behavior is
        undefined in this case, this rule does not apply to the functions
        \c setActive() and \c setLibName().

        \param initStatus will receive the resulting InitStatus if not NULL
        \param errorMsg will receive the system error message if not NULL

        \sa initStatus, deinitialize, CryptPlugWrapper(), ~CryptPlugWrapper
        \sa setActive, active
    */
    bool initialize( InitStatus* initStatus, QString* errorMsg );

    /*! \ingroup groupGeneral
    \brief This function unloads the lib and frees all internal structures.

        After this function has been called, no other plugin functions
        should be called; the behavior is undefined in this case.

        \note Deinitializing sets the internal initStatus value back
              to \c InitStatus_undef.

        \sa initStatus, initialize, CryptPlugWrapper, ~CryptPlugWrapper
        \sa setActive, active
    */
    void deinitialize();

    /*! \ingroup groupGeneral
        \brief Returns this CRYPTPLUG wrapper's initialization state.

        \param errorMsg receives the last system error message, this value
        should be ignored if InitStatus value equals \c InitStatus_Ok.

        \return  whether the relative library was loaded and initialized
                 correctly

        \sa initialize, deinitialize, CryptPlugWrapper(), ~CryptPlugWrapper
        \sa setActive, active
    */
    InitStatus initStatus( QString* errorMsg ) const;


    /*! \ingroup groupGeneral
        \brief This function returns \c true if the
            specified feature is available in the plugin, and
            \c false otherwise.

        Not all plugins will support all features; a complete Sphinx
        implementation will support all features contained in the enum,
        however.

        \note In case this function cannot be executed the system's error
        message may be retrieved by calling initStatus( QString* ).

        \return  whether the relative feature is implemented or not
    */
    bool hasFeature( Feature );


    /*! \ingroup groupGeneral
        \brief This function returns a URL to be used for reporting a bug that
              you found (or suspect, resp.) in this cryptography plug-in.

      If the plugins for some reason cannot specify an appropriate URL you
      should at least be provided with a text giving you some advise on
      how to report a bug.

      \note This function <b>must</b> be implemented by each plug-in using
      this API specification.
    */
    const char* bugURL();


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
                files as specified in the <emphasis>Imhotep</emphasis>
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
      \brief Specifies whether a warning should be emitted when the user
      tries to send an email message unsigned.
    */
    void setWarnSendUnsigned( bool );


    /*! \ingroup groupConfigSign
      \brief Returns whether a warning should be emitted when the user
      tries to send an email message unsigned.
    */
    bool warnSendUnsigned();


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

    /*!
      \ingroup groupConfigSign
      \brief Returns true if the specified email address is contained
      in the specified certificate.
    */
    bool isEmailInCertificate( const char* email, const char* certificate );

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
      \brief Specifies the interval in minutes the PIN must be reentered if
      numPINRequests() is PinRequest_AfterMinutes.
    */
    void setNumPINRequestsInterval( int );


    /*! \ingroup groupConfigSign
      \brief Returns the interval in minutes the PIN must be reentered if
      numPINRequests() is PinRequest_AfterMinutes.
    */
    int numPINRequestsInterval();


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
      \brief Specifies whether a warning should be emitted if the
      signature certificate expires in the near future.
    */
    void setSignatureCertificateExpiryNearWarning( bool );

    /*! \ingroup groupConfigSign
      \brief Returns whether a warning should be emitted if
      the signature certificate expires in the near future.
    */
    bool signatureCertificateExpiryNearWarning( void );

    /*! \ingroup groupConfigSign
      \brief Returns the number of days that are left until the
      specified certificate expires.
      \param certificate the certificate to check
    */
    int signatureCertificateDaysLeftToExpiry( const char* certificate );


    /*! \ingroup groupConfigSign
      \brief Specifies the number of days which a signature certificate must
      be valid before it is considered to expire in the near
      future.
    */
    void setSignatureCertificateExpiryNearInterval( int );

    /*! \ingroup groupConfigSign
      \brief Returns the number of days which a signature certificate must
      be valid before it is considered to expire in the near
      future.
    */
    int signatureCertificateExpiryNearInterval( void );

    /*! \ingroup groupConfigSign
      \brief Specifies whether a warning should be emitted if the
      CA certificate expires in the near future.
    */
    void setCACertificateExpiryNearWarning( bool );

    /*! \ingroup groupConfigSign
      \brief Returns whether a warning should be emitted if
      the CA certificate expires in the near future.
    */
    bool caCertificateExpiryNearWarning( void );

    /*! \ingroup groupConfigSign
      \brief Returns the number of days that are left until the
      CA certificate of the specified certificate expires.
      \param certificate the certificate to check
    */
    int caCertificateDaysLeftToExpiry( const char* certificate );

    /*! \ingroup groupConfigSign
      \brief Specifies the number of days which a CA certificate must
      be valid before it is considered to expire in the near
      future.
    */
    void setCACertificateExpiryNearInterval( int );

    /*! \ingroup groupConfigSign
      \brief Returns the number of days which a CA certificate must
      be valid before it is considered to expire in the near
      future.
    */
    int caCertificateExpiryNearInterval( void );

    /*! \ingroup groupConfigSign
      \brief Specifies whether a warning should be emitted if the
      root certificate expires in the near future.
    */
    void setRootCertificateExpiryNearWarning( bool );

    /*! \ingroup groupConfigSign
      \brief Returns whether a warning should be emitted if
      the root certificate expires in the near future.
    */
    bool rootCertificateExpiryNearWarning( void );

    /*! \ingroup groupConfigSign
      \brief Returns the number of days that are left until the
      root certificate of the specified certificate expires.
      \param certificate the certificate to check
    */
    int rootCertificateDaysLeftToExpiry( const char* certificate );

    /*! \ingroup groupConfigSign
      \brief Specifies the number of days which a root certificate must
      be valid before it is considered to expire in the near
      future.
    */
    void setRootCertificateExpiryNearInterval( int );

    /*! \ingroup groupConfigSign
      \brief Returns the number of days which a signature certificate must
      be valid before it is considered to expire in the near
      future.
    */
    int rootCertificateExpiryNearInterval( void );


    /*! \ingroup groupConfigCrypt
      \brief This function returns an XML representation of a
      configuration dialog for configuring encryption
      handling.

    The syntax is that of <filename>.ui</filename>
                files as specified in the <emphasis>Imhotep</emphasis>
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

    /*! \ingroup groupConfigSign
      \brief Specifies whether a warning should be emitted when the user
      tries to send an email message unencrypted.
    */
    void setWarnSendUnencrypted( bool );


    /*! \ingroup groupConfigSign
      \brief Returns whether a warning should be emitted when the user
      tries to send an email message unencrypted.
    */
    bool warnSendUnencrypted();


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
      \brief Specifies whether the certificate path should be checked
      during encryption.
    */
    void setCheckCertificatePath( bool );

    /*! \ingroup groupConfigCrypt
      \brief Returns whether the certificate path should be checked
      during encryption.
    */
    bool checkCertificatePath();


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
      \brief Specifies whether a warning should be emitted if the
      certificate of the receiver expires in the near future.
    */
    void setReceiverCertificateExpiryNearWarning( bool );

    /*! \ingroup groupConfigCrypt
      \brief Returns whether a warning should be emitted if the
      certificate of the receiver expires in the near future.
    */
    bool receiverCertificateExpiryNearWarning();

    /*! \ingroup groupConfigCrypt
      \brief Returns the number of days until the specified receiver
      certificate expires.
    */
    int receiverCertificateDaysLeftToExpiry( const char* certificate );

    /*! \ingroup groupConfigCrypt
      \brief Specifies the number of days which a receiver certificate
      must be valid before it is considered to expire in the near future.
    */
    void setReceiverCertificateExpiryNearWarningInterval( int );

    /*! \ingroup groupConfigCrypt
      \brief Returns the number of days which a receiver certificate
      must be valid before it is considered to expire in the near future.
    */
    int receiverCertificateExpiryNearWarningInterval();


    /*! \ingroup groupConfigCrypt
      \brief Specifies whether a warning should be emitted if
      a certificate in the chain expires in the near future.
    */
    void setCertificateInChainExpiryNearWarning( bool );

    /*! \ingroup groupConfigCrypt
      \brief Returns whether a warning should be emitted if a
      certificate in the chain expires in the near future.
    */
    bool certificateInChainExpiryNearWarning();

    /*! \ingroup groupConfigCrypt
      \brief Returns the number of days until the first certificate in
      the chain of the receiver certificate expires.
    */
    int certificateInChainDaysLeftToExpiry( const char* certificate );


    /*! \ingroup groupConfigCrypt
      \brief Specifies the number of days which a certificate in the chain
      must be valid before it is considered to expire in the near future.
    */
    void setCertificateInChainExpiryNearWarningInterval( int );

    /*! \ingroup groupConfigCrypt
      \brief Returns the number of days which a certificate in the chain
      must be valid before it is considered to expire in the near future.
    */
    int certificateInChainExpiryNearWarningInterval();


    /*! \ingroup groupConfigCrypt
      \brief Specifies whether a warning is emitted if the email address
      of the receiver does not appear in the certificate.
    */
    void setReceiverEmailAddressNotInCertificateWarning( bool );

    /*! \ingroup groupConfigCrypt
      \brief Returns whether a warning is emitted if the email address
      of the receiver does not appear in the certificate.
    */
    bool receiverEmailAddressNotInCertificateWarning();


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


    /*! \ingroup groupConfigCrypt
      \brief Returns the number of days the currently active certification
      list is still valid.
    */
    int encryptionCRLsDaysLeftToExpiry();

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
    void appendDirectoryServer( const char* servername, int port,
                                const char* description );




    /*! \ingroup groupConfigDir
        Dummy!!  To be replaced by real structure information...
    */
    struct DirectoryServer {
        char* servername; // utf8
        int port;
        char* description; // utf8
    };

    /*! \ingroup groupConfigDir
    \brief Specifies a list of directory servers.

    Will mainly be used for restoring
                configuration data; interactive configuration will be done
                via the configuration dialog returned by
                \c directoryServiceConfigurationDialog().
    */
    void setDirectoryServers( DirectoryServer[],
                              unsigned int );

    /*! \ingroup groupConfigDir
    \brief Returns the list of directory servers.

    Will mainly be used for saving configuration data; interactive
                configuration will be done via the configuration dialog
                returned by
                \c directoryServiceConfigurationDialog().
    */
    DirectoryServer* directoryServers( int* numServers );

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
    \brief Specifies whether CRLs (certificate revocation lists)
           should be retrieved from a directory server, only
           locally, or both.
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
              in \c *ciphertext the signature data bloc that
              is to be added to the message. The length returned
              in \c *cipherLen tells you the size (==amount of bytes)
              of the ciphertext, if the structuring information
              would return with contentTEncCode set to "base64"
              the ciphertext might contain a char 0x00
              and has to be converted into base64 before sending.

      The signature role is specified by \c certificate.
      If \c certificate is \c NULL, the default certificate is used.

      If the message could be signed, the function returns
              \c true, otherwise
              \c false.

      Use the StructuringInfoWrapper data returned in parameter
      \c structuring to find out how to build the respective
      MIME object (or the plain text message body, resp.).

      \note The function allocates memory for the \c *ciphertext, so
            make sure you set free that memory when no longer needing
            it (as shown in example code provided with documentation
            of the class \c StructuringInfoWrapper).

      \note The function also does not use a pure StructuringInfo* struct
        parameter (as defined in cryptplug.h) but a convenient
        StructuringInfoWrapper& class parameter.
        Therefore you <b>must not</b> call the \c free_StructuringInfo()
        function: all memory that will be occupied for structuring info
        by the signing function will be freed automatically by the
        StructuringInfoWrapper's destructor.

      \see StructuringInfoWrapper
    */
    bool signMessage( const char* cleartext,
                      char** ciphertext,
                      const size_t* cipherLen,
                      const char* certificate,
                      StructuringInfoWrapper& structuring,
                      int* errId,
                      char** errTxt );


    /*! \ingroup groupSignAct
     */
    struct SignatureMetaDataExtendedInfo
    {
        struct tm* creation_time;
        const char* status_text;
        const char* fingerprint;
    };

    /*! \ingroup groupSignAct
     */
    struct SignatureMetaData {
        char* status;
        struct SignatureMetaDataExtendedInfo* extended_info;
        int extended_info_count;
        char* nota_xml;
        int status_code;
    };

    /* \ingroup groupSignAct
     * Frees the members of a signature meta data struct, but not the
     * signature meta data struct itself as this could be allocated on
     * the stack.
     */
    void freeSignatureMetaData( struct SignatureMetaData* );


    /*! \ingroup groupSignAct
    \brief Checks whether the signature of a message is
            valid. \c ciphertext specifies the message
            as it was received by the MUA, \c signaturetext the
            signature itself.

    Depending on the configuration, MUAs might not need to use this.
    If \c sigmeta is non-null, the
            \c SignatureMetaData object pointed to will
            contain meta information about the signature after the
            function call.
    */
    bool checkMessageSignature( const char* ciphertext,
                                const char* signaturetext,
                                bool signatureIsBinary,
                                int signatureLen,
                                struct SignatureMetaData* sigmeta );

    /*! \ingroup groupSignAct
    \brief Stores the certificates that follow with the message
            \c ciphertext locally.
    */
    bool storeCertificatesFromMessage( const char* ciphertext );


    /*! \ingroup groupCryptAct
      \brief Find all certificate for a given addressee.

      NOTE: The \c certificate parameter must point to an allready allocated
      block of memory which is large enough to hold the complete list.
    */
    bool findCertificates( const char* addressee, char** certificates );


    /*! \ingroup groupCryptAct
      \brief Encrypts an email message in
              \c cleartext according to the \c addressee and
              the current settings (algorithm, etc.) and
              returns the encoded data bloc in \c *ciphertext.
              The length returned in \c *cipherLen tells you the
              size (==amount of bytes) of the ciphertext, if the
              structuring information would return with
              contentTEncCode set to "base64" the ciphertext
              might contain a char 0x00 and has to be converted
              into base64 before sending.

      If the message could be encrypted, the function returns
              \c true, otherwise
              \c false.

      Use the StructuringInfoWrapper data returned in parameter
      \c structuring to find out how to build the respective
      MIME object (or the plain text message body, resp.).

      \note The function allocates memory for the \c *ciphertext, so
            make sure you set free that memory when no longer needing
            it (as shown in example code provided with documentation
            of the class \c StructuringInfoWrapper).

      \note The function also does not use a pure StructuringInfo* struct
        parameter (as defined in cryptplug.h) but a convenient
        StructuringInfoWrapper& class parameter.
        Therefore you <b>must not</b> call the \c free_StructuringInfo()
        function: all memory that will be occupied for structuring info
        by the signing function will be freed automatically by the
        StructuringInfoWrapper's destructor.

      \see StructuringInfoWrapper
    */
    bool encryptMessage( const char*  cleartext,
                         const char** ciphertext,
                         const size_t* cipherLen,
                         const char*  addressee,
                         StructuringInfoWrapper& structuring,
                         int* errId,
                         char** errTxt );

    /*! \ingroup groupCryptAct
      \brief Combines the functionality of
              \c encryptMessage() and
              \c signMessage().

      If \c certificate is \c NULL,
      the default certificate will be used.

      If the message could be signed and encrypted, the function returns
              \c true, otherwise
              \c false.

      Use the StructuringInfoWrapper data returned in parameter \c structuring
      to find out how to build the respective MIME object (or the plain
      text message body, resp.).

      \note The function allocates memory for the \c *ciphertext, so
            make sure you set free that memory when no longer needing
            it (as shown in example code provided with documentation
            of the class \c StructuringInfoWrapper).

      \note The function also does not use a pure StructuringInfo* struct
        parameter (as defined in cryptplug.h) but a convenient
        StructuringInfoWrapper& class parameter.
        Therefore you <b>must not</b> call the \c free_StructuringInfo()
        function: all memory that will be occupied for structuring info
        by the signing function will be freed automatically by the
        StructuringInfoWrapper's destructor.

      \see StructuringInfoWrapper
    */
    bool encryptAndSignMessage( const char* cleartext,
                                const char** ciphertext,
                                const char* certificate,
                                StructuringInfoWrapper& structuring );

    /*! \ingroup groupCryptAct
    \brief Tries to decrypt an email message
            \c ciphertext and returns the decrypted
            message in \c cleartext.

    The \c certificate is used for decryption. If
            the message could be decrypted, the function returns
            \c true, otherwise
            \c false.
    */
    bool decryptMessage( const char* ciphertext,
                         bool        cipherIsBinary,
                         int         cipherLen,
                         char** cleartext,
                         const char* certificate );

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
            in the first parameter. The memory returned in \a generatedKey
        must be freed with free() by the caller.
    */
    bool requestDecentralCertificate( const char* certparms, 
                                      char** generatedKey,
                                      int* keyLength );

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

    /*
      this function is for internal use
      by the StructuringInfoWrapper class
    */
    void* libPtr() const { return _libPtr; }

    /*
      this function is for internal use
      by the CryptPlugWrapper class and
      by the StructuringInfoWrapper class
    */
    bool wasDLError( const char* funcName );


private:
    QWidget*   _parent;
    QString    _name;
    QString    _libName;
    QString    _updateURL;
    void*      _libPtr;
    bool       _active;
    bool       _initialized;
    InitStatus _initStatus;
    QString    _lastError;

    void voidVoidFunc(                       const char* funcName );
    bool boolConstCharFunc( const char* txt, const char* funcName );
    bool boolConstCharConstCharFunc( const char* txt1, const char* txt2,
                                     const char* funcName );
    int intConstCharFunc( const char* txt, const char* funcName );
    void voidConstCharFunc( const char* txt, const char* funcName );
    const char* constCharVoidFunc(           const char* funcName );
    void voidBoolFunc(      bool flag,       const char* funcName );
    bool boolVoidFunc(                       const char* funcName );
    void voidIntFunc(       int value,       const char* funcName );
    int intVoidFunc(                        const char* funcName );

    void stationeryFunc( void** pixmap, const char** menutext, char* accel,
                         const char** tooltip, const char** statusbartext,
                         const char* funcName );
};

#endif // cryptplugwrapper_h
