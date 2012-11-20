/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 * Copyright (c) 2001 Aaron J. Seigo <aseigo@kde.org>
 * Copyright (c) 2010 Till Adam <adam@kde.org>
 * Copyright (c) 2011, 2012 Laurent Montel <montel@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "searchwindow.h"

#include "folderrequester.h"
#include "kmcommands.h"
#include "kmmainwidget.h"
#include "mailcommon/mailkernel.h"
#include "mailcommon/searchpatternedit.h"
#include "regexplineedit.h"
#include "searchdescriptionattribute.h"
#include "foldertreeview.h"
#include "kmsearchmessagemodel.h"
#include "kmsearchfilterproxymodel.h"

#include <Akonadi/AttributeFactory>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/CollectionStatisticsJob>
#include <Akonadi/EntityTreeView>
#include <Akonadi/ItemModel>
#include <Akonadi/KMime/MessageModel>
#include <akonadi/persistentsearchattribute.h>
#include <Akonadi/SearchCreateJob>
#include <Akonadi/ChangeRecorder>
#include <akonadi/standardactionmanager.h>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <KActionMenu>
#include <KDebug>
#include <KIcon>
#include <KIconLoader>
#include <KLineEdit>
#include <kmime/kmime_message.h>
#include <KPushButton>
#include <KSharedConfig>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <KStatusBar>
#include <KWindowSystem>
#include <KMessageBox>

#include <QtGui/QCheckBox>
#include <QtGui/QCloseEvent>
#include <QtGui/QCursor>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QRadioButton>
#include <QtGui/QVBoxLayout>

#include <assert.h>
#include <stdlib.h>

using namespace KPIM;
using namespace MailCommon;

