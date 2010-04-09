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

#include <akonadi/collectiondialog.h>
#include <akonadi/contact/contactgroupsearchjob.h>
#include <akonadi/contact/contactsearchjob.h>
#include <akonadi/itemcreatejob.h>
#include <kpimutils/email.h>

#include <KLocale>
#include <KDebug>
#include <KLineEdit>
#include <KMessageBox>
#include <KInputDialog>

#include <QLabel>
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
  setCaption( i18nc("@title:window", "Save Distribution List") );
  setButtons( User1 | Cancel );
  setDefaultButton( User1 );
  setModal( false );
  setButtonText( User1, i18nc("@action:button","Save List") );

  QBoxLayout *topLayout = new QVBoxLayout( topFrame );
  topLayout->setSpacing( spacingHint() );

  QBoxLayout *titleLayout = new QHBoxLayout();
  titleLayout->setSpacing( spacingHint() );
  topLayout->addItem( titleLayout );

  QLabel *label = new QLabel(
    i18nc("@label:textbox Name of the distribution list.", "&Name:"), topFrame );
  titleLayout->addWidget( label );

  mTitleEdit = new KLineEdit( topFrame );
  titleLayout->addWidget( mTitleEdit );
  mTitleEdit->setFocus();
  mTitleEdit->setClearButtonShown( true );
  label->setBuddy( mTitleEdit );

  mRecipientsList = new QTreeWidget( topFrame );
  mRecipientsList->setHeaderLabels(
                     QStringList() << i18nc( "@title:column Name of the recipient","Name" )
                                   << i18nc( "@title:column Email of the recipient", "Email" )
                                  );
  mRecipientsList->setRootIsDecorated( false );
  topLayout->addWidget( mRecipientsList );
  connect( this, SIGNAL( user1Clicked() ),
           this, SLOT( slotUser1() ) );
}

void DistributionListDialog::setRecipients( const Recipient::List &recipients )
{
  Recipient::List::ConstIterator it;
  for( it = recipients.constBegin(); it != recipients.constEnd(); ++it ) {
    QStringList emails = KPIMUtils::splitAddressList( (*it).email() );
    QStringList::ConstIterator it2;
    for( it2 = emails.constBegin(); it2 != emails.constEnd(); ++it2 ) {
      QString name;
      QString email;
      KABC::Addressee::parseEmailAddress( *it2, name, email );
      if ( !email.isEmpty() ) {
        Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob(this);
        job->setQuery( Akonadi::ContactSearchJob::Email, email, Akonadi::ContactSearchJob::ExactMatch );
        job->setProperty( "name", name );
        job->setProperty( "email", email );
        connect( job, SIGNAL( result( KJob* ) ), SLOT( slotDelayedSetRecipients( KJob* ) ) );
      }
    }
  }
}

void DistributionListDialog::slotDelayedSetRecipients( KJob *job )
{
  const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob*>( job );
  const KABC::Addressee::List contacts = searchJob->contacts();

  const QString name = searchJob->property( "name" ).toString();
  const QString email = searchJob->property( "email" ).toString();

  DistributionListItem *item = new DistributionListItem( mRecipientsList );

  if ( contacts.isEmpty() ) {
    KABC::Addressee contact;
    contact.setNameFromString( name );
    contact.insertEmail( email );
    item->setTransientAddressee( contact, email );
    item->setCheckState( 0, Qt::Checked );
  } else {
    bool isFirst = true;
    foreach ( const KABC::Addressee &contact, contacts ) {
      item->setAddressee( contact, email );
      if ( isFirst ) {
        item->setCheckState( 0, Qt::Checked );
        isFirst = false;
      }
    }
  }
}

void DistributionListDialog::slotUser1()
{
  bool isEmpty = true;

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
                              i18nc("@info", "There are no recipients in your list. "
                                   "First select some recipients, "
                                   "then try again.") );
    return;
  }

  QString name = mTitleEdit->text();

  if ( name.isEmpty() ) {
    bool ok = false;
    name = KInputDialog::getText( i18nc("@title:window","New Distribution List"),
      i18nc("@label:textbox","Please enter name:"), QString(), &ok, this );
    if ( !ok || name.isEmpty() )
      return;
  }

  Akonadi::ContactGroupSearchJob *job = new Akonadi::ContactGroupSearchJob();
  job->setQuery( Akonadi::ContactGroupSearchJob::Name, name );
  job->setProperty( "name", name );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( slotDelayedUser1( KJob* ) ) );
}

void DistributionListDialog::slotDelayedUser1( KJob *job )
{
  const Akonadi::ContactGroupSearchJob *searchJob = qobject_cast<Akonadi::ContactGroupSearchJob*>( job );
  const QString name = searchJob->property( "name" ).toString();

  if ( !searchJob->contactGroups().isEmpty() ) {
    KMessageBox::information( this,
      i18nc( "@info", "<para>Distribution list with the given name <resource>%1</resource> "
        "already exists. Please select a different name.</para>", name ) );
    return;
  }

  Akonadi::CollectionDialog dlg( this );
  dlg.setMimeTypeFilter( QStringList() << KABC::Addressee::mimeType() << KABC::ContactGroup::mimeType() );
  dlg.setAccessRightsFilter( Akonadi::Collection::CanCreateItem );
  dlg.setDescription( i18n( "Select the address book folder to store the contact group in:" ) );
  if ( !dlg.exec() )
    return;

  const Akonadi::Collection targetCollection = dlg.selectedCollection();

  KABC::ContactGroup group( name );

  for ( int i = 0; i < mRecipientsList->topLevelItemCount(); ++i ) {
    DistributionListItem *item = static_cast<DistributionListItem *>( mRecipientsList->topLevelItem( i ) );
    if ( item && item->checkState( 0 ) == Qt::Checked ) {
      kDebug() << item->addressee().fullEmail() << item->addressee().uid();
      if ( item->isTransient() ) {
        Akonadi::Item contactItem( KABC::Addressee::mimeType() );
        contactItem.setPayload<KABC::Addressee>( item->addressee() );

        Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( contactItem, targetCollection );
        job->exec();

        group.append( KABC::ContactGroup::ContactGroupReference( QString::number( job->item().id() ) ) );
      } else {
        group.append( KABC::ContactGroup::Data( item->addressee().realName(), item->email() ) );
      }
    }
  }

  Akonadi::Item groupItem( KABC::ContactGroup::mimeType() );
  groupItem.setPayload<KABC::ContactGroup>( group );

  new Akonadi::ItemCreateJob( groupItem, targetCollection );

  close();
}
