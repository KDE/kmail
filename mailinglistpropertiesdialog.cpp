/*******************************************************************************
**
** Filename   : mailinglistpropertiesdialog.cpp
** Created on : 30 January, 2005
** Copyright  : (c) 2005 Till Adam
** Email      : adam@kde.org
**
*******************************************************************************/

/*******************************************************************************
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
#include <qlayout.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <keditlistbox.h>
#include <kdialogbase.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kmcommands.h>

#include "kmfolder.h"
#include "mailinglist-magic.h"
#include "mailinglistpropertiesdialog.h"

using namespace KMail;

MailingListFolderPropertiesDialog::MailingListFolderPropertiesDialog( QWidget* parent, KMFolder *folder )
    : KDialogBase( parent, "mailinglist_properties", false, i18n( "Mailinglist Folder Properties" ), 
                   KDialogBase::Ok|KDialogBase::Cancel, 
                   KDialogBase::Ok, true ),
      mFolder( folder )
{
  setWFlags( getWFlags() | WDestructiveClose );
  QLabel* label;
  mLastItem = 0;

  QVBoxLayout *topLayout = new QVBoxLayout( this, 0, KDialog::spacingHint(),
                                            "topLayout" );

  QGroupBox *mlGroup = new QGroupBox( i18n("Associated Mailing List" ), this );
  mlGroup->setColumnLayout( 0,  Qt::Vertical );
  QVBoxLayout *groupLayout = new QVBoxLayout( mlGroup->layout() );
  topLayout->addWidget( mlGroup );
  setMainWidget( mlGroup );

  mHoldsMailingList = new QCheckBox( i18n("&Folder holds a mailing list"), mlGroup );
  QObject::connect( mHoldsMailingList, SIGNAL(toggled(bool)),
                    SLOT(slotHoldsML(bool)) );
  groupLayout->addWidget( mHoldsMailingList );

  groupLayout->addSpacing( 10 );

  mDetectButton = new QPushButton( i18n("Detect Automatically"), mlGroup );
  mDetectButton->setEnabled( false );
  QObject::connect( mDetectButton, SIGNAL(pressed()), SLOT(slotDetectMailingList()) );
  groupLayout->addWidget( mDetectButton, 0, Qt::AlignHCenter );

  groupLayout->addSpacing( 10 );

  QHBoxLayout *handlerLayout = new QHBoxLayout( groupLayout );
  //FIXME: add QWhatsThis
  label = new QLabel( i18n("Preferred handler: "), mlGroup );
  QObject::connect( mHoldsMailingList, SIGNAL(toggled(bool)),
		    label, SLOT(setEnabled(bool)) );
  handlerLayout->addWidget( label, 0, Qt::AlignCenter );
  mMLHandlerCombo = new QComboBox( mlGroup );
  mMLHandlerCombo->insertItem( i18n("KMail"), MailingList::KMail );
  mMLHandlerCombo->insertItem( i18n("Browser"), MailingList::Browser );
  mMLHandlerCombo->setEnabled( false );
  handlerLayout->addWidget( mMLHandlerCombo, 0, Qt::AlignCenter );
  QObject::connect( mMLHandlerCombo, SIGNAL(activated(int)),
                    SLOT(slotMLHandling(int)) );
  label->setBuddy( mMLHandlerCombo );

  //groupLayout->addSpacing( 10 );

  QVBoxLayout *idLayout = new QVBoxLayout( groupLayout );
  label = new QLabel( i18n("<b>Mailing list description: </b>"), mlGroup );
  label->setEnabled( false );
  QObject::connect( mHoldsMailingList, SIGNAL(toggled(bool)),
		    label, SLOT(setEnabled(bool)) );
  idLayout->addWidget( label, 0 );
  mMLId = new QLabel( label, "", mlGroup );
  idLayout->addWidget( mMLId, 0 );
  mMLId->setEnabled( false );

  QGridLayout *mlLayout = new QGridLayout( groupLayout );
  mlLayout->setSpacing( 6 );
  // mlLayout->setColStretch(0, 1);
  // mlLayout->setColStretch(1, 100);

  label = new QLabel( i18n("&Address type:"), mlGroup );
  label->setEnabled(false);
  QObject::connect( mHoldsMailingList, SIGNAL(toggled(bool)),
		    label, SLOT(setEnabled(bool)) );
  mlLayout->addWidget( label, 0, 0, Qt::AlignTop );
  mAddressCombo = new QComboBox( mlGroup );
  label->setBuddy( mAddressCombo );
  mlLayout->addWidget( mAddressCombo, 0, 1, Qt::AlignTop );
  mAddressCombo->setEnabled( false );

  //FIXME: if the mailing list actions have either KAction's or toolbar buttons
  //       associated with them - remove this button since it's really silly
  //       here
  QPushButton *handleButton = new QPushButton( i18n( "Invoke Handler" ), mlGroup );
  handleButton->setEnabled( false );
  if( mFolder)
  {
  	QObject::connect( mHoldsMailingList, SIGNAL(toggled(bool)),
			    handleButton, SLOT(setEnabled(bool)) );
  	QObject::connect( handleButton, SIGNAL(clicked()),
                    SLOT(slotInvokeHandler()) );
  }
  mlLayout->addWidget( handleButton, 0, 2, Qt::AlignTop );

  mEditList = new KEditListBox( mlGroup );
  mEditList->setEnabled( false );
  mlLayout->addMultiCellWidget( mEditList, 1, 2, 0, 3, Qt::AlignTop );

  QStringList el;

  //Order is important because the activate handler and fillMLFromWidgets
  //depend on it
  el << i18n( "Post to List" )
     << i18n( "Subscribe to List" )
     << i18n( "Unsubscribe from List" )
     << i18n( "List Archives" )
     << i18n( "List Help" );
  mAddressCombo->insertStringList( el );
  QObject::connect( mAddressCombo, SIGNAL(activated(int)),
                    SLOT(slotAddressChanged(int)) );

  load();
  resize( QSize(295, 204).expandedTo(minimumSizeHint()) );
  clearWState( WState_Polished );
}

void MailingListFolderPropertiesDialog::slotOk()
{
  save();
  KDialogBase::slotOk();
}

void MailingListFolderPropertiesDialog::load()
{
  if (mFolder) mMailingList = mFolder->mailingList();
  mMLId->setText( (mMailingList.id().isEmpty() ? i18n("Not available") : mMailingList.id()) );
  mMLHandlerCombo->setCurrentItem( mMailingList.handler() );
  mEditList->insertStringList( mMailingList.postURLS().toStringList() );

  mAddressCombo->setCurrentItem( mLastItem );
  mHoldsMailingList->setChecked( mFolder && mFolder->isMailingListEnabled() );
}

//-----------------------------------------------------------------------------
bool MailingListFolderPropertiesDialog::save()
{
  if( mFolder )
  {
    // settings for mailingList
    mFolder->setMailingListEnabled( mHoldsMailingList && mHoldsMailingList->isChecked() );
    fillMLFromWidgets();
    mFolder->setMailingList( mMailingList );
  }
  return true;
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
  int num = mFolder->count();

  kdDebug(5006)<<k_funcinfo<<" Detecting mailing list"<<endl;

  /* FIXME Till - make work without the folder tree
  // first try the currently selected message
  KMFolderTree *folderTree = static_cast<KMFolderTree *>( mDlg->parent() );
  int curMsgIdx = folderTree->mainWidget()->headers()->currentItemIndex();
  if ( curMsgIdx > 0 ) {
    KMMessage *mes = mFolder->getMsg( curMsgIdx );
    if ( mes )
      mMailingList = MailingList::detect( mes );
  }
  */

  // next try the 5 most recently added messages
  if ( !( mMailingList.features() & MailingList::Post ) ) {
    const int maxchecks = 5;
    for( int i = --num; i > num-maxchecks; --i ) {
      KMMessage *mes = mFolder->getMsg( i );
      if ( !mes )
        continue;
      mMailingList = MailingList::detect( mes );
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
  for ( QStringList::ConstIterator it = oldList.begin();
        it != oldList.end(); ++it ) {
    if ( !(*it).startsWith("http:") && !(*it).startsWith("https:") &&
         !(*it).startsWith("mailto:") && ( (*it).find('@') != -1 ) ) {
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

  //mMailingList.setHandler( static_cast<MailingList::Handler>( mMLHandlerCombo->currentItem() ) );
  switch ( mLastItem ) {
  case 0:
    mMailingList.setPostURLS( mEditList->items() );
    break;
  case 1:
    mMailingList.setSubscribeURLS( mEditList->items() );
    break;
  case 2:
    mMailingList.setUnsubscribeURLS( mEditList->items() );
    break;
  case 3:
    mMailingList.setArchiveURLS( mEditList->items() );
    break;
  case 4:
    mMailingList.setHelpURLS( mEditList->items() );
    break;
  default:
    kdWarning( 5006 )<<"Wrong entry in the mailing list entry combo!"<<endl;
  }
}

void MailingListFolderPropertiesDialog::fillEditBox()
{
  mEditList->clear();
  switch ( mAddressCombo->currentItem() ) {
  case 0:
    mEditList->insertStringList( mMailingList.postURLS().toStringList() );
    break;
  case 1:
    mEditList->insertStringList( mMailingList.subscribeURLS().toStringList() );
    break;
  case 2:
    mEditList->insertStringList( mMailingList.unsubscribeURLS().toStringList() );
    break;
  case 3:
    mEditList->insertStringList( mMailingList.archiveURLS().toStringList() );
    break;
  case 4:
    mEditList->insertStringList( mMailingList.helpURLS().toStringList() );
    break;
  default:
    kdWarning( 5006 )<<"Wrong entry in the mailing list entry combo!"<<endl;
  }
}

void MailingListFolderPropertiesDialog::slotInvokeHandler()
{
  KMCommand *command =0;
  switch ( mAddressCombo->currentItem() ) {
  case 0:
    command = new KMMailingListPostCommand( this, mFolder );
    break;
  case 1:
    command = new KMMailingListSubscribeCommand( this, mFolder );
    break;
  case 2:
    command = new KMMailingListUnsubscribeCommand( this, mFolder );
    break;
  case 3:
    command = new KMMailingListArchivesCommand( this, mFolder );
    break;
  case 4:
    command = new KMMailingListHelpCommand( this, mFolder );
    break;
  default:
    kdWarning( 5006 )<<"Wrong entry in the mailing list entry combo!"<<endl;
  }
  if ( command ) command->start();
}

#include "mailinglistpropertiesdialog.moc"
