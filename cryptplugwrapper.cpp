/**
 * cryptplugwrapper.cpp
 *
 * Copyright (c) 2001 Karl-Heinz Zimmer, Klaraelvdalens Datakonsult AB
 *
 * This CRYPTPLUG wrapper implementation is based on cryptplug.h by
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




/*
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                    *
 *  This file's source comments - as well as those in interface file  *
 *  cryptplugwrapper.h - are optimized for processing by Doxygen.     *
 *                                                                    *
 *  To obtain best results please get an updated version of Doxygen,  *
 *  for sources and binaries goto http://www.doxygen.org/index.html   *
 *                                                                    *
  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                                                                      *
                                                                      */



/*! \file cryptplugwrapper.cpp
    \brief C++ wrapper for the CRYPTPLUG library API.

    This CRYPTPLUG wrapper implementation is based on cryptplug.h by
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

    \see cryptplugwrapper.h
*/


#include <dlfcn.h>

#include <stdlib.h>
#include <stdio.h>

#include "cryptplugwrapper.h"
#include "cryptplugwrapper.moc"




/* some multi purpose functions ******************************************/


void CryptPlugWrapper::voidVoidFunc( const char* funcName )
{
    if ( _active && _initialized ) {
        void (*p_func)() = (void (*)()) dlsym(_libPtr, funcName);
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function '%s': %s\n" << funcName << thisError << endl;
        } else
            (*p_func)();
    }
}


bool CryptPlugWrapper::boolConstCharFunc( const char* txt, const char* funcName )
{
    if ( _active && _initialized ) {
        bool (*p_func)( const char* )
            = (bool (*)(const char* ))
                dlsym(_libPtr, funcName);
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function '%s': %s\n" << funcName << thisError << endl;
        } else
            return (*p_func)( txt );
    }
}


void CryptPlugWrapper::voidConstCharFunc( const char* txt, const char* funcName )
{
    if ( _active && _initialized ) {
        void (*p_func)( const char* )
            = (void (*)(const char* ))
                dlsym(_libPtr, funcName);
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function '%s': %s\n" << funcName << thisError << endl;
        } else
            (*p_func)( txt );
    }
}


const char* CryptPlugWrapper::constCharVoidFunc( const char* funcName )
{
    if ( _active && _initialized ) {
        const char* (*p_func)(void) = (const char* (*)(void))
                                      dlsym(_libPtr, funcName);
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function '%s': %s\n" << funcName << thisError << endl;
        } else
            return (*p_func)();
    }
    return 0;
}


void CryptPlugWrapper::voidBoolFunc( bool flag, const char* funcName )
{
    if ( _active && _initialized ) {
        void (*p_func)( bool )
            = (void (*)(bool ))
                dlsym(_libPtr, funcName);
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function '%s': %s\n" << funcName << thisError << endl;
        } else
            (*p_func)( flag );
    }
}


bool CryptPlugWrapper::boolVoidFunc( const char* funcName )
{
    if ( _active && _initialized ) {
        bool (*p_func)( void )
            = (bool (*)(void ))
                dlsym(_libPtr, funcName);
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function '%s': %s\n" << funcName << thisError << endl;
        } else
            return (*p_func)();
    }
}


void CryptPlugWrapper::voidIntFunc( int value, const char* funcName )
{
    if ( _active && _initialized ) {
        void (*p_func)( int )
            = (void (*)(int ))
                dlsym(_libPtr, funcName);
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function '%s': %s\n" << funcName << thisError << endl;
        } else
            (*p_func)( value );
    }
}


int CryptPlugWrapper::intVoidFunc( const char* funcName )
{
    int res = 0;
    if ( _active && _initialized ) {
        int (*p_func)( void )
            = (int (*)(void ))
                dlsym(_libPtr, funcName);
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function '%s': %s\n" << funcName << thisError << endl;
        } else
            res = (*p_func)();
    }
    return res;
}


void CryptPlugWrapper::stationeryFunc( void** pixmap,
                                       const char** menutext,
                                       char* accel,
                                       const char** tooltip,
                                       const char** statusbartext,
                                       const char* funcName )
{
    if ( _active && _initialized ) {
        void  (*p_func)( void**, const char**, char*, const char**, const char** )
            = (void (*)( void**, const char**, char*, const char**, const char** ))
                dlsym(_libPtr, funcName);
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function '%s': %s\n" << funcName << thisError << endl;
        } else
            (*p_func)( pixmap, menutext, accel, tooltip, statusbartext );
    }
}