namespace KMail {

SearchWindow::SearchWindow( KMMainWidget *widget, const Akonadi::Collection &collection )
  : KDialog( 0 ),
    mStopped( false ),
    mCloseRequested( false ),
    mSortColumn( 0 ),
    mSortOrder( Qt::AscendingOrder ),
    mSearchJob( 0 ),
    mTimer( new QTimer( this ) ),
    mResultModel( 0 ),
    mLastFocus( 0 ),
    mKMMainWidget( widget ),
    mAkonadiStandardAction( 0 )
{
  setCaption( i18n( "Find Messages" ) );
  setButtons( User1 | User2 | Close );
  setDefaultButton( User1 );
  setButtonGuiItem( User1, KGuiItem( i18nc( "@action:button Search for messages", "&Search" ), "edit-find" ) );
  setButtonGuiItem( User2, KStandardGuiItem::stop() );

  KWindowSystem::setIcons( winId(), qApp->windowIcon().pixmap( IconSize( KIconLoader::Desktop ),
                                                               IconSize( KIconLoader::Desktop ) ),
                           qApp->windowIcon().pixmap( IconSize( KIconLoader::Small ),
                                                      IconSize( KIconLoader::Small ) ) );

  KSharedConfig::Ptr config = KMKernel::self()->config();

  QWidget *searchWidget = new QWidget( this );
  QVBoxLayout *vbl = new QVBoxLayout( searchWidget );
  vbl->setMargin( 0 );

  QFrame *radioFrame = new QFrame( searchWidget );
  QVBoxLayout *radioLayout = new QVBoxLayout( radioFrame );
  mChkbxAllFolders = new QRadioButton( i18n( "Search in &all folders" ), searchWidget );

  QHBoxLayout *hbl = new QHBoxLayout();

  mChkbxSpecificFolders = new QRadioButton( i18n( "Search &only in:" ), searchWidget );
  mChkbxSpecificFolders->setChecked( true );

  mCbxFolders = new FolderRequester( searchWidget );
  mCbxFolders->setMustBeReadWrite( false );
  mCbxFolders->setNotAllowToCreateNewFolder( true );

  mChkSubFolders = new QCheckBox( i18n( "I&nclude sub-folders" ), searchWidget );
  mChkSubFolders->setChecked( true );

  radioLayout->addWidget( mChkbxAllFolders );
  hbl->addWidget( mChkbxSpecificFolders );
  hbl->addWidget( mCbxFolders );
  hbl->addWidget( mChkSubFolders );
  radioLayout->addLayout( hbl );

  QGroupBox *patternGroupBox = new QGroupBox( searchWidget );
  QHBoxLayout *layout = new QHBoxLayout( patternGroupBox );
  layout->setContentsMargins( 0, 0, 0, 0 );

  mPatternEdit = new SearchPatternEdit( searchWidget );
  layout->addWidget( mPatternEdit );
  patternGroupBox->setFlat( true );

  bool currentFolderIsSearchFolder = false;

  if ( !collection.hasAttribute<Akonadi::PersistentSearchAttribute>() ) {
    // it's not a search folder, make a new search
    mSearchPattern.append( SearchRule::createInstance( "Subject" ) );
    mCbxFolders->setCollection( collection );
  } else {
    // it's a search folder
    if ( collection.hasAttribute<Akonadi::SearchDescriptionAttribute>() ) {
      currentFolderIsSearchFolder = true; // FIXME is there a better way to tell?

      const Akonadi::SearchDescriptionAttribute* searchDescription = collection.attribute<Akonadi::SearchDescriptionAttribute>();
      mSearchPattern.deserialize( searchDescription->description() );

      const Akonadi::Collection col = searchDescription->baseCollection();
      if ( col.isValid() ) {
        mChkbxSpecificFolders->setChecked( true );
        mCbxFolders->setCollection( col );
        mChkSubFolders->setChecked( searchDescription->recursive() );
      } else {
        mChkbxAllFolders->setChecked( true );
      }
    } else {
      // it's a search folder, but not one of ours, warn the user that we can't edit it
      // FIXME show results, but disable edit GUI
      kWarning() << "This search was not created with KMail. It cannot be edited within it.";
      mSearchPattern.clear();
    }
  }

  mPatternEdit->setSearchPattern( &mSearchPattern );

  // enable/disable widgets depending on radio buttons:
  connect( mChkbxSpecificFolders, SIGNAL(toggled(bool)),
           mCbxFolders, SLOT(setEnabled(bool)) );
  connect( mChkbxSpecificFolders, SIGNAL(toggled(bool)),
           mChkSubFolders, SLOT(setEnabled(bool)) );
  connect( mChkbxAllFolders, SIGNAL(toggled(bool)),
           this, SLOT(setEnabledSearchButton(bool)) );

  mLbxMatches = new Akonadi::EntityTreeView( mKMMainWidget->guiClient(), this );
  mLbxMatches->setAlternatingRowColors( true );

  /*
    Default is to sort by date. TODO: Unfortunately this sorts *while*
    inserting, which looks rather strange - the user cannot read
    the results so far as they are constantly re-sorted --dnaber

    Sorting is now disabled when a search is started and reenabled
    when it stops. Items are appended to the list. This not only
    solves the above problem, but speeds searches with many hits
    up considerably. - till

    TODO: subclass QTreeWidgetItem and do proper (and performant)
    compare functions
  */
  mLbxMatches->setSortingEnabled( true );
#if 0 // port me!
  mLbxMatches->sortItems( 2, Qt::DescendingOrder );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif

  mLbxMatches->setAllColumnsShowFocus( true );
  mLbxMatches->setSelectionMode( QAbstractItemView::ExtendedSelection );
  mLbxMatches->setContextMenuPolicy( Qt::CustomContextMenu );

  connect( mLbxMatches, SIGNAL(customContextMenuRequested(QPoint)),
           this, SLOT(slotContextMenuRequested(QPoint)) );
  connect( mLbxMatches, SIGNAL(clicked(Akonadi::Item)),
           this, SLOT(slotShowMsg(Akonadi::Item)) );
  connect( mLbxMatches, SIGNAL(doubleClicked(Akonadi::Item)),
           this, SLOT(slotViewMsg(Akonadi::Item)) );
  connect( mLbxMatches, SIGNAL(currentChanged(Akonadi::Item)),
           this, SLOT(slotCurrentChanged(Akonadi::Item)) );

  connect( KMKernel::self()->folderCollectionMonitor(), SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)), this, SLOT(updateCollectionStatistic(Akonadi::Collection::Id,Akonadi::CollectionStatistics)) );

  QHBoxLayout *hbl2 = new QHBoxLayout();
  mSearchFolderLbl = new QLabel( i18n( "Search folder &name:" ), searchWidget );
  mSearchFolderEdt = new KLineEdit( searchWidget );
  mSearchFolderEdt->setClearButtonShown( true );

  if ( currentFolderIsSearchFolder ) {
    mFolder = collection;
    mSearchFolderEdt->setText( collection.name() );
    Q_ASSERT ( !mResultModel );
    createSearchModel();
  } else {
    mSearchFolderEdt->setText( i18n( "Last Search" ) );
    // find last search and reuse it if possible
    mFolder = CommonKernel->collectionFromId( GlobalSettings::lastSearchCollectionId() );
    // when the last folder got renamed, create a new one
    if ( mFolder.isValid() && mFolder.name() != mSearchFolderEdt->text() ) {
      mFolder = Akonadi::Collection();
    }
  }

  mSearchFolderLbl->setBuddy( mSearchFolderEdt );
  mSearchFolderOpenBtn = new KPushButton( i18n( "Op&en Search Folder" ), searchWidget );
  mSearchFolderOpenBtn->setEnabled( false );
  connect( mSearchFolderEdt, SIGNAL(textChanged(QString)),
           this, SLOT(scheduleRename(QString)) );
  connect( &mRenameTimer, SIGNAL(timeout()),
           this, SLOT(renameSearchFolder()) );
  connect( mSearchFolderOpenBtn, SIGNAL(clicked()),
           this, SLOT(openSearchFolder()) );

  mSearchResultOpenBtn = new KPushButton( i18n( "Open &Message" ), searchWidget );
  mSearchResultOpenBtn->setEnabled( false );
  connect( mSearchResultOpenBtn, SIGNAL(clicked()),
           this, SLOT(slotViewSelectedMsg()) );

  hbl2->addWidget( mSearchFolderLbl );
  hbl2->addWidget( mSearchFolderEdt );
  hbl2->addWidget( mSearchFolderOpenBtn );
  hbl2->addWidget( mSearchResultOpenBtn );

  mStatusBar = new KStatusBar(searchWidget);
  mStatusBar->insertPermanentItem( i18n( "AMiddleLengthText..." ), 0 );
  mStatusBar->changeItem( i18nc( "@info:status finished searching.", "Ready." ), 0 );
  mStatusBar->setItemAlignment( 0, Qt::AlignLeft | Qt::AlignVCenter );
  mStatusBar->insertPermanentItem( QString(), 1, 1 );
  mStatusBar->setItemAlignment( 1, Qt::AlignLeft | Qt::AlignVCenter );

  const int mainWidth = GlobalSettings::self()->searchWidgetWidth();
  const int mainHeight = GlobalSettings::self()->searchWidgetHeight();

  if ( mainWidth || mainHeight )
    resize( mainWidth, mainHeight );

  setMainWidget( searchWidget );
  setButtonsOrientation( Qt::Vertical );
  enableButton( User2, false );

  //Bring all the layouts together
  vbl->addWidget( radioFrame );
  vbl->addWidget( patternGroupBox );
  vbl->addWidget( mLbxMatches );
  vbl->addLayout( hbl2 );
  vbl->addWidget( mStatusBar );

  patternGroupBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );

  connect( this, SIGNAL(user1Clicked()), SLOT(slotSearch()) );
  connect( this, SIGNAL(user2Clicked()), SLOT(slotStop()) );
  connect( this, SIGNAL(finished()), this, SLOT(deleteLater()) );
  connect( this, SIGNAL(closeClicked()),this,SLOT(slotClose()) );

  // give focus to the value field of the first search rule
  RegExpLineEdit* r = mPatternEdit->findChild<RegExpLineEdit*>( "regExpLineEdit" );
  if ( r )
    r->setFocus();
  else
    kDebug() << "SearchWindow: regExpLineEdit not found";

  //set up actions
  KActionCollection *ac = actionCollection();
  mReplyAction = new KAction( KIcon( "mail-reply-sender" ), i18n( "&Reply..." ), this );
  actionCollection()->addAction( "search_reply", mReplyAction );
  connect( mReplyAction, SIGNAL(triggered(bool)), SLOT(slotReplyToMsg()) );

  mReplyAllAction = new KAction( KIcon( "mail-reply-all" ), i18n( "Reply to &All..." ), this );
  actionCollection()->addAction( "search_reply_all", mReplyAllAction );
  connect( mReplyAllAction, SIGNAL(triggered(bool)), SLOT(slotReplyAllToMsg()) );

  mReplyListAction = new KAction( KIcon( "mail-reply-list" ), i18n( "Reply to Mailing-&List..." ), this );
  actionCollection()->addAction( "search_reply_list", mReplyListAction );
  connect( mReplyListAction, SIGNAL(triggered(bool)), SLOT(slotReplyListToMsg()) );

  mForwardActionMenu = new KActionMenu( KIcon( "mail-forward" ), i18nc( "Message->", "&Forward" ), this );
  actionCollection()->addAction( "search_message_forward", mForwardActionMenu );
  connect( mForwardActionMenu, SIGNAL(triggered(bool)), this, SLOT(slotForwardMsg()) );

  mForwardInlineAction = new KAction( KIcon( "mail-forward" ),
                                      i18nc( "@action:inmenu Forward message inline.", "&Inline..." ),
                                      this );
  actionCollection()->addAction( "search_message_forward_inline", mForwardInlineAction );
  connect( mForwardInlineAction, SIGNAL(triggered(bool)), SLOT(slotForwardMsg()) );

  mForwardAttachedAction = new KAction( KIcon( "mail-forward" ), i18nc( "Message->Forward->", "As &Attachment..." ), this );
  actionCollection()->addAction( "search_message_forward_as_attachment", mForwardAttachedAction );
  connect( mForwardAttachedAction, SIGNAL(triggered(bool)), SLOT(slotForwardAttachedMsg()) );

  if ( GlobalSettings::self()->forwardingInlineByDefault() ) {
    mForwardActionMenu->addAction( mForwardInlineAction );
    mForwardActionMenu->addAction( mForwardAttachedAction );
  } else {
    mForwardActionMenu->addAction( mForwardAttachedAction );
    mForwardActionMenu->addAction( mForwardInlineAction );
  }

  mSaveAsAction = actionCollection()->addAction( KStandardAction::SaveAs, "search_file_save_as", this, SLOT(slotSaveMsg()) );

  mSaveAtchAction = new KAction( KIcon( "mail-attachment" ), i18n( "Save Attachments..." ), this );
  actionCollection()->addAction( "search_save_attachments", mSaveAtchAction );
  connect( mSaveAtchAction, SIGNAL(triggered(bool)), SLOT(slotSaveAttachments()) );

  mPrintAction = actionCollection()->addAction( KStandardAction::Print, "search_print", this, SLOT(slotPrintMsg()) );

  mClearAction = new KAction( i18n( "Clear Selection" ), this );
  actionCollection()->addAction( "search_clear_selection", mClearAction );
  connect( mClearAction, SIGNAL(triggered(bool)), SLOT(slotClearSelection()) );

  connect( mCbxFolders, SIGNAL(folderChanged(Akonadi::Collection)),
           this, SLOT(slotFolderActivated()) );

  ac->addAssociatedWidget( this );
  foreach ( QAction* action, ac->actions() )
    action->setShortcutContext( Qt::WidgetWithChildrenShortcut );
}

