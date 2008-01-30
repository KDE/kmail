/*******************************************************************************
**
** Filename   : newfolderdialog.cpp
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

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qregexp.h>

#include <klocale.h>
#include <kdialogbase.h>
#include <kmessagebox.h>

#include "newfolderdialog.h"
#include "kmfolder.h"
#include "folderstorage.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmfoldermgr.h"
#include "kmfolderdir.h"
#include "folderstorage.h"
#include "kmailicalifaceimpl.h"
#include "kmacctimap.h"
#include "kmacctcachedimap.h"

using namespace KMail;

NewFolderDialog::NewFolderDialog( QWidget* parent, KMFolder *folder )
    : KDialogBase( parent, "new_folder_dialog", false, i18n( "New Folder" ),
                   KDialogBase::Ok|KDialogBase::Cancel,
                   KDialogBase::Ok, true ),
      mFolder( folder )
{
  setWFlags( getWFlags() | WDestructiveClose );
  if ( mFolder ) {
    setCaption( i18n("New Subfolder of %1").arg( mFolder->prettyURL() ) );
  }
  QWidget* privateLayoutWidget = new QWidget( this, "mTopLevelLayout" );
  privateLayoutWidget->setGeometry( QRect( 10, 10, 260, 80 ) );
  setMainWidget( privateLayoutWidget );
  mTopLevelLayout = new QVBoxLayout( privateLayoutWidget, 0, spacingHint(),
                                     "mTopLevelLayout");

  mNameHBox = new QHBoxLayout( 0, 0, 6, "mNameHBox");

  mNameLabel = new QLabel( privateLayoutWidget, "mNameLabel" );
  mNameLabel->setText( i18n( "&Name:" ) );
  mNameHBox->addWidget( mNameLabel );

  mNameLineEdit = new QLineEdit( privateLayoutWidget, "mNameLineEdit" );
  mNameLabel->setBuddy( mNameLineEdit );
  QWhatsThis::add( mNameLineEdit, i18n( "Enter a name for the new folder." ) );
  mNameLineEdit->setFocus();
  mNameHBox->addWidget( mNameLineEdit );
  mTopLevelLayout->addLayout( mNameHBox );
  connect( mNameLineEdit, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotFolderNameChanged( const QString & ) ) );

  if ( !mFolder ||
      ( mFolder->folderType() != KMFolderTypeImap &&
        mFolder->folderType() != KMFolderTypeCachedImap ) ) {
    mFormatHBox = new QHBoxLayout( 0, 0, 6, "mFormatHBox");
    mMailboxFormatLabel = new QLabel( privateLayoutWidget, "mMailboxFormatLabel" );
    mMailboxFormatLabel->setText( i18n( "Mailbox &format:" ) );
    mFormatHBox->addWidget( mMailboxFormatLabel );

    mFormatComboBox = new QComboBox( false, privateLayoutWidget, "mFormatComboBox" );
    mMailboxFormatLabel->setBuddy( mFormatComboBox );
    QWhatsThis::add( mFormatComboBox, i18n( "Select whether you want to store the messages in this folder as one file per  message (maildir) or as one big file (mbox). KMail uses maildir by default and this only needs to be changed in rare circumstances. If you are unsure, leave this option as-is." ) );

    mFormatComboBox->insertItem("mbox", 0);
    mFormatComboBox->insertItem("maildir", 1);
    // does the below make any sense?
    //  mFormatComboBox->insertItem("search", 2);
    {
      KConfig *config = KMKernel::config();
      KConfigGroupSaver saver(config, "General");
      int type = config->readNumEntry("default-mailbox-format", 1);
      if ( type < 0 || type > 1 ) type = 1;
      mFormatComboBox->setCurrentItem( type );
    }
    mFormatHBox->addWidget( mFormatComboBox );
    mTopLevelLayout->addLayout( mFormatHBox );
  }

  // --- contents -----
  if ( kmkernel->iCalIface().isEnabled() ) {
    mContentsHBox = new QHBoxLayout( 0, 0, 6, "mContentsHBox");

    mContentsLabel = new QLabel( privateLayoutWidget, "mContentsLabel" );
    mContentsLabel->setText( i18n( "Folder &contains:" ) );
    mContentsHBox->addWidget( mContentsLabel );

    mContentsComboBox = new QComboBox( false, privateLayoutWidget, "mContentsComboBox" );
    mContentsLabel->setBuddy( mContentsComboBox );
    QWhatsThis::add( mContentsComboBox, i18n( "Select whether you want the new folder to be used for mail storage of for storage of groupware items such as tasks or notes. The default is mail. If you are unsure, leave this option as-is." ) );
    mContentsComboBox->insertItem( i18n( "Mail" ) );
    mContentsComboBox->insertItem( i18n( "Calendar" ) );
    mContentsComboBox->insertItem( i18n( "Contacts" ) );
    mContentsComboBox->insertItem( i18n( "Notes" ) );
    mContentsComboBox->insertItem( i18n( "Tasks" ) );
    mContentsComboBox->insertItem( i18n( "Journal" ) );
    if ( mFolder ) // inherit contents type from papa
      mContentsComboBox->setCurrentItem( mFolder->storage()->contentsType() );
    mContentsHBox->addWidget( mContentsComboBox );
    mTopLevelLayout->addLayout( mContentsHBox );
  }

  if ( mFolder &&
      ( mFolder->folderType() == KMFolderTypeImap ||
        mFolder->folderType() == KMFolderTypeCachedImap ) ) {
    bool rootFolder = false;
    QStringList namespaces;
    if ( mFolder->folderType() == KMFolderTypeImap ) {
      ImapAccountBase* ai = static_cast<KMFolderImap*>(mFolder->storage())->account();
      if ( mFolder->storage() == ai->rootFolder() ) {
        rootFolder = true;
        namespaces = ai->namespaces()[ImapAccountBase::PersonalNS];
      }
    }
    if ( mFolder->folderType() == KMFolderTypeCachedImap ) {
      ImapAccountBase* ai = static_cast<KMFolderCachedImap*>(mFolder->storage())->account();
      if ( ai && mFolder->storage() == ai->rootFolder() ) {
        rootFolder = true;
        namespaces = ai->namespaces()[ImapAccountBase::PersonalNS];
      }
    }
    if ( rootFolder && namespaces.count() > 1 ) {
      mNamespacesHBox = new QHBoxLayout( 0, 0, 6, "mNamespaceHBox");

      mNamespacesLabel = new QLabel( privateLayoutWidget, "mNamespacesLabel" );
      mNamespacesLabel->setText( i18n( "Namespace for &folder:" ) );
      mNamespacesHBox->addWidget( mNamespacesLabel );

      mNamespacesComboBox = new QComboBox( false, privateLayoutWidget, "mNamespacesComboBox" );
      mNamespacesLabel->setBuddy( mNamespacesComboBox );
      QWhatsThis::add( mNamespacesComboBox, i18n( "Select the personal namespace the folder should be created in." ) );
      mNamespacesComboBox->insertStringList( namespaces );
      mNamespacesHBox->addWidget( mNamespacesComboBox );
      mTopLevelLayout->addLayout( mNamespacesHBox );
    } else {
      mNamespacesComboBox = 0;
    }
  }

  resize( QSize(282, 108).expandedTo(minimumSizeHint()) );
  clearWState( WState_Polished );
  slotFolderNameChanged( mNameLineEdit->text());
}

void NewFolderDialog::slotFolderNameChanged( const QString & _text)
{
  enableButtonOK( !_text.isEmpty() );
}

void NewFolderDialog::slotOk()
{
  const QString fldName = mNameLineEdit->text();
  if ( fldName.isEmpty() ) {
     KMessageBox::error( this, i18n("Please specify a name for the new folder."),
        i18n( "No Name Specified" ) );
     return;
  }

  // names of local folders must not contain a '/'
  if ( fldName.find( '/' ) != -1 &&
       ( !mFolder ||
         ( mFolder->folderType() != KMFolderTypeImap &&
           mFolder->folderType() != KMFolderTypeCachedImap ) ) ) {
    KMessageBox::error( this, i18n( "Folder names cannot contain the / (slash) character; please choose another folder name." ) );
    return;
  }

  // folder names must not start with a '.'
  if ( fldName.startsWith( "." ) ) {
    KMessageBox::error( this, i18n( "Folder names cannot start with a . (dot) character; please choose another folder name." ) );
    return;
  }

  // names of IMAP folders must not contain the folder delimiter
  if ( mFolder &&
      ( mFolder->folderType() == KMFolderTypeImap ||
        mFolder->folderType() == KMFolderTypeCachedImap ) ) {
    QString delimiter;
    if ( mFolder->folderType() == KMFolderTypeImap ) {
      KMAcctImap* ai = static_cast<KMFolderImap*>( mFolder->storage() )->account();
      if ( ai )
        delimiter = ai->delimiterForFolder( mFolder->storage() );
    } else {
      KMAcctCachedImap* ai = static_cast<KMFolderCachedImap*>( mFolder->storage() )->account();
      if ( ai )
        delimiter = ai->delimiterForFolder( mFolder->storage() );
    }
    if ( !delimiter.isEmpty() && fldName.find( delimiter ) != -1 ) {
      KMessageBox::error( this, i18n( "Your IMAP server does not allow the character '%1'; please choose another folder name." ).arg( delimiter ) );
      return;
    }
  }

  // default parent is Top Level local folders
  KMFolderDir * selectedFolderDir = &(kmkernel->folderMgr()->dir());
  // we got a parent, let's use that
  if ( mFolder )
    selectedFolderDir = mFolder->createChildFolder();

  // check if the folder already exists
  if( selectedFolderDir->hasNamedFolder( fldName )
      && ( !( mFolder
          && ( selectedFolderDir == mFolder->parent() )
          && ( mFolder->storage()->name() == fldName ) ) ) )
  {
    const QString message = i18n( "<qt>Failed to create folder <b>%1</b>, folder already exists.</qt>" ).arg(fldName);
    KMessageBox::error( this, message );
    return;
  }

  /* Ok, obvious errors caught, let's try creating it for real. */
  const QString message = i18n( "<qt>Failed to create folder <b>%1</b>."
            "</qt> " ).arg(fldName);
  bool success = false;
  KMFolder *newFolder = 0;

   if ( mFolder && mFolder->folderType() == KMFolderTypeImap ) {
    KMFolderImap* selectedStorage = static_cast<KMFolderImap*>( mFolder->storage() );
    KMAcctImap *anAccount = selectedStorage->account();
    // check if a connection is available BEFORE creating the folder
    if (anAccount->makeConnection() == ImapAccountBase::Connected) {
      newFolder = kmkernel->imapFolderMgr()->createFolder( fldName, false, KMFolderTypeImap, selectedFolderDir );
      if ( newFolder ) {
        QString imapPath, parent;
        if ( mNamespacesComboBox ) {
          // create folder with namespace
          parent = anAccount->addPathToNamespace( mNamespacesComboBox->currentText() );
          imapPath = anAccount->createImapPath( parent, fldName );
        } else {
          imapPath = anAccount->createImapPath( selectedStorage->imapPath(), fldName );
        }
        KMFolderImap* newStorage = static_cast<KMFolderImap*>( newFolder->storage() );
        selectedStorage->createFolder(fldName, parent); // create it on the server
        newStorage->initializeFrom( selectedStorage, imapPath, QString::null );
        static_cast<KMFolderImap*>(mFolder->storage())->setAccount( selectedStorage->account() );
        success = true;
      }
    }
  } else if ( mFolder && mFolder->folderType() == KMFolderTypeCachedImap ) {
    newFolder = kmkernel->dimapFolderMgr()->createFolder( fldName, false, KMFolderTypeCachedImap, selectedFolderDir );
    if ( newFolder ) {
      KMFolderCachedImap* selectedStorage = static_cast<KMFolderCachedImap*>( mFolder->storage() );
      KMFolderCachedImap* newStorage = static_cast<KMFolderCachedImap*>( newFolder->storage() );
      newStorage->initializeFrom( selectedStorage );
      if ( mNamespacesComboBox ) {
        // create folder with namespace
        QString path = selectedStorage->account()->createImapPath(
            mNamespacesComboBox->currentText(), fldName );
        newStorage->setImapPathForCreation( path );
      }
      success = true;
    }
  } else {
    // local folder
    if (mFormatComboBox->currentItem() == 1)
      newFolder = kmkernel->folderMgr()->createFolder(fldName, false, KMFolderTypeMaildir, selectedFolderDir );
    else
      newFolder = kmkernel->folderMgr()->createFolder(fldName, false, KMFolderTypeMbox, selectedFolderDir );
    if ( newFolder )
      success = true;
  }
  if ( !success ) {
    KMessageBox::error( this, message );
    return;
  }

  // Set type field
  if ( kmkernel->iCalIface().isEnabled() && mContentsComboBox ) {
    KMail::FolderContentsType type =
      static_cast<KMail::FolderContentsType>( mContentsComboBox->currentItem() );
    newFolder->storage()->setContentsType( type );
    newFolder->storage()->writeConfig(); // connected slots will read it
  }
  KDialogBase::slotOk();
}

#include "newfolderdialog.moc"