/* some special functions ************************************************/


CryptPlugWrapper::CryptPlugWrapper( QWidget*       parent,
                                    const QString& libName,
                                    bool           active = false ):
    _parent(  parent  ),
    _libName( libName ),
    _libPtr(  0 ),
    _active(  active  ),
    _initialized( false ),
    _initStatus( InitStatus_undef )
{
    // this space left empty intentionally
}


CryptPlugWrapper::~CryptPlugWrapper()
{
    deinitialize();
}


void CryptPlugWrapper::setActive( bool active )
{
    _active = active;
}


bool CryptPlugWrapper::active() const
{
    return _active;
}


bool CryptPlugWrapper::setLibName( const QString& libName )
{
    bool bOk = ! _initialized;  // Changing the lib name is only allowed
    if( bOk )                   // when either no initialization took
        _libName = libName;     // place or 'deinitialize()' has been
    return bOk;                 // called afterwards.
}

QString CryptPlugWrapper::libName() const
{
    return _libName;
}


bool CryptPlugWrapper::initialize( InitStatus* initStatus, QString* errorMsg )
{
    bool bOk = false;
    if ( _active && ! _initialized )
    {
        _initStatus = InitStatus_undef;
        /* make sure we have a lib name */
        if ( _libName.isEmpty() ) {
            _initStatus = InitStatus_NoLibName;
kdDebug() << "Couldn't open plug-in library: No library name was given.\n" << endl;
        }
        else
        {
            QString thisError;

            /* try to load the library */
            _libPtr   = dlopen(_libName.latin1(), RTLD_LAZY);
            thisError = dlerror();
            if ( !_libPtr ) {
                _initStatus = InitStatus_LoadError;
kdDebug() << "Couldn't open '%s': %s\n" << _libName.latin1() << thisError << endl;
            } else {
                /* get the symbol */
                bool (*p_initialize)(void) = (bool (*)(void))
                                             dlsym(_libPtr, "initialize");
                thisError = dlerror();
                if ( ! thisError.isEmpty() ) {
                    _initStatus = InitStatus_LoadError;
kdDebug() << "Couldn't find function 'initialize': %s\n" << thisError << endl;
                } else {
                    /* now call the function in the library */
                    if( ! (*p_initialize)() )
                    {
                        _initStatus = InitStatus_InitError;
kdDebug() << "Error while executing function 'initialize'.\n" << endl;
                    } else {
                        bOk = true;
                        _initStatus  = InitStatus_Ok;
                    }
                }
            }
            if( errorMsg )
                *errorMsg = thisError;
            if( ! thisError.isEmpty() )
                _lastError = thisError;
        }
        _initialized = _initStatus == InitStatus_Ok;
    }
    if( initStatus )
        *initStatus = _initStatus;
    return bOk;
}



void CryptPlugWrapper::deinitialize()
{
    if ( _active && _initialized ) {
        bool (*p_deinitialize)(void) = (bool (*)(void))
                                       dlsym(_libPtr, "deinitialize");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'deinitialize': %s\n" << thisError << endl;
        } else
            (*p_deinitialize)();
        _initialized = false;
    }
}


CryptPlugWrapper::InitStatus CryptPlugWrapper::initStatus( QString* errorMsg ) const
{
    if( errorMsg )
        *errorMsg = _lastError;
    return _initStatus;
}


bool CryptPlugWrapper::hasFeature( Feature flag )
{
    bool bHasIt = false;
    if ( _active && _initialized ) {
        bool (*p_func)(Feature) = (bool (*)(Feature))
                                  dlsym(_libPtr, "hasFeature");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'hasFeature': %s\n" << thisError << endl;
        } else
            bHasIt = (*p_func)( flag );
    }
    return bHasIt;
}


void CryptPlugWrapper::unsafeStationery( void** pixmap,
                                         const char** menutext,
                                         char* accel,
                                         const char** tooltip,
                                         const char** statusbartext )
{
    stationeryFunc( pixmap, menutext, accel, tooltip, statusbartext,
                    "unsafeStationery" );
}


