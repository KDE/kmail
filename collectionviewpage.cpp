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


#include "collectionviewpage.h"

#include <akonadi/collection.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/private/collectionutils_p.h>
#include <kmkernel.h>
#include <QGroupBox>
#include <QHBoxLayout>
#include <kicondialog.h>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <KLocale>
#include <KDialog>
#include "messagelist/utils/aggregationcombobox.h"
#include "messagelist/utils/aggregationconfigbutton.h"
#include "messagelist/utils/themecombobox.h"
#include "messagelist/utils/themeconfigbutton.h"

#include "foldercollection.h"

CollectionViewPage::CollectionViewPage(QWidget * parent) :
    CollectionPropertiesPage( parent ), mFolderCollection( 0 )
{
  setPageTitle( i18nc( "@title:tab View settings for a folder.", "View" ) );
}

CollectionViewPage::~CollectionViewPage()
{
  delete mFolderCollection;
}

void CollectionViewPage::init(const Akonadi::Collection & col)
{
  mCurrentCollection = col;
  mIsLocalSystemFolder = KMKernel::self()->isSystemFolderCollection( col );

  qDebug()<<" mIsLocalSystemFolder :"<<mIsLocalSystemFolder;
  QVBoxLayout * topLayout = new QVBoxLayout( this );
  topLayout->setSpacing( KDialog::spacingHint() );
  topLayout->setMargin( 0 );
  // Musn't be able to edit details for non-resource, system folder.
  if ( !mIsLocalSystemFolder /*|| mIsResourceFolder*/ )
  {
    // icons
    mIconsCheckBox = new QCheckBox( i18n( "Use custom &icons"), this );
    mIconsCheckBox->setChecked( false );

    mNormalIconLabel = new QLabel( i18nc( "Icon used for folders with no unread messages.", "&Normal:" ), this );
    mNormalIconLabel->setEnabled( false );

    mNormalIconButton = new KIconButton( this );
    mNormalIconLabel->setBuddy( mNormalIconButton );
    mNormalIconButton->setIconType( KIconLoader::NoGroup, KIconLoader::Place, false );
    mNormalIconButton->setIconSize( 16 );
    mNormalIconButton->setStrictIconSize( true );
    mNormalIconButton->setFixedSize( 28, 28 );
    // Can't use iconset here.
    mNormalIconButton->setIcon( "folder" );
    mNormalIconButton->setEnabled( false );

    mUnreadIconLabel = new QLabel( i18nc( "Icon used for folders which do have unread messages.", "&Unread:" ), this );
    mUnreadIconLabel->setEnabled( false );

    mUnreadIconButton = new KIconButton( this );
    mUnreadIconLabel->setBuddy( mUnreadIconButton );
    mUnreadIconButton->setIconType( KIconLoader::NoGroup, KIconLoader::Place, false );
    mUnreadIconButton->setIconSize( 16 );
    mUnreadIconButton->setStrictIconSize( true );
    mUnreadIconButton->setFixedSize( 28, 28 );
    // Can't use iconset here.
    mUnreadIconButton->setIcon( "folder-open" );
    mUnreadIconButton->setEnabled( false );

    QHBoxLayout * iconHLayout = new QHBoxLayout();
    iconHLayout->addWidget( mIconsCheckBox );
    iconHLayout->addStretch( 2 );
    iconHLayout->addWidget( mNormalIconLabel );
    iconHLayout->addWidget( mNormalIconButton );
    iconHLayout->addWidget( mUnreadIconLabel );
    iconHLayout->addWidget( mUnreadIconButton );
    iconHLayout->addStretch( 1 );
    topLayout->addLayout( iconHLayout );

    connect( mIconsCheckBox, SIGNAL( toggled( bool ) ),
             mNormalIconLabel, SLOT( setEnabled( bool ) ) );
    connect( mIconsCheckBox, SIGNAL( toggled( bool ) ),
             mNormalIconButton, SLOT( setEnabled( bool ) ) );
    connect( mIconsCheckBox, SIGNAL( toggled( bool ) ),
             mUnreadIconButton, SLOT( setEnabled( bool ) ) );
    connect( mIconsCheckBox, SIGNAL( toggled( bool ) ),
             mUnreadIconLabel, SLOT( setEnabled( bool ) ) );

    connect( mNormalIconButton, SIGNAL( iconChanged( const QString& ) ),
             this, SLOT( slotChangeIcon( const QString& ) ) );
  }

  // sender or receiver column
  const QString senderReceiverColumnTip = i18n( "Show Sender/Receiver Column in List of Messages" );

  QLabel * senderReceiverColumnLabel = new QLabel( i18n( "Sho&w column:" ), this );
  mShowSenderReceiverComboBox = new KComboBox( this );
  mShowSenderReceiverComboBox->setToolTip( senderReceiverColumnTip );
  senderReceiverColumnLabel->setBuddy( mShowSenderReceiverComboBox );
  mShowSenderReceiverComboBox->insertItem( 0, i18nc( "@item:inlistbox Show default value.", "Default" ) );
  mShowSenderReceiverComboBox->insertItem( 1, i18nc( "@item:inlistbox Show sender.", "Sender" ) );
  mShowSenderReceiverComboBox->insertItem( 2, i18nc( "@item:inlistbox Show receiver.", "Receiver" ) );

  QHBoxLayout * senderReceiverColumnHLayout = new QHBoxLayout();
  senderReceiverColumnHLayout->addWidget( senderReceiverColumnLabel );
  senderReceiverColumnHLayout->addWidget( mShowSenderReceiverComboBox );
  topLayout->addLayout( senderReceiverColumnHLayout );

  // message list
  QGroupBox * messageListGroup = new QGroupBox( i18n( "Message List" ), this );
  QVBoxLayout * messageListGroupLayout = new QVBoxLayout( messageListGroup );
  messageListGroupLayout->setSpacing( KDialog::spacingHint() );
  topLayout->addWidget( messageListGroup );

  // message list aggregation
  mUseDefaultAggregationCheckBox = new QCheckBox( i18n( "Use default aggregation" ), messageListGroup );
  messageListGroupLayout->addWidget( mUseDefaultAggregationCheckBox );
  connect( mUseDefaultAggregationCheckBox, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotAggregationCheckboxChanged() ) );

  mAggregationComboBox = new MessageList::Utils::AggregationComboBox( messageListGroup );

  QLabel * aggregationLabel = new QLabel( i18n( "Aggregation" ), messageListGroup );
  aggregationLabel->setBuddy( mAggregationComboBox );

  using MessageList::Utils::AggregationConfigButton;
  AggregationConfigButton * aggregationConfigButton = new AggregationConfigButton( messageListGroup, mAggregationComboBox );
  // Make sure any changes made in the aggregations configure dialog are reflected in the combo.
  connect( aggregationConfigButton, SIGNAL( configureDialogCompleted() ),
           this, SLOT( slotSelectFolderAggregation() ) );

  QHBoxLayout * aggregationLayout = new QHBoxLayout();
  aggregationLayout->addWidget( aggregationLabel, 1 );
  aggregationLayout->addWidget( mAggregationComboBox, 1 );
  aggregationLayout->addWidget( aggregationConfigButton, 0 );
  messageListGroupLayout->addLayout( aggregationLayout );

  // message list theme
  mUseDefaultThemeCheckBox = new QCheckBox( i18n( "Use default theme" ), messageListGroup );
  messageListGroupLayout->addWidget( mUseDefaultThemeCheckBox );
  connect( mUseDefaultThemeCheckBox, SIGNAL( stateChanged( int ) ),
           this, SLOT( slotThemeCheckboxChanged() ) );

  mThemeComboBox = new MessageList::Utils::ThemeComboBox( messageListGroup );

  QLabel * themeLabel = new QLabel( i18n( "Theme" ), messageListGroup );
  themeLabel->setBuddy( mThemeComboBox );

  using MessageList::Utils::ThemeConfigButton;
  ThemeConfigButton * themeConfigButton = new ThemeConfigButton( messageListGroup, mThemeComboBox );
  // Make sure any changes made in the themes configure dialog are reflected in the combo.
  connect( themeConfigButton, SIGNAL( configureDialogCompleted() ),
           this, SLOT( slotSelectFolderTheme() ) );

  QHBoxLayout * themeLayout = new QHBoxLayout();
  themeLayout->addWidget( themeLabel, 1 );
  themeLayout->addWidget( mThemeComboBox, 1 );
  themeLayout->addWidget( themeConfigButton, 0 );
  messageListGroupLayout->addLayout( themeLayout );

  topLayout->addStretch( 100 );

}

