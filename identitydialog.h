/*  -*- c++ -*-
    identitydialog.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 the KMail authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef __KMAIL_IDENTITYDIALOG_H__
#define __KMAIL_IDENTITYDIALOG_H__

#include <kdialogbase.h>

class KMIdentity;
class QLineEdit;
class KMFolderComboBox;
class QCheckBox;
class QComboBox;
class QString;
class QStringList;
namespace Kpgp {
  class SecretKeyRequester;
};
namespace KMail {
  class SignatureConfigurator;
};

namespace KMail {

  class IdentityDialog : public KDialogBase {
    Q_OBJECT
  public:
    IdentityDialog( QWidget * parent=0, const char * name = 0 );

    void setIdentity( /*_not_ const*/ KMIdentity & ident );

    void updateIdentity( KMIdentity & ident );

  public slots:
    void slotUpdateTransportCombo( const QStringList & sl );

  protected:
    bool checkFolderExists( const QString & folder, const QString & msg );

  protected:
    // "general" tab:
    QLineEdit                    *mNameEdit;
    QLineEdit                    *mOrganizationEdit;
    QLineEdit                    *mEmailEdit;
    // "advanced" tab:
    QLineEdit                    *mReplyToEdit;
    QLineEdit                    *mBccEdit;
    Kpgp::SecretKeyRequester     *mPgpKeyRequester;
    KMFolderComboBox             *mFccCombo;
    KMFolderComboBox             *mDraftsCombo;
    QCheckBox                    *mTransportCheck;
    QComboBox                    *mTransportCombo; // should be a KMTransportCombo...
    // "signature" tab:
    KMail::SignatureConfigurator *mSignatureConfigurator;
  };

}; // namespace KMail

#endif // __KMAIL_IDENTITYDIALOG_H__
