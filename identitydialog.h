/*  -*- mode: C++; c-file-style: "gnu" -*-
    identitydialog.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

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

#ifndef __KMAIL_IDENTITYDIALOG_H__
#define __KMAIL_IDENTITYDIALOG_H__

#include <kdialog.h>

class QCheckBox;
class QString;
class TemplatesConfiguration;

class KComboBox;
class KLineEdit;
class KPushButton;

namespace Kleo {
  class EncryptionKeyRequester;
  class SigningKeyRequester;
}
namespace KPIMIdentities {
  class Identity;
  class SignatureConfigurator;
}
namespace KMail {
  class XFaceConfigurator;
  class FolderRequester;
}
namespace KPIM {
  class DictionaryComboBox;
}

namespace MailTransport {
  class TransportComboBox;
}

namespace KMail {

  class IdentityDialog : public KDialog {
    Q_OBJECT
  public:
    IdentityDialog( QWidget * parent=0 );
    virtual ~IdentityDialog();

    void setIdentity( /*_not_ const*/ KPIMIdentities::Identity & ident );

    void updateIdentity( KPIMIdentities::Identity & ident );

  protected slots:
    void slotAboutToShow( QWidget * w );

    /*! \reimp */
    virtual void slotButtonClicked( int button );

    // copy default templates to identity templates
    void slotCopyGlobal();

  private:
    bool checkFolderExists( const QString & folder, const QString & msg );
    bool validateAddresses( const QString & addresses );

  protected:
    // "general" tab:
    KLineEdit                    *mNameEdit;
    KLineEdit                    *mOrganizationEdit;
    KLineEdit                    *mEmailEdit;
    // "cryptography" tab:
    QWidget                      *mCryptographyTab;
    Kleo::SigningKeyRequester    *mPGPSigningKeyRequester;
    Kleo::EncryptionKeyRequester *mPGPEncryptionKeyRequester;
    Kleo::SigningKeyRequester    *mSMIMESigningKeyRequester;
    Kleo::EncryptionKeyRequester *mSMIMEEncryptionKeyRequester;
    KComboBox                    *mPreferredCryptoMessageFormat;
    // "advanced" tab:
    KLineEdit                        *mReplyToEdit;
    KLineEdit                        *mBccEdit;
    KPIM::DictionaryComboBox         *mDictionaryCombo;
    FolderRequester                  *mFccCombo;
    FolderRequester                  *mDraftsCombo;
    FolderRequester                  *mTemplatesCombo;
    QCheckBox                        *mTransportCheck;
    MailTransport::TransportComboBox *mTransportCombo;
    // "templates" tab:
    TemplatesConfiguration       *mWidget;
    QCheckBox                    *mCustom;
    KPushButton                  *mCopyGlobal;
    // "signature" tab:
    KPIMIdentities::SignatureConfigurator *mSignatureConfigurator;
    // "X-Face" tab:
    KMail::XFaceConfigurator *mXFaceConfigurator;
  };

} // namespace KMail

#endif // __KMAIL_IDENTITYDIALOG_H__