void CryptPlugWrapper::signedStationery( void** pixmap,
                                         const char** menutext,
                                         char* accel,
                                         const char** tooltip,
                                         const char** statusbartext )
{
    stationeryFunc( pixmap, menutext, accel, tooltip, statusbartext,
                    "signedStationery" );
}


void CryptPlugWrapper::encryptedStationery( void** pixmap,
                                            const char** menutext,
                                            char* accel,
                                            const char** tooltip,
                                            const char** statusbartext )
{
    stationeryFunc( pixmap, menutext, accel, tooltip, statusbartext,
                    "encryptedStationery" );
}


void CryptPlugWrapper::signedEncryptedStationery(void** pixmap,
                                                 const char** menutext,
                                                 char* accel,
                                                 const char** tooltip,
                                                 const char** statusbartext)
{
    stationeryFunc( pixmap, menutext, accel, tooltip, statusbartext,
                    "signedEncryptedStationery" );
}


const char* CryptPlugWrapper::signatureConfigurationDialog()
{
    return constCharVoidFunc( "signatureConfigurationDialog" );
}


const char* CryptPlugWrapper::signatureKeySelectionDialog()
{
    return constCharVoidFunc( "signatureKeySelectionDialog" );
}


const char* CryptPlugWrapper::signatureAlgorithmDialog()
{
    return constCharVoidFunc( "signatureAlgorithmDialog" );
}


const char* CryptPlugWrapper::signatureHandlingDialog()
{
    return constCharVoidFunc( "signatureHandlingDialog" );
}


void CryptPlugWrapper::setSignatureKeyCertificate( const char* certificate )
{
    voidConstCharFunc( certificate, "setSignatureKeyCertificate" );
}


const char* CryptPlugWrapper::signatureKeyCertificate()
{
    return constCharVoidFunc( "signatureKeyCertificate" );
}


void CryptPlugWrapper::setSignatureAlgorithm( SignatureAlgorithm sigAlg )
{
    voidIntFunc( (int)sigAlg, "setSignatureAlgorithm" );
}


SignatureAlgorithm CryptPlugWrapper::signatureAlgorithm()
{
    return (SignatureAlgorithm) intVoidFunc( "signatureAlgorithm" );
}


void CryptPlugWrapper::setSendCertificates( SendCertificates sendCert )
{
    voidIntFunc( (int)sendCert, "setSendCertificates" );
}


SendCertificates CryptPlugWrapper::sendCertificates()
{
    return (SendCertificates) intVoidFunc( "sendCertificates" );
}


void CryptPlugWrapper::setSignEmail( SignEmail signMail )
{
    voidIntFunc( (int)signMail, "setSignEmail" );
}


SignEmail CryptPlugWrapper::signEmail()
{
    return (SignEmail) intVoidFunc( "signEmail" );
}


void CryptPlugWrapper::setSaveSentSignatures( bool flag )
{
    voidBoolFunc( flag, "setSaveSentSignatures" );
}


bool CryptPlugWrapper::saveSentSignatures()
{
    return boolVoidFunc( "saveSentSignatures" );
}


void CryptPlugWrapper::setCertificateExpiryNearWarning( bool flag )
{
    voidBoolFunc( flag, "setCertificateExpiryNearWarning" );
}


bool CryptPlugWrapper::certificateExpiryNearWarning()
{
    return boolVoidFunc( "certificateExpiryNearWarning" );
}


void CryptPlugWrapper::setWarnNoCertificate( bool flag )
{
    voidBoolFunc( flag, "setWarnNoCertificate" );
}


bool CryptPlugWrapper::warnNoCertificate()
{
    return boolVoidFunc( "warnNoCertificate" );
}


void CryptPlugWrapper::setNumPINRequests( PinRequests reqMode )
{
    voidIntFunc( (int)reqMode, "setNumPINRequests" );
}


PinRequests CryptPlugWrapper::numPINRequests()
{
    return (PinRequests) intVoidFunc( "numPINRequests" );
}


void CryptPlugWrapper::setCheckSignatureCertificatePathToRoot( bool flag )
{
    voidBoolFunc( flag, "setCheckSignatureCertificatePathToRoot" );
}


bool CryptPlugWrapper::checkSignatureCertificatePathToRoot()
{
    return boolVoidFunc( "checkSignatureCertificatePathToRoot" );
}


