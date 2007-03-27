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

#include "config.h"

#include "distributionlistdialog.h"

#include <kpimutils/email.h>
#include <kabc/resource.h>
#include <kabc/stdaddressbook.h>
#include <kabc/distributionlist.h>

#include <k3listview.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kinputdialog.h>

#include <QLayout>
#include <QLabel>
#include <QLineEdit>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QBoxLayout>
#include "distributionlistdialog.moc"

class DistributionListItem : public Q3CheckListItem
{
  public:
    DistributionListItem( Q3ListView *list )
      : Q3CheckListItem( list, QString(), CheckBox )
    {
    }

    void setAddressee( const KABC::Addressee &a, const QString &email )
    {
      mIsTransient = false;
      init( a, email );
    }

    void setTransientAddressee( const KABC::Addressee &a, const QString &email )
    {
      mIsTransient = true;
      init( a, email );
    }

    void init( const KABC::Addressee &a, const QString &email )
    {
      mAddressee = a;
      mEmail = email;
      setText( 1, mAddressee.realName() );
      setText( 2, mEmail );
    }

    KABC::Addressee addressee() const
    {
      return mAddressee;
    }

    QString email() const
    {
      return mEmail;
    }

    bool isTransient() const
    {
      return mIsTransient;
    }

  private:
    KABC::Addressee mAddressee;
    QString mEmail;
    bool mIsTransient;
};


DistributionListDialog::DistributionListDialog( QWidget *parent )
  : KDialog( parent )
{
  QFrame *topFrame = new QFrame( this );
  setMainWidget( topFrame );
  setCaption( i18n("Save Distribution List") );
  setButtons( User1 | Cancel );
  setDefaultButton( User1 );
  setModal( false );
  setButtonText( User1, i18n("Save List") );

  QBoxLayout *topLayout = new QVBoxLayout( topFrame );
  topLayout->setSpacing( spacingHint() );

  QBoxLayout *titleLayout = new QHBoxLayout();
  topLayout->addItem( titleLayout );

  QLabel *label = new QLabel( i18n("Name:"), topFrame );
  titleLayout->addWidget( label );

  mTitleEdit = new QLineEdit( topFrame );
  titleLayout->addWidget( mTitleEdit );
  mTitleEdit->setFocus();

  mRecipientsList = new K3ListView( topFrame );
  mRecipientsList->addColumn( QString() );
  mRecipientsList->addColumn( i18n("Name") );
  mRecipientsList->addColumn( i18n("Email") );
  topLayout->addWidget( mRecipientsList );
  connect(this,SIGNAL(user1Clicked()),this,SLOT(slotUser1()));
}

void DistributionListDialog::setRecipients( const Recipient::List &recipients )
{
  Recipient::List::ConstIterator it;
  for( it = recipients.begin(); it != recipients.end(); ++it ) {
    QStringList emails = KPIMUtils::splitAddressList( (*it).email() );
    QStringList::ConstIterator it2;
    for( it2 = emails.begin(); it2 != emails.end(); ++it2 ) {
      QString name;
      QString email;
      KABC::Addressee::parseEmailAddress( *it2, name, email );
      if ( !email.isEmpty() ) {
        DistributionListItem *item = new DistributionListItem( mRecipientsList );
        KABC::Addressee::List addressees =
          KABC::StdAddressBook::self( true )->findByEmail( email );
        if ( addressees.isEmpty() ) {
          KABC::Addressee a;
          a.setNameFromString( name );
          a.insertEmail( email );
          item->setTransientAddressee( a, email );
          item->setOn( true );
        } else {
          KABC::Addressee::List::ConstIterator it3;
          for( it3 = addressees.begin(); it3 != addressees.end(); ++it3 ) {
            item->setAddressee( *it3, email );
            if ( it3 == addressees.begin() ) item->setOn( true );
          }
        }
      }
    }
  }
}

void DistributionListDialog::slotUser1()
{
  bool isEmpty = true;

  KABC::AddressBook *ab = KABC::StdAddressBook::self( true );

  Q3ListViewItem *i = mRecipientsList->firstChild();
  while( i ) {
    DistributionListItem *item = static_cast<DistributionListItem *>( i );
    if ( item->isOn() ) {
      isEmpty = false;
      break;
    }
    i = i->nextSibling();
  }

  if ( isEmpty ) {
    KMessageBox::information( this,
                              i18n("There are no recipients in your list. "
                                   "First select some recipients, "
                                   "then try again.") );
    return;
  }

  KABC::DistributionListManager manager( ab );
  manager.load();

  QString name = mTitleEdit->text();

  if ( name.isEmpty() ) {
    bool ok = false;
    name = KInputDialog::getText( i18n("New Distribution List"),
      i18n("Please enter name:"), QString(), &ok, this );
    if ( !ok || name.isEmpty() )
      return;
  }

  if ( manager.list( name ) ) {
    KMessageBox::information( this,
      i18n( "<qt>Distribution list with the given name <b>%1</b> "
        "already exists. Please select a different name.</qt>", name ) );
    return;
  }

  KABC::DistributionList *dlist = new KABC::DistributionList( &manager, name );
  i = mRecipientsList->firstChild();
  while( i ) {
    DistributionListItem *item = static_cast<DistributionListItem *>( i );
    if ( item->isOn() ) {
      kDebug() << "  " << item->addressee().fullEmail() << endl;
      if ( item->isTransient() ) {
        ab->insertAddressee( item->addressee() );
      }
      if ( item->email() == item->addressee().preferredEmail() ) {
        dlist->insertEntry( item->addressee() );
      } else {
        dlist->insertEntry( item->addressee(), item->email() );
      }
    }
    i = i->nextSibling();
  }

  // FIXME: Ask the user which resource to save to instead of the default
  bool saveError = true;
  KABC::Ticket *ticket = ab->requestSaveTicket( 0 /*default resource */ );
  if ( ticket )
    if ( ab->save( ticket ) )
      saveError = false;
    else
      ab->releaseSaveTicket( ticket );

  if ( saveError )
    kWarning(5006) << k_funcinfo << " Couldn't save new addresses in the distribution list just created to the address book" << endl;

  manager.save();

  close();
}