#if 0
bool FolderDialogViewTab::save()
{
#ifdef OLD_MESSAGELIST
  // message list aggregation
  MessageListView::StorageModel messageListStorageModel( folder );
  const bool usePrivateAggregation = !mUseDefaultAggregationCheckBox->isChecked();
  mAggregationComboBox->writeStorageModelConfig( &messageListStorageModel, usePrivateAggregation );

  // message list theme
  const bool usePrivateTheme = !mUseDefaultThemeCheckBox->isChecked();
  mThemeComboBox->writeStorageModelConfig( &messageListStorageModel, usePrivateTheme );
#endif
  return true;
}
#endif

void CollectionViewPage::slotChangeIcon( const QString & icon )
{
    mUnreadIconButton->setIcon( icon );
}

void CollectionViewPage::slotAggregationCheckboxChanged()
{
  mAggregationComboBox->setEnabled( !mUseDefaultAggregationCheckBox->isChecked() );
}

void CollectionViewPage::slotThemeCheckboxChanged()
{
  mThemeComboBox->setEnabled( !mUseDefaultThemeCheckBox->isChecked() );
}

void CollectionViewPage::slotSelectFolderAggregation()
{
#ifdef OLD_MESSAGELIST
  MessageListView::StorageModel messageListStorageModel( mDlg->folder() );
  bool usesPrivateAggregation = false;
  mAggregationComboBox->readStorageModelConfig( &messageListStorageModel, usesPrivateAggregation );
  mUseDefaultAggregationCheckBox->setChecked( !usesPrivateAggregation );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void CollectionViewPage::slotSelectFolderTheme()
{
  bool usesPrivateTheme = false;
  mThemeComboBox->readStorageModelConfig( mCurrentCollection, usesPrivateTheme );
  mUseDefaultThemeCheckBox->setChecked( !usesPrivateTheme );
}

void CollectionViewPage::load( const Akonadi::Collection & col )
{
  init( col );
  mFolderCollection = new FolderCollection( col );
  if ( !mIsLocalSystemFolder ) {
    QString iconName;
    QString unreadIconName;
    bool iconWasEmpty = false;
    if ( col.hasAttribute<Akonadi::EntityDisplayAttribute>() ) {
      iconName = col.attribute<Akonadi::EntityDisplayAttribute>()->iconName();
      unreadIconName = col.attribute<Akonadi::EntityDisplayAttribute>()->activeIconName();
    }

    if ( iconName.isEmpty() ) {
      iconName = Akonadi::CollectionUtils::defaultIconName( col );
      iconWasEmpty = true;
    }
    mNormalIconButton->setIcon( iconName );

    if ( unreadIconName.isEmpty() ) {
      mUnreadIconButton->setIcon( iconName );
    }
    else {
      mUnreadIconButton->setIcon( unreadIconName );
    }

    mIconsCheckBox->setChecked( !iconWasEmpty );
  }
  // sender or receiver column
  const QString whoField = mFolderCollection->userWhoField();
  if ( whoField.isEmpty() )
    mShowSenderReceiverComboBox->setCurrentIndex( 0 );
  else if ( whoField == "From" )
    mShowSenderReceiverComboBox->setCurrentIndex( 1 );
  else if ( whoField == "To" )
    mShowSenderReceiverComboBox->setCurrentIndex( 2 );

  // message list aggregation
  slotSelectFolderAggregation();

  // message list theme
  slotSelectFolderTheme();
}

void CollectionViewPage::save( Akonadi::Collection & col )
{
  if ( !mIsLocalSystemFolder ) {
    if ( mIconsCheckBox->isChecked() ) {
      col.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Collection::AddIfMissing )->setIconName( mNormalIconButton->icon() );
      col.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Collection::AddIfMissing )->setActiveIconName( mUnreadIconButton->icon() );
    }
    else if ( col.hasAttribute<Akonadi::EntityDisplayAttribute>() ) {
      col.attribute<Akonadi::EntityDisplayAttribute>()->setIconName( QString() );
      col.attribute<Akonadi::EntityDisplayAttribute>()->setActiveIconName( QString() );
    }
  }

  if ( mFolderCollection ) {
    if ( mShowSenderReceiverComboBox->currentIndex() == 1 )
      mFolderCollection->setUserWhoField( "From" );
    else if ( mShowSenderReceiverComboBox->currentIndex() == 2 )
      mFolderCollection->setUserWhoField( "To" );
    else
      mFolderCollection->setUserWhoField( "" );
  }

    // message list theme
  const bool usePrivateTheme = !mUseDefaultThemeCheckBox->isChecked();
  mThemeComboBox->writeStorageModelConfig( mCurrentCollection, usePrivateTheme );
}