void CryptPlugWrapper::setSignatureUseCRLs( bool flag )
{
    voidBoolFunc( flag, "setSignatureUseCRLs" );
}



bool CryptPlugWrapper::signatureUseCRLs()
{
    return boolVoidFunc( "signatureUseCRLs" );
}


void CryptPlugWrapper::setSignatureCRLExpiryNearWarning( bool flag )
{
    voidBoolFunc( flag, "setSignatureCRLExpiryNearWarning" );
}


bool CryptPlugWrapper::signatureCRLExpiryNearWarning()
{
    return boolVoidFunc( "signatureCRLExpiryNearWarning" );
}


void CryptPlugWrapper::setSignatureCRLNearExpiryInterval( int interval )
{
    voidIntFunc( interval, "setSignatureCRLNearExpiryInterval" );
}


int CryptPlugWrapper::signatureCRLNearExpiryInterval()
{
    return intVoidFunc( "signatureCRLNearExpiryInterval" );
}


const char* CryptPlugWrapper::encryptionConfigurationDialog()
{
    return constCharVoidFunc( "encryptionConfigurationDialog" );
}


const char* CryptPlugWrapper::encryptionAlgorithmDialog()
{
    return constCharVoidFunc( "encryptionAlgorithmDialog" );
}


const char* CryptPlugWrapper::encryptionHandlingDialog()
{
    return constCharVoidFunc( "encryptionHandlingDialog" );
}


const char* CryptPlugWrapper::encryptionReceiverDialog()
{
    return constCharVoidFunc( "encryptionReceiverDialog" );
}


void CryptPlugWrapper::setEncryptionAlgorithm( EncryptionAlgorithm cryptAlg )
{
    voidIntFunc( (int)cryptAlg, "setEncryptionAlgorithm" );
}



EncryptionAlgorithm CryptPlugWrapper::encryptionAlgorithm()
{
    return (EncryptionAlgorithm) intVoidFunc( "encryptionAlgorithm" );
}


void CryptPlugWrapper::setEncryptEmail( EncryptEmail cryptMode )
{
    voidIntFunc( (int)cryptMode, "setEncryptEmail" );
}



EncryptEmail CryptPlugWrapper::encryptEmail()
{
    return (EncryptEmail) intVoidFunc( "encryptEmail" );
}


void CryptPlugWrapper::setSaveMessagesEncrypted( bool flag )
{
    voidBoolFunc( flag, "setSaveMessagesEncrypted" );
}


bool CryptPlugWrapper::saveMessagesEncrypted()
{
    return boolVoidFunc( "saveMessagesEncrypted" );
}


void CryptPlugWrapper::setCheckEncryptionCertificatePathToRoot( bool flag )
{
    voidBoolFunc( flag, "setCheckEncryptionCertificatePathToRoot" );
}



bool CryptPlugWrapper::checkEncryptionCertificatePathToRoot()
{
    return boolVoidFunc( "checkEncryptionCertificatePathToRoot" );
}


void CryptPlugWrapper::setEncryptionUseCRLs( bool flag )
{
    voidBoolFunc( flag, "setEncryptionUseCRLs" );
}



bool CryptPlugWrapper::encryptionUseCRLs()
{
    return boolVoidFunc( "encryptionUseCRLs" );
}


void CryptPlugWrapper::setEncryptionCRLExpiryNearWarning( bool flag )
{
    voidBoolFunc( flag, "setEncryptionCRLExpiryNearWarning" );
}



bool CryptPlugWrapper::encryptionCRLExpiryNearWarning()
{
    return boolVoidFunc( "encryptionCRLExpiryNearWarning" );
}


void CryptPlugWrapper::setEncryptionCRLNearExpiryInterval( int interval )
{
    voidIntFunc( interval, "setEncryptionCRLNearExpiryInterval" );
}



int CryptPlugWrapper::encryptionCRLNearExpiryInterval()
{
    return intVoidFunc( "encryptionCRLNearExpiryInterval" );
}



const char* CryptPlugWrapper::directoryServiceConfigurationDialog()
{
    return constCharVoidFunc( "directoryServiceConfigurationDialog" );
}


