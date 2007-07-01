/*  -*- c++ -*-
    partmetadata.h

    KMail, the KDE mail client.
    Copyright (c) 2002-2003 Karl-Heinz Zimmer <khz@kde.org>
    Copyright (c) 2003      Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/


#ifndef _KMAIL_PARTMETADATA_H_
#define _KMAIL_PARTMETADATA_H_

#include <cryptplugwrapper.h>

#include <kpgp/kpgp.h>
#include <QString>
#include <time.h>

namespace KMail {

  class PartMetaData {
  public:
    PartMetaData()
      : isSigned( false ),
        isGoodSignature( false ),
        sigStatusFlags( CryptPlugWrapper::SigStatus_UNKNOWN ),
        isEncrypted( false ),
        isDecryptable( false ),
        technicalProblem( false ),
        isEncapsulatedRfc822Message( false )
    {
      creationTime.tm_year = 0;
      creationTime.tm_mon  = 1;
      creationTime.tm_mday = 1;
    }
    bool isSigned;
    bool isGoodSignature;
    CryptPlugWrapper::SigStatusFlags sigStatusFlags;
    QString signClass;
    QString signer;
    QStringList signerMailAddresses;
    QByteArray keyId;
    Kpgp::Validity keyTrust;
    QString status;  // to be used for unknown plug-ins
    int status_code; // to be used for i18n of OpenPGP and S/MIME CryptPlugs
    QString errorText;
    tm creationTime;
    bool isEncrypted;
    bool isDecryptable;
    QString decryptionError;
    bool technicalProblem;
    bool isEncapsulatedRfc822Message;
  };

} // namespace KMail

#endif // _KMAIL_PARTMETADATA_H_

