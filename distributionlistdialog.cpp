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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "distributionlistdialog.h"

#include <libemailfunctions/email.h>
#include <kabc/stdaddressbook.h>
#include <kabc/distributionlist.h>

#include <klistview.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kinputdialog.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>

class NewStyleRecipientItem : public QCheckListItem
{
  public:
    NewStyleRecipientItem( QListView *list, const KABC::Addressee &addressee )
      : QCheckListItem( list, addressee.realName(), CheckBox ),
        mAddressee( addressee )
    {
      setText( 1, addressee.preferredEmail() );
    }

    KABC::Addressee addressee() const
    {
      return mAddressee;
    }
    
  private:
    KABC::Addressee mAddressee;
};


DistributionListDialog::DistributionListDialog( QWidget *parent )
  : KDialogBase( Plain, i18n("Create Distribution List"), User1 | Cancel,
                 Cancel, parent, 0, false, false, i18n("Create List") )
{
  QFrame *topFrame = plainPage();
  
  QBoxLayout *topLayout = new QVBoxLayout( topFrame );
  topLayout->setSpacing( spacingHint() );
  
  QBoxLayout *titleLayout = new QHBoxLayout( topLayout );
  
  QLabel *label = new QLabel( i18n("Name:"), topFrame );
  titleLayout->addWidget( label );
  
  mTitleEdit = new QLineEdit( topFrame );
  titleLayout->addWidget( mTitleEdit );
  
  mRecipientsList = new KListView( topFrame );
  mRecipientsList->addColumn( i18n("Recipients") );
  topLayout->addWidget( mRecipientsList );
}

void DistributionListDialog::setRecipients( const Recipient::List &recipients )
{
  Recipient::List::ConstIterator it;
  for( it = recipients.begin(); it != recipients.end(); ++it ) {
    QStringList emails = KPIM::splitEmailAddrList( (*it).email() );
    QStringList::ConstIterator it2;
    for( it2 = emails.begin(); it2 != emails.end(); ++it2 ) {
      QString email = KPIM::getEmailAddress( *it2 );
      if ( !email.isEmpty() ) {
        KABC::Addressee::List addressees =
          KABC::StdAddressBook::self( true )->findByEmail( email );
        KABC::Addressee::List::ConstIterator it3;
        for( it3 = addressees.begin(); it3 != addressees.end(); ++it3 ) {
          NewStyleRecipientItem *item = new NewStyleRecipientItem( mRecipientsList, *it3 );
          if ( it3 == addressees.begin() ) item->setOn( true );
        }
      }
    }
  }
}

void DistributionListDialog::slotUser1()
{
  kdDebug() << "Create Distribution List '" << mTitleEdit->text() << "':"
    << endl;

  KABC::Addressee::List addressees;

  KABC::StdAddressBook *ab = KABC::StdAddressBook::self( true );

  QListViewItem *i = mRecipientsList->firstChild();
  while( i ) {
    NewStyleRecipientItem *item = static_cast<NewStyleRecipientItem *>( i );
    if ( item->isOn() ) {
      kdDebug() << "  " << item->addressee().fullEmail() << endl;
      addressees.append( item->addressee() );
    }
    i = i->nextSibling();
  }

  if ( addressees.isEmpty() ) {
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
      i18n("Please enter name:"), QString::null, &ok, this );
    if ( !ok || name.isEmpty() )
      return;
  }

  if ( manager.list( name ) ) {
    KMessageBox::information( this,
      i18n( "<qt>Distribution list with the given name <b>%1</b> "
        "already exists. Please select a different name.</qt>" ).arg( name ) );
    return;
  }

  KABC::DistributionList *dlist = new KABC::DistributionList( &manager, name );
  for ( KABC::Addressee::List::iterator itr = addressees.begin();
        itr != addressees.end(); ++itr ) {
    dlist->insertEntry( *itr );
  }

  manager.save();
}
