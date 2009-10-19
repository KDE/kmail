/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "foldercollection.h"
#include <kdebug.h>
#include "kmkernel.h"
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>

FolderCollection::FolderCollection( const Akonadi::Collection & col )
  : mCollection( col )
{
  mIdentity = KMKernel::self()->identityManager()->defaultIdentity().uoid();

  readConfig();
  connect( KMKernel::self()->identityManager(), SIGNAL( changed() ),
           this, SLOT( slotIdentitiesChanged() ) );
}

FolderCollection::~FolderCollection()
{
  kDebug()<<" FolderCollection::~FolderCollection";
  writeConfig();
}

Akonadi::Collection::Rights FolderCollection::rights() const
{
  return mCollection.rights();
}

Akonadi::CollectionStatistics FolderCollection::statistics() const
{
  return mCollection.statistics();
}

Akonadi::Collection FolderCollection::collection()
{
  return mCollection;
}

void FolderCollection::slotIdentitiesChanged()
{
  uint defaultIdentity = KMKernel::self()->identityManager()->defaultIdentity().uoid();

  // The default identity may have changed, therefore set it again
  // if necessary
  if ( mUseDefaultIdentity )
    mIdentity = defaultIdentity;

  // Fall back to the default identity if the one used currently is invalid
  if ( KMKernel::self()->identityManager()->identityForUoid( mIdentity ).isNull() ) {
    mIdentity = defaultIdentity;
    mUseDefaultIdentity = true;
  }
}

void FolderCollection::readConfig()
{
  KConfigGroup configGroup( KMKernel::config(), "Folder-" + QString::number( mCollection.id() ) );
  mExpireMessages = configGroup.readEntry( "ExpireMessages", false );
  mReadExpireAge = configGroup.readEntry( "ReadExpireAge", 3 );
  mReadExpireUnits = (ExpireUnits)configGroup.readEntry( "ReadExpireUnits", (int)expireMonths );
  mUnreadExpireAge = configGroup.readEntry( "UnreadExpireAge", 12 );
  mUnreadExpireUnits = (ExpireUnits)
      configGroup.readEntry( "UnreadExpireUnits", (int)expireNever );
  mExpireAction = configGroup.readEntry( "ExpireAction", "Delete") == "Move" ? ExpireMove : ExpireDelete;
  mExpireToFolderId = configGroup.readEntry( "ExpireToFolder" );

  mMailingListEnabled = configGroup.readEntry( "MailingListEnabled", false );
  mMailingList.readConfig( configGroup );

  mUseDefaultIdentity = configGroup.readEntry( "UseDefaultIdentity", true );
  uint defaultIdentity = KMKernel::self()->identityManager()->defaultIdentity().uoid();
  mIdentity = configGroup.readEntry("Identity", defaultIdentity );
  slotIdentitiesChanged();

  setUserWhoField( configGroup.readEntry( "WhoField" ), false );
  uint savedId = configGroup.readEntry( "Id", 0 );
#if 0 //TODO ???
  // make sure that we don't overwrite a valid id
  if ( savedId != 0 && mId == 0 )
    mId = savedId;
#endif
  mPutRepliesInSameFolder = configGroup.readEntry( "PutRepliesInSameFolder", false );
  mHideInSelectionDialog = configGroup.readEntry( "HideInSelectionDialog", false );
  mIgnoreNewMail = configGroup.readEntry( "IgnoreNewMail", false );


  QString shortcut( configGroup.readEntry( "Shortcut" ) );
  if ( !shortcut.isEmpty() ) {
    KShortcut sc( shortcut );
    setShortcut( sc );
  }
}

bool FolderCollection::isValid() const
{
  return mCollection.isValid();
}

QString FolderCollection::idString() const
{
  return QString::number( mCollection.id() );
}

