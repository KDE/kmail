/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef KMAIl_KLEOJOBEXECUTOR_H
#define KMAIl_KLEOJOBEXECUTOR_H

#include <gpgme++/decryptionresult.h>
#include <gpgme++/importresult.h>
#include <gpgme++/verificationresult.h>

#include <QtCore/QObject>

#include <utility>

class QEventLoop;

namespace Kleo {
  class DecryptVerifyJob;
  class ImportJob;
  class VerifyDetachedJob;
  class VerifyOpaqueJob;
}

namespace KMail {

/**
  Helper class for synchronous execution of Kleo crypto jobs.
*/
class KleoJobExecutor : public QObject
{
  Q_OBJECT
  public:
    KleoJobExecutor( QObject *parent = 0 );

    GpgME::VerificationResult exec( Kleo::VerifyDetachedJob* job,
                                    const QByteArray &signature,
                                    const QByteArray &signedData );
    GpgME::VerificationResult exec( Kleo::VerifyOpaqueJob* job,
                                    const QByteArray &signedData,
                                    QByteArray &plainText );
    std::pair<GpgME::DecryptionResult,GpgME::VerificationResult> exec( Kleo::DecryptVerifyJob *job,
                                                                       const QByteArray &cipherText,
                                                                       QByteArray &plainText );
    GpgME::ImportResult exec( Kleo::ImportJob* job, const QByteArray &certData );

    QString auditLogAsHtml() const;

  private slots:
    void verificationResult( const GpgME::VerificationResult &result );
    void verificationResult( const GpgME::VerificationResult &result, const QByteArray &plainText );
    void decryptResult( const GpgME::DecryptionResult & decryptionresult,
                        const GpgME::VerificationResult & verificationresult,
                        const QByteArray & plainText );
    void importResult( const GpgME::ImportResult &result );

  private:
    QEventLoop *mEventLoop;
    GpgME::VerificationResult mVerificationResult;
    GpgME::DecryptionResult mDecryptResult;
    GpgME::ImportResult mImportResult;
    QByteArray mData;
    QString mAuditLog;
};

}

#endif
