/*******************************************************************************
**
** Filename   : mailinglistpropertiesdialog.cpp
** Created on : 30 January, 2005
** Copyright  : (c) 2005 Till Adam
** Email      : adam@kde.org
**
*******************************************************************************/

/*******************************************************************************
**  Copyright (c) 2011 Montel Laurent <montel@kde.org>

**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
*******************************************************************************/

#include "mailinglistpropertiesdialog.h"

#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>

#include <kcombobox.h>
#include <klocale.h>
#include <keditlistwidget.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include "mailutil.h"
#include "util.h"
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kmime/messageparts.h>

using namespace KMail;
using namespace MailCommon;

MailingListFolderPropertiesDialog::MailingListFolderPropertiesDialog( QWidget* parent, const QSharedPointer<FolderCollection> &col )
    : KDialog( parent ),
      mFolder( col )
{
  setCaption( i18n( "Mailinglist Folder Properties" ) );
  setButtons( Ok | Cancel );
  setObjectName( "mailinglist_properties" );
  setModal( false );
  setAttribute( Qt::WA_DeleteOnClose );
  QLabel* label;
  mLastItem = 0;

  connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );

  QVBoxLayout *topLayout = new QVBoxLayout( mainWidget() );
  topLayout->setObjectName( "topLayout" );
  topLayout->setSpacing( spacingHint() );

  QGroupBox *mlGroup = new QGroupBox( i18n("Associated Mailing List" ), this );

//  mlGroup->setColumnLayout( 0,  Qt::Vertical );
  QGridLayout *groupLayout = new QGridLayout( mlGroup );
  //mlGroup->layout()->addItem( groupLayout );
  groupLayout->setSpacing( spacingHint() );
  topLayout->addWidget( mlGroup );
  setMainWidget( mlGroup );

  mHoldsMailingList = new QCheckBox( i18n("&Folder holds a mailing list"), mlGroup );
  connect( mHoldsMailingList, SIGNAL(toggled(bool)),
           SLOT(slotHoldsML(bool)) );
  groupLayout->addWidget( mHoldsMailingList, 0, 0, 1, 3 );

  groupLayout->addItem( new QSpacerItem( 0, 10 ), 1, 0 );

  mDetectButton = new QPushButton( i18n("Detect Automatically"), mlGroup );
  mDetectButton->setEnabled( false );
  connect( mDetectButton, SIGNAL(pressed()),
           SLOT(slotDetectMailingList()) );
  groupLayout->addWidget( mDetectButton, 2, 1 );

  groupLayout->addItem( new QSpacerItem( 0, 10 ), 3, 0 );

  label = new QLabel( i18n("Mailing list description:"), mlGroup );
  label->setEnabled( false );
  connect( mHoldsMailingList, SIGNAL(toggled(bool)),
           label, SLOT(setEnabled(bool)) );
  groupLayout->addWidget( label, 4, 0 );
  mMLId = new QLabel( "", mlGroup );
  mMLId->setBuddy( label );
  groupLayout->addWidget( mMLId, 4, 1, 1, 2 );
  mMLId->setEnabled( false );

  //FIXME: add QWhatsThis
  label = new QLabel( i18n("Preferred handler:"), mlGroup );
  label->setEnabled(false);
  connect( mHoldsMailingList, SIGNAL(toggled(bool)),
           label, SLOT(setEnabled(bool)) );
  groupLayout->addWidget( label, 5, 0 );
  mMLHandlerCombo = new KComboBox( mlGroup );
  mMLHandlerCombo->addItem( i18n("KMail"), MailingList::KMail );
  mMLHandlerCombo->addItem( i18n("Browser"), MailingList::Browser );
  mMLHandlerCombo->setEnabled( false );
  groupLayout->addWidget( mMLHandlerCombo, 5, 1, 1, 2 );
  connect( mMLHandlerCombo, SIGNAL(activated(int)),
           SLOT(slotMLHandling(int)) );
  label->setBuddy( mMLHandlerCombo );

  label = new QLabel( i18n("&Address type:"), mlGroup );
  label->setEnabled(false);
  connect( mHoldsMailingList, SIGNAL(toggled(bool)),
           label, SLOT(setEnabled(bool)) );
  groupLayout->addWidget( label, 6, 0 );
  mAddressCombo = new KComboBox( mlGroup );
  label->setBuddy( mAddressCombo );
  groupLayout->addWidget( mAddressCombo, 6, 1 );
  mAddressCombo->setEnabled( false );

  //FIXME: if the mailing list actions have either KAction's or toolbar buttons
  //       associated with them - remove this button since it's really silly
  //       here
  QPushButton *handleButton = new QPushButton( i18n( "Invoke Handler" ), mlGroup );
  handleButton->setEnabled( false );
  if( mFolder)
  {
    connect( mHoldsMailingList, SIGNAL(toggled(bool)),
             handleButton, SLOT(setEnabled(bool)) );
    connect( handleButton, SIGNAL(clicked()),
             SLOT(slotInvokeHandler()) );
  }
  groupLayout->addWidget( handleButton, 6, 2 );

  mEditList = new KEditListWidget( mlGroup );
  mEditList->setEnabled( false );
  groupLayout->addWidget( mEditList, 7, 0, 1, 4 );

  QStringList el;

  //Order is important because the activate handler and fillMLFromWidgets
  //depend on it
  el << i18n( "Post to List" )
     << i18n( "Subscribe to List" )
     << i18n( "Unsubscribe From List" )
     << i18n( "List Archives" )
     << i18n( "List Help" );
  mAddressCombo->addItems( el );
  connect( mAddressCombo, SIGNAL(activated(int)),
           SLOT(slotAddressChanged(int)) );

  load();
  resize( QSize(295, 204).expandedTo(minimumSizeHint()) );
  setAttribute(Qt::WA_WState_Polished);
}