void CryptPlugWrapper::appendDirectoryServer( const char* servername, int port )
{
    if ( _active && _initialized ) {
        void (*p_func)( const char*, int )
            = (void (*)(const char*, int ))
                dlsym(_libPtr, "appendDirectoryServer");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'appendDirectoryServer': %s\n" << thisError << endl;
        } else
            (*p_func)( servername, port );
    }
}



void CryptPlugWrapper::setDirectoryServers( struct DirectoryServer server )
{
    if ( _active && _initialized ) {
        void (*p_func)( struct DirectoryServer )
            = (void (*)(struct DirectoryServer ))
                dlsym(_libPtr, "setDirectoryServers");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'setDirectoryServers': %s\n" << thisError << endl;
        } else
            (*p_func)( server );
    }
}



CryptPlugWrapper::DirectoryServer * CryptPlugWrapper::directoryServers()
{
    CryptPlugWrapper::DirectoryServer * res = 0;
    if ( _active && _initialized ) {
        CryptPlugWrapper::DirectoryServer * (*p_func)( void )
            = (CryptPlugWrapper::DirectoryServer * (*)(void ))
                dlsym(_libPtr, "directoryServers");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'directoryServers': %s\n" << thisError << endl;
        } else
            res = (*p_func)();
    }
    return res;
}


void CryptPlugWrapper::setCertificateSource( CertificateSource source )
{
    voidIntFunc( (int)source, "setCertificateSource" );
}


CertificateSource CryptPlugWrapper::certificateSource()
{
    return (CertificateSource) intVoidFunc( "certificateSource" );
}


void CryptPlugWrapper::setCRLSource( CertificateSource source )
{
    voidIntFunc( (int)source, "setCRLSource" );
}


CertificateSource CryptPlugWrapper::crlSource()
{
    return (CertificateSource) intVoidFunc( "crlSource" );
}



bool CryptPlugWrapper::certificateValidity( const char* certificate,
                          int* level )
{
    bool res = false;
    if ( _active && _initialized ) {
        bool (*p_func)( const char*, int* )
            = (bool (*)(const char*, int* ))
                dlsym(_libPtr, "certificateValidity");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'certificateValidity': %s\n" << thisError << endl;
        } else
            res = (*p_func)( certificate, level );
    }
    return res;
}



bool CryptPlugWrapper::signMessage( const char* cleartext, const char**
          ciphertext, const char* certificate )
{
    bool res = false;
    if ( _active && _initialized ) {
        bool (*p_func)( const char*, const char**, const char* )
            = (bool (*)(const char*, const char**, const char* ))
                dlsym(_libPtr, "signMessage");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'signMessage': %s\n" << thisError << endl;
        } else
            res = (*p_func)( cleartext, ciphertext, certificate );
    }
    return res;
}


bool CryptPlugWrapper::checkMessageSignature( const char* ciphertext, const char**
        cleartext, struct SignatureMetaData* sigmeta )
{
    bool res = false;
    if ( _active && _initialized ) {
        bool (*p_func)( const char*, const char**, struct SignatureMetaData* )
            = (bool (*)(const char*, const char**, struct SignatureMetaData* ))
                dlsym(_libPtr, "checkMessageSignature");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'checkMessageSignature': %s\n" << thisError << endl;
        } else
            res = (*p_func)( ciphertext, cleartext, sigmeta );
    }
    return res;
}


bool CryptPlugWrapper::storeCertificatesFromMessage(
        const char* ciphertext )
{
    return boolConstCharFunc( ciphertext, "storeCertificatesFromMessage" );
}



bool CryptPlugWrapper::encryptMessage( const char* cleartext,
                     const char** ciphertext )
{
    bool res = false;
    if ( _active && _initialized ) {
        bool (*p_func)( const char*, const char** )
            = (bool (*)(const char*, const char** ))
                dlsym(_libPtr, "encryptMessage");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'encryptMessage': %s\n" << thisError << endl;
        } else
            res = (*p_func)( cleartext, ciphertext );
    }
    return res;
}


bool CryptPlugWrapper::encryptAndSignMessage( const char* cleartext,
          const char** ciphertext, const char* certificate,
          struct SignatureMetaData* sigmeta )
{
    bool res = false;
    if ( _active && _initialized ) {
        bool (*p_func)( const char*, const char**, const char*, struct SignatureMetaData* )
            = (bool (*)(const char*, const char**, const char*, struct SignatureMetaData* ))
                dlsym(_libPtr, "encryptAndSignMessage");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'encryptAndSignMessage': %s\n" << thisError << endl;
        } else
            res = (*p_func)( cleartext, ciphertext, certificate, sigmeta );
    }
    return res;
}


