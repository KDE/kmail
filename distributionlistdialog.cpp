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

#include <config.h> // for KDEPIM_NEW_DISTRLISTS

#include "distributionlistdialog.h"

#include <libemailfunctions/email.h>
#include <kabc/resource.h>
#include <kabc/stdaddressbook.h>
#include <kabc/distributionlist.h>

#ifdef KDEPIM_NEW_DISTRLISTS
#include <libkdepim/distributionlist.h>
#endif

#include <klistview.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kinputdialog.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>

class DistributionListItem : public QCheckListItem
{
  public:
    DistributionListItem( QListView *list )
      : QCheckListItem( list, QString::null, CheckBox )
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
  : KDialogBase( Plain, i18n("Save Distribution List"), User1 | Cancel,
                 User1, parent, 0, false, false, i18n("Save List") )
{
  QFrame *topFrame = plainPage();
  
  QBoxLayout *topLayout = new QVBoxLayout( topFrame );
  topLayout->setSpacing( spacingHint() );
  
  QBoxLayout *titleLayout = new QHBoxLayout( topLayout );
  
  QLabel *label = new QLabel( i18n("Name:"), topFrame );
  titleLayout->addWidget( label );
  
  mTitleEdit = new QLineEdit( topFrame );
  titleLayout->addWidget( mTitleEdit );
  mTitleEdit->setFocus();
  
  mRecipientsList = new KListView( topFrame );
  mRecipientsList->addColumn( QString::null );
  mRecipientsList->addColumn( i18n("Name") );
  mRecipientsList->addColumn( i18n("Email") );
  topLayout->addWidget( mRecipientsList );
}

void DistributionListDialog::setRecipients( const Recipient::List &recipients )
{
  Recipient::List::ConstIterator it;
  for( it = recipients.begin(); it != recipients.end(); ++it ) {
    QStringList emails = KPIM::splitEmailAddrList( (*it).email() );
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

  QListViewItem *i = mRecipientsList->firstChild();
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

#ifndef KDEPIM_NEW_DISTRLISTS
  KABC::DistributionListManager manager( ab );
  manager.load();
#endif

  QString name = mTitleEdit->text();

  if ( name.isEmpty() ) {
    bool ok = false;
    name = KInputDialog::getText( i18n("New Distribution List"),
      i18n("Please enter name:"), QString::null, &ok, this );
    if ( !ok || name.isEmpty() )
      return;
  }

#ifdef KDEPIM_NEW_DISTRLISTS
  if ( !KPIM::DistributionList::findByName( ab, name ).isEmpty() ) {
#else
  if ( manager.list( name ) ) {
#endif
    KMessageBox::information( this,
      i18n( "<qt>Distribution list with the given name <b>%1</b> "
        "already exists. Please select a different name.</qt>" ).arg( name ) );
    return;
  }

#ifdef KDEPIM_NEW_DISTRLISTS
  KPIM::DistributionList dlist;
  dlist.setName( name );

  i = mRecipientsList->firstChild();
  while( i ) {
    DistributionListItem *item = static_cast<DistributionListItem *>( i );
    if ( item->isOn() ) {
      kdDebug() << "  " << item->addressee().fullEmail() << endl;
      if ( item->isTransient() ) {
        ab->insertAddressee( item->addressee() );
      }
      if ( item->email() == item->addressee().preferredEmail() ) {
        dlist.insertEntry( item->addressee() );
      } else {
        dlist.insertEntry( item->addressee(), item->email() );
      }
    }
    i = i->nextSibling();
  }

  ab->insertAddressee( dlist );
#else
  KABC::DistributionList *dlist = new KABC::DistributionList( &manager, name );
  i = mRecipientsList->firstChild();
  while( i ) {
    DistributionListItem *item = static_cast<DistributionListItem *>( i );
    if ( item->isOn() ) {
      kdDebug() << "  " << item->addressee().fullEmail() << endl;
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
#endif

  // FIXME: Ask the user which resource to save to instead of the default
  bool saveError = true;
  KABC::Ticket *ticket = ab->requestSaveTicket( 0 /*default resource */ );
  if ( ticket )
    if ( ab->save( ticket ) )
      saveError = false;
    else
      ab->releaseSaveTicket( ticket );

  if ( saveError )
    kdWarning(5006) << k_funcinfo << " Couldn't save new addresses in the distribution list just created to the address book" << endl;

#ifndef KDEPIM_NEW_DISTRLISTS
  manager.save();
#endif

  close();
}