MailingListFolderPropertiesDialog::~MailingListFolderPropertiesDialog()
{
}

void MailingListFolderPropertiesDialog::slotOk()
{
  save();
}

void MailingListFolderPropertiesDialog::load()
{
  if ( mFolder )
    mMailingList = mFolder->mailingList();
  mMLId->setText( (mMailingList.id().isEmpty() ? i18n("Not available") : mMailingList.id()) );
  mMLHandlerCombo->setCurrentIndex( mMailingList.handler() );
  mEditList->insertStringList( mMailingList.postUrls().toStringList() );

  mAddressCombo->setCurrentIndex( mLastItem );
  mHoldsMailingList->setChecked( mFolder && mFolder->isMailingListEnabled() );
}

//-----------------------------------------------------------------------------
void MailingListFolderPropertiesDialog::save()
{
  if( mFolder )
  {
    // settings for mailingList
    mFolder->setMailingListEnabled( mHoldsMailingList && mHoldsMailingList->isChecked() );
    fillMLFromWidgets();
    mFolder->setMailingList( mMailingList );
  }
}

//----------------------------------------------------------------------------
void MailingListFolderPropertiesDialog::slotHoldsML( bool holdsML )
{
  mMLHandlerCombo->setEnabled( holdsML );
  if ( mFolder && mFolder->count() )
    mDetectButton->setEnabled( holdsML );
  mAddressCombo->setEnabled( holdsML );
  mEditList->setEnabled( holdsML );
  mMLId->setEnabled( holdsML );
}

//----------------------------------------------------------------------------
void MailingListFolderPropertiesDialog::slotDetectMailingList()
{
  if ( !mFolder ) return; // in case the folder was just created

  kDebug()<< "Detecting mailing list";

  // next try the 5 most recently added messages
  if ( !( mMailingList.features() & MailingList::Post ) ) {
    //FIXME not load all folder
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mFolder->collection(), this );
    job->fetchScope().fetchPayloadPart( Akonadi::MessagePart::Header );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotFetchDone(KJob*)) );
    //Don't allow to reactive it
    mDetectButton->setEnabled( false );
  }
  else {
    mMLId->setText( (mMailingList.id().isEmpty() ? i18n("Not available.") : mMailingList.id() ) );
    fillEditBox();
  }
}