void FolderCollection::writeConfig() const
{
  KConfigGroup configGroup( KMKernel::config(), "Folder-" + QString::number( mCollection.id() ) );
  configGroup.writeEntry("ExpireMessages", mExpireMessages);
  configGroup.writeEntry("ReadExpireAge", mReadExpireAge);
  configGroup.writeEntry("ReadExpireUnits", (int)mReadExpireUnits);
  configGroup.writeEntry("UnreadExpireAge", mUnreadExpireAge);
  configGroup.writeEntry("UnreadExpireUnits", (int)mUnreadExpireUnits);
  configGroup.writeEntry("ExpireAction", mExpireAction == ExpireDelete ? "Delete" : "Move");
  configGroup.writeEntry("ExpireToFolder", mExpireToFolderId);

  configGroup.writeEntry("MailingListEnabled", mMailingListEnabled);
  mMailingList.writeConfig( configGroup );


  configGroup.writeEntry( "UseDefaultIdentity", mUseDefaultIdentity );
#if 0 //TODO port it
  if ( !mUseDefaultIdentity && ( !mStorage || !mStorage->account() ||
                           mIdentity != mStorage->account()->identityId() ) )
      configGroup.writeEntry("Identity", mIdentity);
  else
      configGroup.deleteEntry("Identity");
#endif
  configGroup.writeEntry("WhoField", mUserWhoField);
#if 0 //TODO ????
  configGroup.writeEntry("Id", mId);
#endif
  configGroup.writeEntry( "PutRepliesInSameFolder", mPutRepliesInSameFolder );
  configGroup.writeEntry( "HideInSelectionDialog", mHideInSelectionDialog );
  configGroup.writeEntry( "IgnoreNewMail", mIgnoreNewMail );
  if ( !mShortcut.isEmpty() )
    configGroup.writeEntry( "Shortcut", mShortcut.toString() );
  else
    configGroup.deleteEntry( "Shortcut" );
}

void FolderCollection::setShortcut( const KShortcut &sc )
{
  if ( mShortcut != sc ) {
    mShortcut = sc;
    emit shortcutChanged( mCollection );
  }
}


void FolderCollection::setUserWhoField( const QString& whoField, bool writeConfig )
{
#if 0 //TODO port it
  if ( mUserWhoField == whoField && !whoField.isEmpty() )
    return;

  if ( whoField.isEmpty() )
  {
    // default setting
    const KPIMIdentities::Identity & identity =
      kmkernel->identityManager()->identityForUoidOrDefault( mIdentity );

    if ( isSystemFolder() && folderType() != KMFolderTypeImap ) {
      // local system folders
      if ( this == kmkernel->inboxFolder() ||
           this == kmkernel->trashFolder() )
        mWhoField = "From";
      if ( this == kmkernel->outboxFolder() ||
           this == kmkernel->sentFolder() ||
           this == kmkernel->templatesFolder() ||
           this == kmkernel->draftsFolder() )
        mWhoField = "To";
    } else if ( identity.drafts() == idString() ||
                identity.templates() == idString() ||
                identity.fcc() == idString() )
      // drafts, templates or sent of the identity
      mWhoField = "To";
    else
      mWhoField = "From";
  } else if ( whoField == "From" || whoField == "To" )
    // set the whoField according to the user-setting
    mWhoField = whoField;
  else {
    // this should not happen...
    kDebug() << "Illegal setting" << whoField << "for userWhoField!";
    return; // don't use the value
  }
  mUserWhoField = whoField;

  if (writeConfig)
    writeConfig();
  emit viewConfigChanged();
#endif
}

void FolderCollection::setUseDefaultIdentity( bool useDefaultIdentity )
{
  mUseDefaultIdentity = useDefaultIdentity;
  if ( mUseDefaultIdentity )
    mIdentity = KMKernel::self()->identityManager()->defaultIdentity().uoid();
  kmkernel->slotRequestConfigSync();
}

void FolderCollection::setIdentity( uint identity )
{
  mIdentity = identity;
  KMKernel::self()->slotRequestConfigSync();
}

uint FolderCollection::identity() const
{
#if 0  //Port to akonadi
  // if we don't have one set ourselves, check our account
  if ( mUseDefaultIdentity && mStorage )
    if ( KMAccount *act = mStorage->account() )
      return act->identityId();
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return mIdentity;
}


void FolderCollection::setAutoExpire( bool enabled )
{
  if( enabled != mExpireMessages ) {
    mExpireMessages = enabled;
    writeConfig();
  }
}

void FolderCollection::setUnreadExpireAge( int age )
{
  if( age >= 0 && age != mUnreadExpireAge ) {
    mUnreadExpireAge = age;
    writeConfig();
  }
}

void FolderCollection::setUnreadExpireUnits( ExpireUnits units )
{
  if (units >= expireNever && units < expireMaxUnits) {
    mUnreadExpireUnits = units;
    writeConfig();
  }
}

void FolderCollection::setReadExpireAge( int age )
{
  if( age >= 0 && age != mReadExpireAge ) {
    mReadExpireAge = age;
    writeConfig();
  }
}

void FolderCollection::setReadExpireUnits( ExpireUnits units )
{
  if (units >= expireNever && units <= expireMaxUnits) {
    mReadExpireUnits = units;
    writeConfig();
  }
}


void FolderCollection::setExpireAction( ExpireAction a )
{
  if ( a != mExpireAction ) {
    mExpireAction = a;
    writeConfig();
  }
}

void FolderCollection::setExpireToFolderId( const QString& id )
{
  if ( id != mExpireToFolderId ) {
    mExpireToFolderId = id;
    writeConfig();
  }
}
