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


#include "distributionlistdialog.h"

#include <kpimutils/email.h>
#include <kabc/resource.h>
#include <kabc/stdaddressbook.h>
#include <kabc/distributionlist.h>

#include <KLocale>
#include <KDebug>
#include <KMessageBox>
#include <KInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "distributionlistdialog.moc"

class DistributionListItem : public QTreeWidgetItem
{
  public:
    DistributionListItem( QTreeWidget *tree )
      : QTreeWidgetItem( tree )
    {
      setFlags( flags() | Qt::ItemIsUserCheckable );
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
      setText( 0, mAddressee.realName() );
      setText( 1, mEmail );
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
  titleLayout->setSpacing( spacingHint() );
  topLayout->addItem( titleLayout );

  QLabel *label = new QLabel( i18n("&Name:"), topFrame );
  titleLayout->addWidget( label );

  mTitleEdit = new QLineEdit( topFrame );
  titleLayout->addWidget( mTitleEdit );
  mTitleEdit->setFocus();
  label->setBuddy( mTitleEdit );

  mRecipientsList = new QTreeWidget( topFrame );
  mRecipientsList->setHeaderLabels(
                     QStringList() << i18n( "Name" ) << i18n( "Email" )
                                  );
  mRecipientsList->setRootIsDecorated( false );
  topLayout->addWidget( mRecipientsList );
  connect( this, SIGNAL( user1Clicked() ),
           this, SLOT( slotUser1() ) );
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
          item->setCheckState( 0, Qt::Checked );
        } else {
          KABC::Addressee::List::ConstIterator it3;
          for( it3 = addressees.begin(); it3 != addressees.end(); ++it3 ) {
            item->setAddressee( *it3, email );
            if ( it3 == addressees.begin() ) item->setCheckState( 0, Qt::Checked );
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

  for (int i = 0; i < mRecipientsList->topLevelItemCount(); ++i) {
    DistributionListItem *item = static_cast<DistributionListItem *>(
        mRecipientsList->topLevelItem( i ));
    if ( item && item->checkState( 0 ) == Qt::Checked ) {
      isEmpty = false;
      break;
    }
  }

  if ( isEmpty ) {
    KMessageBox::information( this,
                              i18n("There are no recipients in your list. "
                                   "First select some recipients, "
                                   "then try again.") );
    return;
  }

  QString name = mTitleEdit->text();

  if ( name.isEmpty() ) {
    bool ok = false;
    name = KInputDialog::getText( i18n("New Distribution List"),
      i18n("Please enter name:"), QString(), &ok, this );
    if ( !ok || name.isEmpty() )
      return;
  }

  if ( ab->findDistributionListByName( name ) ) {
    KMessageBox::information( this,
      i18n( "<qt>Distribution list with the given name <b>%1</b> "
        "already exists. Please select a different name.</qt>", name ) );
    return;
  }

  KABC::DistributionList *dlist = ab->createDistributionList( name );

  for (int i = 0; i < mRecipientsList->topLevelItemCount(); ++i) {
    DistributionListItem *item = static_cast<DistributionListItem *>(
        mRecipientsList->topLevelItem( i ));
    if ( item && item->checkState( 0 ) == Qt::Checked ) {
      kDebug() << item->addressee().fullEmail() << item->addressee().uid();
      if ( item->isTransient() ) {
        ab->insertAddressee( item->addressee() );
      }
      if ( item->email() == item->addressee().preferredEmail() ) {
        dlist->insertEntry( item->addressee() );
      } else {
        dlist->insertEntry( item->addressee(), item->email() );
      }
    }
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
    kWarning(5006) <<" Couldn't save new addresses in the distribution list just created to the address book";

  close();
}