void MailingListFolderPropertiesDialog::slotFetchDone( KJob* job )
{
  mDetectButton->setEnabled( true );
  if ( MailCommon::Util::showJobErrorMessage(job) ) {
    return;
  }
  Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob*>( job );
  Q_ASSERT( fjob );
  Akonadi::Item::List items = fjob->items();

  const int maxchecks = 5;
  int num = items.size();
  for ( int i = --num ; ( i > num - maxchecks ) && ( i >= 0 ); --i ) {
    Akonadi::Item item = items[i];
    if ( item.hasPayload<KMime::Message::Ptr>() ) {
      KMime::Message::Ptr message = item.payload<KMime::Message::Ptr>();
      mMailingList = MessageCore::MailingList::detect( message );
      if ( mMailingList.features() & MailingList::Post )
        break;
    }
  }
  if ( !(mMailingList.features() & MailingList::Post) ) {
    KMessageBox::error( this,
                        i18n("KMail was unable to detect a mailing list in this folder. "
                             "Please fill the addresses by hand.") );
  } else {
    mMLId->setText( (mMailingList.id().isEmpty() ? i18n("Not available.") : mMailingList.id() ) );
    fillEditBox();
  }

}

//----------------------------------------------------------------------------
void MailingListFolderPropertiesDialog::slotMLHandling( int element )
{
  mMailingList.setHandler( static_cast<MailingList::Handler>( element ) );
}

//----------------------------------------------------------------------------
void MailingListFolderPropertiesDialog::slotAddressChanged( int i )
{
  fillMLFromWidgets();
  fillEditBox();
  mLastItem = i;
}

//----------------------------------------------------------------------------
void MailingListFolderPropertiesDialog::fillMLFromWidgets()
{
  if ( !mHoldsMailingList->isChecked() )
    return;

  // make sure that email addresses are prepended by "mailto:"
  bool changed = false;
  QStringList oldList = mEditList->items();
  QStringList newList; // the correct string list
  QStringList::ConstIterator end = oldList.constEnd();
  for ( QStringList::ConstIterator it = oldList.constBegin(); it != end; ++it ) {
    if ( !(*it).startsWith(QLatin1String("http:")) && !(*it).startsWith(QLatin1String("https:")) &&
         !(*it).startsWith(QLatin1String("mailto:")) && ( (*it).contains(QLatin1Char('@')) ) ) {
      changed = true;
      newList << "mailto:" + *it;
    }
    else {
      newList << *it;
    }
  }
  if ( changed ) {
    mEditList->clear();
    mEditList->insertStringList( newList );
  }

  //mMailingList.setHandler( static_cast<MailingList::Handler>( mMLHandlerCombo->currentIndex() ) );
  switch ( mLastItem ) {
  case 0:
    mMailingList.setPostUrls( mEditList->items() );
    break;
  case 1:
    mMailingList.setSubscribeUrls( mEditList->items() );
    break;
  case 2:
    mMailingList.setUnsubscribeUrls( mEditList->items() );
    break;
  case 3:
    mMailingList.setArchiveUrls( mEditList->items() );
    break;
  case 4:
    mMailingList.setHelpUrls( mEditList->items() );
    break;
  default:
    kWarning()<<"Wrong entry in the mailing list entry combo!";
  }
}

void MailingListFolderPropertiesDialog::fillEditBox()
{
  mEditList->clear();
  switch ( mAddressCombo->currentIndex() ) {
  case 0:
    mEditList->insertStringList( mMailingList.postUrls().toStringList() );
    break;
  case 1:
    mEditList->insertStringList( mMailingList.subscribeUrls().toStringList() );
    break;
  case 2:
    mEditList->insertStringList( mMailingList.unsubscribeUrls().toStringList() );
    break;
  case 3:
    mEditList->insertStringList( mMailingList.archiveUrls().toStringList() );
    break;
  case 4:
    mEditList->insertStringList( mMailingList.helpUrls().toStringList() );
    break;
  default:
    kWarning()<<"Wrong entry in the mailing list entry combo!";
  }
}

void MailingListFolderPropertiesDialog::slotInvokeHandler()
{
  save();
  switch ( mAddressCombo->currentIndex() ) {
  case 0:
    KMail::Util::mailingListPost( mFolder );
    break;
  case 1:
    KMail::Util::mailingListSubscribe( mFolder );
    break;
  case 2:
    KMail::Util::mailingListUnsubscribe( mFolder );
    break;
  case 3:
    KMail::Util::mailingListArchives( mFolder );
    break;
  case 4:
    KMail::Util::mailingListHelp( mFolder );
    break;
  default:
    kWarning()<<"Wrong entry in the mailing list entry combo!";
  }
}

#include "mailinglistpropertiesdialog.moc"