SearchWindow::~SearchWindow()
{
  if ( mResultModel ) {
    if ( mLbxMatches->columnWidth( 0 ) > 0 ) {
      GlobalSettings::self()->setCollectionWidth( mLbxMatches->columnWidth( 0 )  );
    }
    if ( mLbxMatches->columnWidth( 1 ) > 0 ) {
      GlobalSettings::self()->setSubjectWidth( mLbxMatches->columnWidth( 1 )  );
    }
    if ( mLbxMatches->columnWidth( 2 ) > 0 ) {
      GlobalSettings::self()->setSenderWidth( mLbxMatches->columnWidth( 2 ) );
    }
    if ( mLbxMatches->columnWidth( 3 ) > 0 ) {
      GlobalSettings::self()->setReceiverWidth( mLbxMatches->columnWidth( 3 ) );
    }
    if ( mLbxMatches->columnWidth( 4 ) > 0 ) {
      GlobalSettings::self()->setDateWidth( mLbxMatches->columnWidth( 4 ) );
    }
    if ( mLbxMatches->columnWidth( 5 ) > 0 ) {
      GlobalSettings::self()->setFolderWidth( mLbxMatches->columnWidth( 5 ) );
    }
    GlobalSettings::self()->setSearchWidgetWidth( width() );
    GlobalSettings::self()->setSearchWidgetHeight( height() );
    GlobalSettings::self()->requestSync();
    mResultModel->deleteLater();
  }
}