bool CryptPlugWrapper::decryptMessage( const char* ciphertext, const
          char** cleartext, const char* certificate )
{
    bool res = false;
    if ( _active && _initialized ) {
        bool (*p_func)( const char*, const char**, const char* )
            = (bool (*)(const char*, const char**, const char* ))
                dlsym(_libPtr, "decryptMessage");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'decryptMessage': %s\n" << thisError << endl;
        } else
            res = (*p_func)( ciphertext, cleartext, certificate );
    }
    return res;
}


bool CryptPlugWrapper::decryptAndCheckMessage( const char* ciphertext,
          const char** cleartext, const char* certificate,
          struct SignatureMetaData* sigmeta )
{
    bool res = false;
    if ( _active && _initialized ) {
        bool (*p_func)( const char*, const char**, const char*, struct SignatureMetaData* )
            = (bool (*)(const char*, const char**, const char*, struct SignatureMetaData* ))
                dlsym(_libPtr, "decryptAndCheckMessage");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'decryptAndCheckMessage': %s\n" << thisError << endl;
        } else
            res = (*p_func)( ciphertext, cleartext, certificate, sigmeta );
    }
    return res;
}



const char* CryptPlugWrapper::requestCertificateDialog()
{
    return constCharVoidFunc( "requestCertificateDialog" );
}


bool CryptPlugWrapper::requestDecentralCertificate( const char* name, const char*
          email, const char* organization, const char* department,
          const char* ca_address )
{
    bool res = false;
    if ( _active && _initialized ) {
        bool (*p_func)( const char*, const char*, const char*, const char*, const char*  )
            = (bool (*)(const char*, const char*, const char*, const char*, const char*  ))
                dlsym(_libPtr, "requestDecentralCertificate");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'requestDecentralCertificate': %s\n" << thisError << endl;
        } else
            res = (*p_func)( name, email, organization, department, ca_address );
    }
    return res;
}


bool CryptPlugWrapper::requestCentralCertificateAndPSE( const char* name,
          const char* email, const char* organization, const char* department,
          const char* ca_address )
{
    bool res = false;
    if ( _active && _initialized ) {
        bool (*p_func)( const char*, const char*, const char*, const char*, const char*  )
            = (bool (*)(const char*, const char*, const char*, const char*, const char*  ))
                dlsym(_libPtr, "requestCentralCertificateAndPSE");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'requestCentralCertificateAndPSE': %s\n" << thisError << endl;
        } else
            res = (*p_func)( name, email, organization, department, ca_address );
    }
    return res;
}


bool CryptPlugWrapper::createPSE()
{
    return boolVoidFunc( "createPSE" );
}


bool CryptPlugWrapper::registerCertificate( const char* certificate )
{
    boolConstCharFunc( certificate, "registerCertificate" );
}


bool CryptPlugWrapper::requestCertificateProlongation( const char* certificate,
                                     const char* ca_address )
{
    bool res = false;
    if ( _active && _initialized ) {
        bool (*p_func)( const char*, const char*  )
            = (bool (*)(const char*, const char*  ))
                dlsym(_libPtr, "requestCertificateProlongation");
        QString thisError = dlerror();
        if ( ! thisError.isEmpty() ) {
            _lastError = thisError;
kdDebug() << "Couldn't find function 'requestCertificateProlongation': %s\n" << thisError << endl;
        } else
            res = (*p_func)( certificate, ca_address );
    }
    return res;
}


const char* CryptPlugWrapper::certificateChain()
{
    return constCharVoidFunc( "certificateChain" );
}


bool CryptPlugWrapper::deleteCertificate( const char* certificate )
{
    boolConstCharFunc( certificate, "deleteCertificate" );
}


bool CryptPlugWrapper::archiveCertificate( const char* certificate )
{
    boolConstCharFunc( certificate, "archiveCertificate" );
}



const char* CryptPlugWrapper::displayCRL()
{
    return constCharVoidFunc( "displayCRL" );
}


void CryptPlugWrapper::updateCRL()
{
    voidVoidFunc( "updateCRL" );
}
