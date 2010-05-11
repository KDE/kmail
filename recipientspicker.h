/*
    This file is part of KMail.

    Copyright (c) 2005 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef RECIPIENTSPICKER_H
#define RECIPIENTSPICKER_H

#include <messagecomposer/recipient.h>

#include <kabc/addressee.h>
#include <kdialog.h>

namespace Akonadi {
class EmailAddressSelectionView;
}

namespace KLDAP {
class LdapSearchDialog;
}

class RecipientsPicker : public KDialog
{
  Q_OBJECT

  public:
    RecipientsPicker( QWidget *parent );
    ~RecipientsPicker();

    void setRecipients( const Recipient::List & );

    void setDefaultType( Recipient::Type );

  Q_SIGNALS:
    void pickedRecipient( const Recipient & );

  protected:
    void readConfig();
    void writeConfig();

    void pick( Recipient::Type );

    void keyPressEvent( QKeyEvent* );

  protected Q_SLOTS:
    void slotToClicked();
    void slotCcClicked();
    void slotBccClicked();
    void slotPicked();
    void slotSearchLDAP();
    void ldapSearchResult();
    void slotSelectionChanged();

  private:
    Akonadi::EmailAddressSelectionView *mView;

    QPushButton *mToButton;
    QPushButton *mCcButton;
    QPushButton *mBccButton;

    QPushButton *mSearchLDAPButton;
    KLDAP::LdapSearchDialog *mLdapSearchDialog;

    Recipient::Type mDefaultType;
};

#endif