void SearchWindow::createSearchModel()
{
  if ( mResultModel ) {
    mResultModel->deleteLater();
  }
  mResultModel = new KMSearchMessageModel( this );
  mResultModel->setCollection( mFolder );
  KMSearchFilterProxyModel *sortproxy = new KMSearchFilterProxyModel( mResultModel );
  sortproxy->setSourceModel( mResultModel );
  mLbxMatches->setModel( sortproxy );

  mLbxMatches->setColumnWidth( 0, GlobalSettings::self()->collectionWidth() );
  mLbxMatches->setColumnWidth( 1, GlobalSettings::self()->subjectWidth() );
  mLbxMatches->setColumnWidth( 2, GlobalSettings::self()->senderWidth() );
  mLbxMatches->setColumnWidth( 3, GlobalSettings::self()->receiverWidth() );
  mLbxMatches->setColumnWidth( 4, GlobalSettings::self()->dateWidth() );
  mLbxMatches->setColumnWidth( 5, GlobalSettings::self()->folderWidth() );
  mLbxMatches->setColumnHidden( 6, true );
  mLbxMatches->header()->setSortIndicator( 2, Qt::DescendingOrder );
  mLbxMatches->header()->setStretchLastSection( false );
  mLbxMatches->header()->restoreState( mHeaderState );
  //mLbxMatches->header()->setResizeMode( 3, QHeaderView::Stretch );
  if(!mAkonadiStandardAction)
      mAkonadiStandardAction = new Akonadi::StandardMailActionManager( actionCollection(), this );
  mAkonadiStandardAction->setItemSelectionModel( mLbxMatches->selectionModel() );
  mAkonadiStandardAction->setCollectionSelectionModel( mKMMainWidget->folderTreeView()->selectionModel() );

}


