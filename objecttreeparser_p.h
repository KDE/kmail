/*  -*- mode: C++; c-file-style: "gnu" -*-
    objecttreeparser_p.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2009 Klarälvdalens Datakonsult AB

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef _KMAIL_OBJECTTREEPARSER_P_H_
#define _KMAIL_OBJECTTREEPARSER_P_H_

#include <gpgme++/verificationresult.h>
#include <gpgme++/decryptionresult.h>
#include <gpgme++/key.h>

#include <QObject>
#include <QString>
#include <QPointer>

#include "isubject.h"
#include "interfaces/bodypart.h"

namespace Kleo {
  class DecryptVerifyJob;
  class VerifyDetachedJob;
  class VerifyOpaqueJob;
  class KeyListJob;
}

class QStringList;

namespace KMail {

  class DecryptVerifyBodyPartMemento
    : public QObject,
      public KMail::Interface::BodyPartMemento,
      public KMail::ISubject
  {
    Q_OBJECT
  public:
    DecryptVerifyBodyPartMemento( Kleo::DecryptVerifyJob * job, const QByteArray & cipherText );
    ~DecryptVerifyBodyPartMemento();

    /* reimp */ Interface::Observer   * asObserver()   { return 0;    }
    /* reimp */ Interface::Observable * asObservable() { return this; }

    bool start();
    void exec();

    bool isRunning() const { return m_running; }

    const QByteArray & plainText() const { return m_plainText; }
    const GpgME::DecryptionResult & decryptResult() const { return m_dr; }
    const GpgME::VerificationResult & verifyResult() const { return m_vr; }
    const QString & auditLogAsHtml() const { return m_auditLog; }
    GpgME::Error auditLogError() const { return m_auditLogError; }

  private slots:
    void slotResult( const GpgME::DecryptionResult & dr,
                     const GpgME::VerificationResult & vr,
                     const QByteArray & plainText );
    void notify() {
      ISubject::notify();
    }

  private:
    void saveResult( const GpgME::DecryptionResult &,
                     const GpgME::VerificationResult &,
                     const QByteArray & );
  private:
    // input:
    const QByteArray m_cipherText;
    QPointer<Kleo::DecryptVerifyJob> m_job;
    bool m_running;
    // output:
    GpgME::DecryptionResult m_dr;
    GpgME::VerificationResult m_vr;
    QByteArray m_plainText;
    QString m_auditLog;
    GpgME::Error m_auditLogError;
  };


  class VerifyDetachedBodyPartMemento
    : public QObject,
      public KMail::Interface::BodyPartMemento,
      public KMail::ISubject
  {
    Q_OBJECT
  public:
    VerifyDetachedBodyPartMemento( Kleo::VerifyDetachedJob * job,
                                   Kleo::KeyListJob * klj,
                                   const QByteArray & signature,
                                   const QByteArray & plainText );
    ~VerifyDetachedBodyPartMemento();

    /* reimp */ Interface::Observer   * asObserver()   { return 0;    }
    /* reimp */ Interface::Observable * asObservable() { return this; }

    bool start();
    void exec();

    bool isRunning() const { return m_running; }

    const GpgME::VerificationResult & verifyResult() const { return m_vr; }
    const QString & auditLogAsHtml() const { return m_auditLog; }
    GpgME::Error auditLogError() const { return m_auditLogError; }
    const GpgME::Key & signingKey() const { return m_key; }

  private slots:
    void slotResult( const GpgME::VerificationResult & vr );
    void slotKeyListJobDone();
    void slotNextKey( const GpgME::Key & );
    void notify() {
      ISubject::notify();
    }

  private:
    void saveResult( const GpgME::VerificationResult & );
    bool canStartKeyListJob() const;
    QStringList keyListPattern() const;
    bool startKeyListJob();
  private:
    // input:
    const QByteArray m_signature;
    const QByteArray m_plainText;
    QPointer<Kleo::VerifyDetachedJob> m_job;
    QPointer<Kleo::KeyListJob> m_keylistjob;
    bool m_running;
    // output:
    GpgME::VerificationResult m_vr;
    QString m_auditLog;
    GpgME::Error m_auditLogError;
    GpgME::Key m_key;
  };


  class VerifyOpaqueBodyPartMemento
    : public QObject,
      public KMail::Interface::BodyPartMemento,
      public KMail::ISubject
  {
    Q_OBJECT
  public:
    VerifyOpaqueBodyPartMemento( Kleo::VerifyOpaqueJob * job,
                                 Kleo::KeyListJob * klj,
                                 const QByteArray & signature );
    ~VerifyOpaqueBodyPartMemento();

    /* reimp */ Interface::Observer   * asObserver()   { return 0;    }
    /* reimp */ Interface::Observable * asObservable() { return this; }

    bool start();
    void exec();

    bool isRunning() const { return m_running; }

    const QByteArray & plainText() const { return m_plainText; }
    const GpgME::VerificationResult & verifyResult() const { return m_vr; }
    const QString & auditLogAsHtml() const { return m_auditLog; }
    GpgME::Error auditLogError() const { return m_auditLogError; }
    const GpgME::Key & signingKey() const { return m_key; }

  private slots:
    void slotResult( const GpgME::VerificationResult & vr,
                     const QByteArray & plainText );
    void slotKeyListJobDone();
    void slotNextKey( const GpgME::Key & );
    void notify() {
      ISubject::notify();
    }

  private:
    void saveResult( const GpgME::VerificationResult &,
                     const QByteArray & );
    bool canStartKeyListJob() const;
    QStringList keyListPattern() const;
    bool startKeyListJob();
  private:
    // input:
    const QByteArray m_signature;
    QPointer<Kleo::VerifyOpaqueJob> m_job;
    QPointer<Kleo::KeyListJob> m_keylistjob;
    bool m_running;
    // output:
    GpgME::VerificationResult m_vr;
    QByteArray m_plainText;
    QString m_auditLog;
    GpgME::Error m_auditLogError;
    GpgME::Key m_key;
  };


} // namespace KMail

#endif // _KMAIL_OBJECTTREEPARSER_H_