void SearchWindow::setEnabledSearchButton( bool )
{
  //Make sure that button is enable
  //Before when we selected a folder == "Local Folder" as that it was not a folder
  //search button was disable, and when we select "Search in all local folder"
  //Search button was never enabled :(
  enableButton( User1, true );
}

void SearchWindow::updateCollectionStatistic(Akonadi::Collection::Id id,Akonadi::CollectionStatistics statistic)
{
  QString genMsg;
  if ( id == mFolder.id() ) {
    genMsg = i18np( "%1 match", "%1 matches", statistic.count() );
  }
  mStatusBar->changeItem( genMsg, 0 );
}

void SearchWindow::keyPressEvent( QKeyEvent *event )
{
  if ( event->key() == Qt::Key_Escape && mSearchJob ) {
    slotStop();
    return;
  }

  KDialog::keyPressEvent( event );
}

void SearchWindow::slotFolderActivated()
{
  mChkbxSpecificFolders->setChecked( true );
}

void SearchWindow::activateFolder( const Akonadi::Collection &collection )
{
  mChkbxSpecificFolders->setChecked( true );
  mCbxFolders->setCollection( collection );
}

void SearchWindow::slotSearch()
{
  mLastFocus = focusWidget();
  setButtonFocus( User1 );     // set focus so we don't miss key event

  mStopped = false;

  if ( mSearchFolderEdt->text().isEmpty() ) {
    mSearchFolderEdt->setText( i18n( "Last Search" ) );
  }
  enableButton( User1, false );
  enableButton( User2, true );
  if ( mResultModel )
    mHeaderState = mLbxMatches->header()->saveState();

  mLbxMatches->setModel( 0 );

  mSortColumn = mLbxMatches->header()->sortIndicatorSection();
  mSortOrder = mLbxMatches->header()->sortIndicatorOrder();
  mLbxMatches->setSortingEnabled( false );

  if ( mSearchJob ) {
    mSearchJob->kill( KJob::Quietly );
    mSearchJob->deleteLater();
    mSearchJob = 0;
  }

  mSearchFolderEdt->setEnabled( false );

  KUrl::List urls;
  if ( !mChkbxAllFolders->isChecked() ) {
    const Akonadi::Collection col = mCbxFolders->collection();
    urls<< col.url( Akonadi::Collection::UrlShort );
    if ( mChkSubFolders->isChecked() ) {
      childCollectionsFromSelectedCollection( col, urls );
    }
  }

  mPatternEdit->updateSearchPattern();

  SearchPattern searchPattern( mSearchPattern );
  searchPattern.purify();
  enableGUI();

  mTimer->start( 200 );


#ifdef AKONADI_USE_STRIGI_SEARCH
  const QString query = searchPattern.asXesamQuery();
  const QString queryLanguage = "XESAM";
#else
  const QString query = searchPattern.asSparqlQuery(urls);
  const QString queryLanguage = "SPARQL";
#endif

  qDebug() << queryLanguage;
  qDebug() << query;
  if ( query.isEmpty() )
    return;
  mSearchFolderOpenBtn->setEnabled( true );

  if ( !mFolder.isValid() ) {
    // FIXME if another app created a virtual 'Last Search' folder without
    // out custom attributes it will result in problems
    mSearchJob = new Akonadi::SearchCreateJob( mSearchFolderEdt->text(), query, this );
  } else {
    Akonadi::PersistentSearchAttribute *attribute = mFolder.attribute<Akonadi::PersistentSearchAttribute>();
    attribute->setQueryLanguage( queryLanguage );
    attribute->setQueryString( query );
    mSearchJob = new Akonadi::CollectionModifyJob( mFolder, this );
  }

  connect( mSearchJob, SIGNAL(result(KJob*)), SLOT(searchDone(KJob*)) );
}

void SearchWindow::searchDone( KJob* job )
{
  Q_ASSERT( job == mSearchJob );
  if ( job->error() ) {
    KMessageBox::sorry( this, i18n("Can not get search result. %1", job->errorString() ) );
    if ( mSearchJob ) {
      mSearchJob = 0;
    }
  }
  else
  {

    if ( Akonadi::SearchCreateJob *searchJob = qobject_cast<Akonadi::SearchCreateJob*>( mSearchJob ) ) {
      mFolder = searchJob->createdCollection();
    } else if ( Akonadi::CollectionModifyJob *modifyJob = qobject_cast<Akonadi::CollectionModifyJob*>( mSearchJob ) ) {
      mFolder = modifyJob->collection();
    }
    /// TODO: cope better with cases where this fails
      Q_ASSERT( mFolder.isValid() );
      Q_ASSERT( mFolder.hasAttribute<Akonadi::PersistentSearchAttribute>() );

      GlobalSettings::setLastSearchCollectionId( mFolder.id() );
      GlobalSettings::self()->writeConfig();
      GlobalSettings::self()->requestSync();

      // store the kmail specific serialization of the search in an attribute on
      // the server, for easy retrieval when editing it again
      const QByteArray search = mSearchPattern.serialize();
      Q_ASSERT( !search.isEmpty() );
      Akonadi::SearchDescriptionAttribute *searchDescription = mFolder.attribute<Akonadi::SearchDescriptionAttribute>( Akonadi::Entity::AddIfMissing );
      searchDescription->setDescription( search );
      if ( !mChkbxAllFolders->isChecked() ) {
        const Akonadi::Collection collection = mCbxFolders->collection();
        searchDescription->setBaseCollection( collection );
      } else {
        searchDescription->setBaseCollection( Akonadi::Collection() );
      }
      searchDescription->setRecursive( mChkSubFolders->isChecked() );
      new Akonadi::CollectionModifyJob( mFolder, this );
      mSearchJob = 0;

      createSearchModel();

      mTimer->stop();

      QTimer::singleShot( 0, this, SLOT(enableGUI()) );

      if ( mLastFocus )
        mLastFocus->setFocus();

      if ( mCloseRequested )
        close();

      mLbxMatches->setSortingEnabled( true );
      mLbxMatches->header()->setSortIndicator( mSortColumn, mSortOrder );

      mSearchFolderEdt->setEnabled( true );
  }
}

void SearchWindow::slotStop()
{
  if ( mSearchJob ) {
    mSearchJob->kill( KJob::Quietly );
    mSearchJob->deleteLater();
    mSearchJob = 0;
  }

  mStopped = true;
  enableButton( User2, false );
}

void SearchWindow::slotClose()
{
  accept();
}

void SearchWindow::closeEvent( QCloseEvent *event )
{
  if ( mSearchJob ) {
    mCloseRequested = true;
    //Cancel search in progress
    mSearchJob->kill( KJob::Quietly );
    mSearchJob->deleteLater();
    mSearchJob = 0;
    QTimer::singleShot( 0, this, SLOT(slotClose()) );
  } else {
    KDialog::closeEvent( event );
  }
}

void SearchWindow::scheduleRename( const QString &text )
{
  if ( !text.isEmpty() ) {
    mRenameTimer.setSingleShot( true );
    mRenameTimer.start( 250 );
    mSearchFolderOpenBtn->setEnabled( false );
  } else {
    mRenameTimer.stop();
    mSearchFolderOpenBtn->setEnabled( !text.isEmpty() );
  }
}

void SearchWindow::renameSearchFolder()
{
  const QString name = mSearchFolderEdt->text();
  if ( mFolder.isValid() ) {
    if ( mFolder.name() != name ) {
      mFolder.setName( name );
      Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( mFolder, this );
      connect( job, SIGNAL(result(KJob*)),
               this, SLOT(slotSearchFolderRenameDone(KJob*)) );
    }
    mSearchFolderOpenBtn->setEnabled( true );
  }
}

void SearchWindow::slotSearchFolderRenameDone( KJob *job )
{
  Q_ASSERT( job );
  if ( job->error() ) {
    kWarning() << "Job failed:" << job->errorText();
    KMessageBox::information( this, i18n( "There was a problem renaming your search folder. "
                                          "A common reason for this is that another search folder "
                                          "with the same name already exists." ) );
  }
}

void SearchWindow::openSearchFolder()
{
  Q_ASSERT( mFolder.isValid() );
  renameSearchFolder();
  mKMMainWidget->slotSelectCollectionFolder( mFolder );
  slotClose();
}

bool SearchWindow::slotShowMsg( const Akonadi::Item &item )
{
  if ( item.isValid() ) {
    mKMMainWidget->slotMessageSelected( item );
    return true;
  }

  return false;
}

void SearchWindow::slotViewSelectedMsg()
{
  mKMMainWidget->slotMessageActivated( selectedMessage() );
}

bool SearchWindow::slotViewMsg( const Akonadi::Item &item )
{
  if ( item.isValid() ) {
    mKMMainWidget->slotMessageActivated( item );
    return true;
  }

  return false;
}

void SearchWindow::slotCurrentChanged( const Akonadi::Item &item )
{
  mSearchResultOpenBtn->setEnabled( item.isValid() );
}

void SearchWindow::enableGUI()
{
  const bool searching = (mSearchJob != 0);

  enableButton( KDialog::Close, !searching );

  mCbxFolders->setEnabled( !searching && !mChkbxAllFolders->isChecked() );
  mChkSubFolders->setEnabled( !searching && !mChkbxAllFolders->isChecked() );
  mChkbxAllFolders->setEnabled( !searching );
  mChkbxSpecificFolders->setEnabled( !searching );
  mPatternEdit->setEnabled( !searching);

  enableButton( User1, !searching );
  enableButton( User2, searching );
}

Akonadi::Item::List SearchWindow::selectedMessages() const
{
  Akonadi::Item::List messages;

  foreach ( const QModelIndex &index, mLbxMatches->selectionModel()->selectedRows() ) {
    const Akonadi::Item item = index.data( Akonadi::ItemModel::ItemRole ).value<Akonadi::Item>();
    if ( item.isValid() )
      messages.append( item );
  }

  return messages;
}

Akonadi::Item SearchWindow::selectedMessage() const
{
  return mLbxMatches->currentIndex().data( Akonadi::ItemModel::ItemRole ).value<Akonadi::Item>();
}

void SearchWindow::updateContextMenuActions()
{
  const int count = selectedMessages().count();
  const bool singleActions = (count == 1);

  mReplyAction->setEnabled( singleActions );
  mReplyAllAction->setEnabled( singleActions );
  mReplyListAction->setEnabled( singleActions );
  mPrintAction->setEnabled( singleActions );
  mSaveAtchAction->setEnabled( count > 0 );
  mSaveAsAction->setEnabled( count > 0 );
  mClearAction->setEnabled( count > 0 );
}

void SearchWindow::slotContextMenuRequested( const QPoint& )
{
  if ( !selectedMessage().isValid() || selectedMessages().isEmpty() )
    return;

  QMenu *menu = new QMenu( this );
  updateContextMenuActions();

  // show most used actions
  menu->addAction( mReplyAction );
  menu->addAction( mReplyAllAction );
  menu->addAction( mReplyListAction );
  menu->addAction( mForwardActionMenu );
  menu->addSeparator();
  KAction *act = mAkonadiStandardAction->createAction( Akonadi::StandardActionManager::CopyItems );
  mAkonadiStandardAction->setActionText( Akonadi::StandardActionManager::CopyItems, ki18np( "Copy Message", "Copy %1 Messages" ) );
  menu->addAction( act );
  act = mAkonadiStandardAction->createAction( Akonadi::StandardActionManager::CutItems );
  mAkonadiStandardAction->setActionText( Akonadi::StandardActionManager::CutItems, ki18np( "Cut Message", "Cut %1 Messages" ) );
  menu->addAction( act );
  menu->addAction( mAkonadiStandardAction->createAction( Akonadi::StandardActionManager::CopyItemToMenu ) );
  menu->addAction( mAkonadiStandardAction->createAction( Akonadi::StandardActionManager::MoveItemToMenu ) );
  menu->addSeparator();
  menu->addAction( mSaveAsAction );
  menu->addAction( mSaveAtchAction );
  menu->addAction( mPrintAction );
  menu->addSeparator();
  menu->addAction( mClearAction );
  menu->exec( QCursor::pos(), 0 );

  delete menu;
}

void SearchWindow::slotClearSelection()
{
  mLbxMatches->clearSelection();
}

void SearchWindow::slotReplyToMsg()
{
  KMCommand *command = new KMReplyCommand( this, selectedMessage(), MessageComposer::ReplySmart );
  command->start();
}

void SearchWindow::slotReplyAllToMsg()
{
  KMCommand *command = new KMReplyCommand( this, selectedMessage(),MessageComposer::ReplyAll );
  command->start();
}

void SearchWindow::slotReplyListToMsg()
{
  KMCommand *command = new KMReplyCommand( this, selectedMessage(),MessageComposer::ReplyList );
  command->start();
}

void SearchWindow::slotForwardMsg()
{
  KMCommand *command = new KMForwardCommand( this, selectedMessages() );
  command->start();
}

void SearchWindow::slotForwardAttachedMsg()
{
  KMCommand *command = new KMForwardAttachedCommand( this, selectedMessages() );
  command->start();
}

void SearchWindow::slotSaveMsg()
{
  KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand( this, selectedMessages() );
  saveCommand->start();
}

void SearchWindow::slotSaveAttachments()
{
  KMSaveAttachmentsCommand *saveCommand = new KMSaveAttachmentsCommand( this, selectedMessages() );
  saveCommand->start();
}

void SearchWindow::slotPrintMsg()
{
  KMCommand *command = new KMPrintCommand( this, selectedMessage() );
  command->start();
}

void SearchWindow::addRulesToSearchPattern( const SearchPattern &pattern )
{
  SearchPattern p( mSearchPattern );
  p.purify();

  QList<SearchRule::Ptr>::const_iterator it;
  QList<SearchRule::Ptr>::const_iterator end(pattern.constEnd() ) ;

  for ( it = pattern.constBegin() ; it != end ; ++it ) {
    p.append( SearchRule::createInstance( **it ) );
  }

  mSearchPattern = p;
  mPatternEdit->setSearchPattern( &mSearchPattern );
}

void SearchWindow::setSearchPattern( const SearchPattern &pattern )
{
  mSearchPattern = pattern;
  mPatternEdit->setSearchPattern( &mSearchPattern );
}


void SearchWindow::childCollectionsFromSelectedCollection( const Akonadi::Collection& collection, KUrl::List&lstUrlCollection )
{  
  if ( collection.isValid() )  {
    QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection( KMKernel::self()->collectionModel(), collection );
    if ( idx.isValid() ) {
      getChildren( KMKernel::self()->collectionModel(), idx, lstUrlCollection );
    }
  }
}

void SearchWindow::getChildren( const QAbstractItemModel *model,
                                const QModelIndex &parentIndex,
                                KUrl::List &list )
{
  const int rowCount = model->rowCount( parentIndex );
  for ( int row = 0; row < rowCount; ++row ) {
    const QModelIndex index = model->index( row, 0, parentIndex );
    if ( model->rowCount( index ) > 0 ) {
      
      getChildren( model, index, list );
    }
    Akonadi::Collection c = model->data(index, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
    if ( c.isValid() )
      list << c.url( Akonadi::Collection::UrlShort );
  }
}


}

#include "searchwindow.moc"
