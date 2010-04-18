/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 * Copyright (c) 2001 Aaron J. Seigo <aseigo@kde.org>
 * Copyright (c) 2010 Till Adam <adam@kde.org>
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

#include <QCheckBox>
#include <QCloseEvent>
#include <QCursor>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QRadioButton>
#include <QVBoxLayout>

#include <KActionMenu>
#include <KConfigGroup>
#include <KDebug>
#include <KIcon>
#include <KIconLoader>
#include <KLineEdit>
#include <KPushButton>
#include <KSharedConfig>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <KStatusBar>
#include <KWindowSystem>
#include <KMessageBox>

#include "folderrequester.h"
#include "kmcommands.h"
#include "kmmainwidget.h"
#include "kmsearchpatternedit.h"
#include "kmsearchpattern.h"
#include "searchdescriptionattribute.h"

#include "regexplineedit.h"

using namespace KPIM;

#include <Akonadi/AttributeFactory>
#include <Akonadi/EntityTreeView>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/SearchCreateJob>
#include <Akonadi/KMime/MessageModel>
#include <akonadi/persistentsearchattribute.h>

#include <kmime/kmime_message.h>

#include <assert.h>
#include <stdlib.h>

namespace KMail {

const int SearchWindow::MSGID_COLUMN = 4;

#if 0 //TODO port to akonadi
void MatchListView::contextMenuEvent( QContextMenuEvent* event )
{
  emit contextMenuRequested( itemAt( event->pos() ) );
}
#endif

//-----------------------------------------------------------------------------
SearchWindow::SearchWindow(KMMainWidget* w, const Akonadi::Collection& curFolder):
  KDialog(0),
  mStopped(false),
  mCloseRequested(false),
  mSortColumn(0),
  mSortOrder(Qt::AscendingOrder),
  mSearchJob( 0 ),
  mTimer(new QTimer(this)),
  mResultModel( 0 ),
  mLastFocus(0),
  mKMMainWidget(w)
{
  setCaption( i18n("Find Messages") );
  setButtons( User1 | User2 | Close );
  setDefaultButton( User1 );
  setButtonGuiItem( User1, KGuiItem( i18nc("@action:button Search for messags", "&Search"), "edit-find" ) );
  setButtonGuiItem( User2, KStandardGuiItem::stop() );
  KWindowSystem::setIcons( winId(), qApp->windowIcon().pixmap(
                               IconSize( KIconLoader::Desktop ),
                               IconSize( KIconLoader::Desktop ) ),
                           qApp->windowIcon().pixmap(
                               IconSize( KIconLoader::Small ),
                               IconSize( KIconLoader::Small ) ) );

  KSharedConfig::Ptr config = KMKernel::config();
  KConfigGroup group( config, "SearchDialog" );

  QWidget* searchWidget = new QWidget(this);
  QVBoxLayout *vbl = new QVBoxLayout( searchWidget );
  vbl->setObjectName( "kmfs_vbl" );
  vbl->setMargin( 0 );

  QFrame * radioFrame = new QFrame( searchWidget );
  QVBoxLayout * radioLayout = new QVBoxLayout( radioFrame );
  mChkbxAllFolders = new QRadioButton(i18n("Search in &all local folders"), searchWidget );

  QHBoxLayout *hbl = new QHBoxLayout();
  hbl->setObjectName( "kmfs_hbl" );

  mChkbxSpecificFolders = new QRadioButton( i18n("Search &only in:"), searchWidget );
  mChkbxSpecificFolders->setChecked(true);

  mCbxFolders = new FolderRequester( searchWidget );
  mCbxFolders->setMustBeReadWrite( false );

  mCbxFolders->setFolder(curFolder);

  mChkSubFolders = new QCheckBox(i18n("I&nclude sub-folders"), searchWidget);
  mChkSubFolders->setChecked(true);

  radioLayout->addWidget( mChkbxAllFolders );
  hbl->addWidget(mChkbxSpecificFolders);
  hbl->addWidget(mCbxFolders);
  hbl->addWidget(mChkSubFolders);
  radioLayout->addLayout( hbl );


  mPatternEdit = new KMSearchPatternEdit( QString(), searchWidget, false, true );
  mPatternEdit->setFlat( true );

#if 0 //TODO re-enable for legacy importing?
  KMFolderSearch *searchFolder = 0;
  if (curFolder)
      searchFolder = dynamic_cast<KMFolderSearch*>(curFolder->storage());
  if (searchFolder) {
      KConfig config(curFolder->location());
      KMFolder *root = searchFolder->search()->root();
      KConfigGroup group( &config, "Search Folder" );
      mSearchPattern->readConfig( group );
      if (root) {
          mChkbxSpecificFolders->setChecked(true);
          mCbxFolders->setFolder(root);
          mChkSubFolders->setChecked(searchFolder->search()->recursive());
      } else {
          mChkbxAllFolders->setChecked(true);
      }
  }

#endif

  bool currentFolderIsSearchFolder = false;
    
  if ( curFolder.hasAttribute<Akonadi::SearchDescriptionAttribute>() ) {
    currentFolderIsSearchFolder = true; // FIXME is there a better way to tell?
    const Akonadi::SearchDescriptionAttribute* searchDescription = curFolder.attribute<Akonadi::SearchDescriptionAttribute>();
    mSearchPattern.deserialize( searchDescription->description() );
    const Akonadi::Collection col = searchDescription->baseCollection();
    if ( col.isValid() ) {
      mChkbxSpecificFolders->setChecked( true );
      mCbxFolders->setFolder( col );
      mChkSubFolders->setChecked( searchDescription->recursive() );
    } else {
      mChkbxAllFolders->setChecked( true );
    }
  } else {
    mSearchPattern.append( KMSearchRule::createInstance( "Subject" ) );
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
  mLbxMatches->setObjectName( "Find Messages" );
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
     comapare functions
  */
  mLbxMatches->setSortingEnabled( true );
#if 0 // port me!
  mLbxMatches->sortItems( 2, Qt::DescendingOrder );
  mLbxMatches->header()->setSortIndicator( 2, Qt::DescendingOrder );
#endif
  mLbxMatches->setAllColumnsShowFocus( true );
  mLbxMatches->setSelectionMode( QAbstractItemView::ExtendedSelection );

#if 0 // port me!
  mLbxMatches->header()->setStretchLastSection( false );
  mLbxMatches->header()->setResizeMode( 3, QHeaderView::Stretch );

  connect( mLbxMatches, SIGNAL( contextMenuRequested( QTreeWidgetItem*) ),
           this, SLOT( slotContextMenuRequested( QTreeWidgetItem* ) ) );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  connect( mLbxMatches, SIGNAL(clicked(Akonadi::Item)), SLOT(slotShowMsg(Akonadi::Item)) );
  connect( mLbxMatches, SIGNAL(doubleClicked(Akonadi::Item)), SLOT(slotViewMsg(Akonadi::Item)) );
  connect( mLbxMatches, SIGNAL(currentChanged(Akonadi::Item)), SLOT(slotCurrentChanged(Akonadi::Item)) );


  QHBoxLayout *hbl2 = new QHBoxLayout();
  hbl2->setObjectName( "kmfs_hbl2" );
  mSearchFolderLbl = new QLabel( i18n("Search folder &name:"), searchWidget );
  mSearchFolderEdt = new KLineEdit( searchWidget );
  mSearchFolderEdt->setClearButtonShown( true );

  if ( currentFolderIsSearchFolder ) {
    mFolder = curFolder;
    mSearchFolderEdt->setText( curFolder.name() );
  } else {
    mSearchFolderEdt->setText( i18n("Last Search") );
    // TODO find last search and set mFolder to it
  }
  mSearchFolderLbl->setBuddy(mSearchFolderEdt);
  mSearchFolderOpenBtn = new KPushButton(i18n("Op&en Search Folder"), searchWidget);
  mSearchFolderOpenBtn->setEnabled(false);
  connect( mSearchFolderEdt, SIGNAL( textChanged( const QString &)),
           this, SLOT( scheduleRename( const QString & )));
  connect( &mRenameTimer, SIGNAL( timeout() ),
           this, SLOT( renameSearchFolder() ));
  connect( mSearchFolderOpenBtn, SIGNAL( clicked() ),
           this, SLOT( openSearchFolder() ));

  mSearchResultOpenBtn = new KPushButton( i18n("Open &Message"), searchWidget );
  mSearchResultOpenBtn->setEnabled( false );
  connect( mSearchResultOpenBtn, SIGNAL( clicked() ),
           this, SLOT( slotViewSelectedMsg() ) );

  hbl2->addWidget(mSearchFolderLbl);
  hbl2->addWidget(mSearchFolderEdt);
  hbl2->addWidget(mSearchFolderOpenBtn);
  hbl2->addWidget(mSearchResultOpenBtn);

  mStatusBar = new KStatusBar(searchWidget);
  mStatusBar->insertPermanentItem(i18n("AMiddleLengthText..."), 0);
  mStatusBar->changeItem(i18nc("@info:status finished searching.", "Ready."), 0);
  mStatusBar->setItemAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
  mStatusBar->insertPermanentItem(QString(), 1, 1);
  mStatusBar->setItemAlignment(1, Qt::AlignLeft | Qt::AlignVCenter);

  int mainWidth = group.readEntry("SearchWidgetWidth", 0 );
  int mainHeight = group.readEntry("SearchWidgetHeight", 0 );

  if (mainWidth || mainHeight)
    resize(mainWidth, mainHeight);

  setMainWidget(searchWidget);
  setButtonsOrientation(Qt::Vertical);
  enableButton(User2, false);

  //Bring all the layouts together
  vbl->addWidget( radioFrame );
  vbl->addWidget( mPatternEdit );
  vbl->addWidget( mLbxMatches );
  vbl->addLayout( hbl2 );
  vbl->addWidget( mStatusBar );

  connect(this, SIGNAL(user1Clicked()), SLOT(slotSearch()));
  connect(this, SIGNAL(user2Clicked()), SLOT(slotStop()));
  connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
  connect(this,SIGNAL(closeClicked()),this,SLOT(slotClose()));
  // give focus to the value field of the first search rule
  RegExpLineEdit* r = mPatternEdit->findChild<RegExpLineEdit*>( "regExpLineEdit" );
  if ( r )
      r->setFocus();
  else
      kDebug() << "SearchWindow: regExpLineEdit not found";

  //set up actions
  KActionCollection *ac = actionCollection();
  mReplyAction  = new KAction(KIcon("mail-reply-sender"), i18n("&Reply..."), this);
  actionCollection()->addAction("search_reply", mReplyAction );
  connect(mReplyAction, SIGNAL(triggered(bool)), SLOT(slotReplyToMsg()));
  mReplyAllAction  = new KAction(KIcon("mail-reply-all"), i18n("Reply to &All..."), this);
  actionCollection()->addAction("search_reply_all", mReplyAllAction );
  connect(mReplyAllAction, SIGNAL(triggered(bool) ), SLOT(slotReplyAllToMsg()));
  mReplyListAction  = new KAction(KIcon("mail-reply-list"), i18n("Reply to Mailing-&List..."), this);
  actionCollection()->addAction("search_reply_list", mReplyListAction );
  connect(mReplyListAction, SIGNAL(triggered(bool) ), SLOT(slotReplyListToMsg()));

  mForwardActionMenu = new KActionMenu(KIcon("mail-forward"), i18nc("Message->","&Forward"), this);
  actionCollection()->addAction("search_message_forward", mForwardActionMenu );
  connect( mForwardActionMenu, SIGNAL(triggered(bool)), this,
           SLOT(slotForwardMsg()) );

  mForwardInlineAction = new KAction( KIcon( "mail-forward" ),
                                      i18nc( "@action:inmenu Forward message inline.",
                                             "&Inline..." ),
                                      this );
  actionCollection()->addAction("search_message_forward_inline", mForwardInlineAction );
  connect(mForwardInlineAction, SIGNAL(triggered(bool) ), SLOT(slotForwardMsg()));

  mForwardAttachedAction = new KAction(KIcon("mail-forward"), i18nc("Message->Forward->","As &Attachment..."), this);
  actionCollection()->addAction("search_message_forward_as_attachment", mForwardAttachedAction );
  connect(mForwardAttachedAction, SIGNAL(triggered(bool)), SLOT(slotForwardAttachedMsg()));

  if ( GlobalSettings::self()->forwardingInlineByDefault() ) {
    mForwardActionMenu->addAction( mForwardInlineAction );
    mForwardActionMenu->addAction( mForwardAttachedAction );
  }
  else {
    mForwardActionMenu->addAction( mForwardAttachedAction );
    mForwardActionMenu->addAction( mForwardInlineAction );
  }

  mSaveAsAction = actionCollection()->addAction(KStandardAction::SaveAs,  "search_file_save_as", this, SLOT(slotSaveMsg()));
  mSaveAtchAction  = new KAction(KIcon("mail-attachment"), i18n("Save Attachments..."), this);
  actionCollection()->addAction("search_save_attachments", mSaveAtchAction );
  connect(mSaveAtchAction, SIGNAL(triggered(bool)), SLOT(slotSaveAttachments()));

  mPrintAction = actionCollection()->addAction(KStandardAction::Print,  "search_print", this, SLOT(slotPrintMsg()));
  mClearAction  = new KAction(i18n("Clear Selection"), this);
  actionCollection()->addAction("search_clear_selection", mClearAction );
  connect(mClearAction, SIGNAL(triggered(bool)), SLOT(slotClearSelection()));

  mCopyAction = ac->addAction( KStandardAction::Copy, "search_copy_messages", this, SLOT(slotCopyMsgs()) );
  mCutAction = ac->addAction( KStandardAction::Cut, "search_cut_messages", this, SLOT(slotCutMsgs()) );

  connect(mTimer, SIGNAL(timeout()), this, SLOT(updateStatusLine()));
  connect(mCbxFolders, SIGNAL(folderChanged(const Akonadi::Collection&)),
          this, SLOT(slotFolderActivated()));

  ac->addAssociatedWidget( this );
  foreach (QAction* action, ac->actions())
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

//-----------------------------------------------------------------------------
SearchWindow::~SearchWindow()
{
  if ( mResultModel ) {
    KSharedConfig::Ptr config = KMKernel::config();
    KConfigGroup group( config, "SearchDialog" );
    group.writeEntry( "SubjectWidth", mLbxMatches->columnWidth( 0 ) );
    group.writeEntry( "SenderWidth", mLbxMatches->columnWidth( 1 ) );
    group.writeEntry( "DateWidth", mLbxMatches->columnWidth( 2 ) );
    group.writeEntry( "FolderWidth", mLbxMatches->columnWidth( 3 ) );
    group.writeEntry( "SearchWidgetWidth", width());
    group.writeEntry( "SearchWidgetHeight", height());
    config->sync();
  }
}

void SearchWindow::setEnabledSearchButton( bool )
{
  //Make sure that button is enable
  //Before when we selected a folder == "Local Folder" as that it was not a folder
  //search button was disable, and when we select "Search in all local folder"
  //Search button was never enabled :(
  enableButton( User1, true );
}

//-----------------------------------------------------------------------------
void SearchWindow::updateStatusLine()
{
    QString genMsg, detailMsg;
    int numMatches = 0;
    if ( mFolder.isValid() ) {
        numMatches = mFolder.statistics().count();
    }

    if ( mFolder.isValid() && mSearchJob ) {
        if(!mStopped) {
            genMsg = i18nc( "Search finished.", "Done" );
            detailMsg = i18np( "%1 match", "%1 matches", numMatches );
        } else {
            genMsg = i18n( "Search canceled" );
            detailMsg = i18np( "%1 match so far",
                               "%1 matches so far", numMatches );
        }
    } else {
        genMsg = i18np( "%1 match", "%1 matches", numMatches );
        detailMsg = i18n( "Searching in %1", mFolder.name() );
    }

    mStatusBar->changeItem(genMsg, 0);
    mStatusBar->changeItem(detailMsg, 1);
}


//-----------------------------------------------------------------------------
void SearchWindow::keyPressEvent(QKeyEvent *evt)
{
    if (evt->key() == Qt::Key_Escape && mSearchJob) {
        slotStop();
        return;
    }
    KDialog::keyPressEvent(evt);
}


//-----------------------------------------------------------------------------
void SearchWindow::slotFolderActivated()
{
    mChkbxSpecificFolders->setChecked(true);
}

//-----------------------------------------------------------------------------
void SearchWindow::activateFolder(const Akonadi::Collection& curFolder)
{
    mChkbxSpecificFolders->setChecked(true);
    mCbxFolders->setFolder(curFolder);
}

//-----------------------------------------------------------------------------
void SearchWindow::slotSearch()
{
  mLastFocus = focusWidget();
  setButtonFocus( User1 );     // set focus so we don't miss key event

  mStopped = false;

  mSearchFolderOpenBtn->setEnabled( true );
  if ( mSearchFolderEdt->text().isEmpty() ) {
    mSearchFolderEdt->setText( i18n("Last Search") );
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
#if 0
  if ( mChkbxAllFolders->isChecked() ) {
    search->setRecursive( true );
  } else {
    search->setRoot( mCbxFolders->folder() );
    search->setRecursive( mChkSubFolders->isChecked() );
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif

  mPatternEdit->updateSearchPattern();
  KMSearchPattern searchPattern( mSearchPattern );
  searchPattern.purify();
  enableGUI();

  mTimer->start( 200 );

  kDebug() << searchPattern.asSparqlQuery();

  if ( !mFolder.isValid() ) {
    mSearchJob = new Akonadi::SearchCreateJob( mSearchFolderEdt->text(), searchPattern.asSparqlQuery(), this );
  } else {
    Akonadi::PersistentSearchAttribute *psa = mFolder.attribute<Akonadi::PersistentSearchAttribute>();
    psa->setQueryString( searchPattern.asSparqlQuery() );
    mFolder.addAttribute( psa );
    mSearchJob = new Akonadi::CollectionModifyJob( mFolder, this );
  }
  connect( mSearchJob, SIGNAL(result(KJob*)), SLOT(searchDone(KJob*)) );
}

//-----------------------------------------------------------------------------
void SearchWindow::searchDone( KJob* job )
{
    Q_ASSERT( job == mSearchJob );
    if ( job->error() )
      kWarning() << job->errorText(); // TODO

    if ( Akonadi::SearchCreateJob *scj = qobject_cast<Akonadi::SearchCreateJob*>( mSearchJob ) ) {
      mFolder = scj->createdCollection();
    } else if ( Akonadi::CollectionModifyJob *mj = qobject_cast<Akonadi::CollectionModifyJob*>( mSearchJob ) ) {
      mFolder = mj->collection();
    }

    // store the kmail specific serialization of the search in an attribute on
    // the server, for easy retrieval when editing it again
    const QByteArray search = mSearchPattern.serialize();
    Q_ASSERT( !search.isEmpty() );
    Akonadi::SearchDescriptionAttribute *searchDescription  = mFolder.attribute<Akonadi::SearchDescriptionAttribute>( Akonadi::Entity::AddIfMissing );
    searchDescription->setDescription( search );
    const Akonadi::Collection col = mCbxFolders->folderCollection();
    searchDescription->setBaseCollection( col );
    searchDescription->setRecursive( mChkSubFolders->isChecked() );
    new Akonadi::CollectionModifyJob( mFolder, this );

    mSearchJob = 0;

    if ( !mResultModel ) {
      mResultModel = new Akonadi::MessageModel( this );
      mResultModel->setCollection( mFolder );
      mLbxMatches->setModel( mResultModel );

      KSharedConfig::Ptr config = KMKernel::config();
      KConfigGroup group( config, "SearchDialog" );

      mLbxMatches->setColumnWidth( 0, group.readEntry( "SubjectWidth", 150 ) );
      mLbxMatches->setColumnWidth( 1, group.readEntry( "SenderWidth", 120 ) );
      mLbxMatches->setColumnWidth( 2, group.readEntry( "DateWidth", 120 ) );
      mLbxMatches->setColumnWidth( 3, group.readEntry( "FolderWidth", 100 ) );
      mLbxMatches->setColumnWidth( 4, 0 );
    } else {
      mResultModel->setCollection( mFolder );
      mLbxMatches->setModel( mResultModel );
      mLbxMatches->header()->restoreState( mHeaderState );
    }

    mTimer->stop();
    updateStatusLine();

    QTimer::singleShot(0, this, SLOT(enableGUI()));
    if(mLastFocus)
        mLastFocus->setFocus();
    if (mCloseRequested)
        close();

    mLbxMatches->setSortingEnabled( true );
    mLbxMatches->header()->setSortIndicator( mSortColumn, mSortOrder );

    mSearchFolderEdt->setEnabled( true );
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void SearchWindow::slotClose()
{
  accept();
}

//-----------------------------------------------------------------------------
void SearchWindow::closeEvent(QCloseEvent *e)
{
  if ( mSearchJob ) {
    mCloseRequested = true;
    //Cancel search in progress
    mSearchJob->kill( KJob::Quietly );
    mSearchJob->deleteLater();
    mSearchJob = 0;
    QTimer::singleShot( 0, this, SLOT( slotClose() ) );
  } else {
    KDialog::closeEvent( e );
  }
}

//-----------------------------------------------------------------------------
void SearchWindow::scheduleRename( const QString &s )
{
  if ( !s.isEmpty() ) {
    mRenameTimer.setSingleShot( true );
    mRenameTimer.start( 250 );
    mSearchFolderOpenBtn->setEnabled( false );
  } else {
    mRenameTimer.stop();
    mSearchFolderOpenBtn->setEnabled( !s.isEmpty() );
  }
}

//-----------------------------------------------------------------------------
void SearchWindow::renameSearchFolder()
{
  if ( mFolder.isValid() && ( mFolder.name() != mSearchFolderEdt->text() ) ) {
    mFolder.setName( mSearchFolderEdt->text() );
    Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( mFolder, this );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( slotSearchFolderRenameDone( KJob* ) ) );
  }
  if ( mFolder.isValid() )
    mSearchFolderOpenBtn->setEnabled( true );
}

void SearchWindow::slotSearchFolderRenameDone( KJob *job )
{
  Q_ASSERT( job );
  if ( job->error() ) {
    kWarning() << "Job failed:" << job->errorText();
    KMessageBox::information( this, i18n("There was a problem renaming your search folder. "
                               "A common reason for this is that another search folder "
                               "with the same name already exists.") );
  } else {
    kDebug() << "Search Collection succesfully renamed.";
  }
}

void SearchWindow::openSearchFolder()
{
  Q_ASSERT( mFolder.isValid() );
  renameSearchFolder();
  mKMMainWidget->selectCollectionFolder( mFolder );
  slotClose();
}

//-----------------------------------------------------------------------------
bool SearchWindow::slotShowMsg( const Akonadi::Item &item )
{
  if ( !item.isValid() )
    return false;
  mKMMainWidget->slotMessageSelected( item );
  return true;
}

//-----------------------------------------------------------------------------
void SearchWindow::slotViewSelectedMsg()
{
  mKMMainWidget->slotMessageActivated( message() );
}

//-----------------------------------------------------------------------------
bool SearchWindow::slotViewMsg( const Akonadi::Item &item )
{
  if ( item.isValid() ) {
    mKMMainWidget->slotMessageActivated( item );
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void SearchWindow::slotCurrentChanged( const Akonadi::Item &item )
{
  mSearchResultOpenBtn->setEnabled( item.isValid() );
}

//-----------------------------------------------------------------------------
void SearchWindow::enableGUI()
{
    const bool searching = mSearchJob != 0;
    enableButton(KDialog::Close, !searching);
    mCbxFolders->setEnabled(!searching && !mChkbxAllFolders->isChecked());
    mChkSubFolders->setEnabled(!searching && !mChkbxAllFolders->isChecked());
    mChkbxAllFolders->setEnabled(!searching);
    mChkbxSpecificFolders->setEnabled(!searching);
    mPatternEdit->setEnabled(!searching);
    enableButton(User1, !searching);
    enableButton(User2, searching);
}

//-----------------------------------------------------------------------------
QList<Akonadi::Item> SearchWindow::selectedMessages()
{
  QList<Akonadi::Item> msgList;
#if 0 // TODO port me!
    KMFolder* folder = 0;
    int msgIndex = -1;
    for (QTreeWidgetItemIterator it(mLbxMatches); (*it); ++it)
        if ((*it)->isSelected()) {
            KMMsgDict::instance()->getLocation((*it)->text(MSGID_COLUMN).toUInt(),
                                           &folder, &msgIndex);
            if (folder && msgIndex >= 0)
                msgList.append(folder->getMsgBase(msgIndex));
        }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
    return msgList;
}

//-----------------------------------------------------------------------------
Akonadi::Item SearchWindow::message()
{
  return mLbxMatches->currentIndex().data( Akonadi::ItemModel::ItemRole ).value<Akonadi::Item>();
}
#if 0
//-----------------------------------------------------------------------------
void SearchWindow::slotMoveSelectedMessagesToFolder( QAction* act )
{
  KMFolder *dest = static_cast<KMFolder *>( act->data().value<void *>() );
  if ( !dest )
    return;
#if 0
  // Fixme: isn't this already handled by KMHeaders ?
  QList<Akonadi::Item> msgList = selectedMessages();
  KMCommand *command = new KMMoveCommand( dest, msgList );
  command->start();
#endif
}

//-----------------------------------------------------------------------------
void SearchWindow::slotCopySelectedMessagesToFolder( QAction* act )
{
#ifdef OLD_COMMAND
  KMFolder *dest = static_cast<KMFolder *>( act->data().value<void *>() );
  if ( !dest )
    return;

  // Fixme: isn't this already handled by KMHeaders ?
  QList<Akonadi::Item> msgList = selectedMessages();
  KMCommand *command = new KMCopyCommand( dest, msgList );
  command->start();
#endif
}
#endif

//-----------------------------------------------------------------------------
void SearchWindow::updateContextMenuActions()
{
    int count = selectedMessages().count();
    bool single_actions = count == 1;
    mReplyAction->setEnabled( single_actions );
    mReplyAllAction->setEnabled( single_actions );
    mReplyListAction->setEnabled( single_actions );
    mPrintAction->setEnabled( single_actions );
    mCopyAction->setEnabled( count > 0 );
    mCutAction->setEnabled( count > 0 );
}

#if 0
//-----------------------------------------------------------------------------
void SearchWindow::slotContextMenuRequested( QTreeWidgetItem *lvi )
{
    if (!lvi)
        return;
    if ( !lvi->isSelected() ) {
      lvi->setSelected( lvi );
#if 0 // TODO port me!
      mLbxMatches->setCurrentItem( lvi );
#endif
    }

    // FIXME is this ever unGetMsg()'d?
    if (!message().isValid())
        return;
    QMenu *menu = new QMenu(this);
    updateContextMenuActions();
#ifdef OLD_FOLDERVIEW
    QMenu *msgMoveMenu = new QMenu(menu);
    mKMMainWidget->mainFolderView()->folderToPopupMenu( MainFolderView::MoveMessage,
        this, msgMoveMenu );
    QMenu *msgCopyMenu = new QMenu(menu);
    mKMMainWidget->mainFolderView()->folderToPopupMenu( MainFolderView::CopyMessage,
        this, msgCopyMenu );
#endif
    // show most used actions
    menu->addAction( mReplyAction );
    menu->addAction( mReplyAllAction );
    menu->addAction( mReplyListAction );
    menu->addAction( mForwardActionMenu );
    menu->addSeparator();
    menu->addAction( mCopyAction );
    menu->addAction( mCutAction );
#ifdef OLD_FOLDERVIEW
    msgCopyMenu->setTitle( i18n( "&Copy To" ) );
    msgCopyMenu->setIcon( KIcon( "edit-copy" ) );
    menu->addMenu( msgCopyMenu );

    msgMoveMenu->setTitle( i18n( "&Move To" ) );
    msgMoveMenu->setIcon( KIcon( "go-jump" ) );
    menu->addMenu( msgMoveMenu );
#endif
    menu->addSeparator();
    menu->addAction( mSaveAsAction );
    menu->addAction( mSaveAtchAction );
    menu->addAction( mPrintAction );
    menu->addSeparator();
    menu->addAction( mClearAction );
    menu->exec( QCursor::pos(), 0 );
    delete menu;
}
#endif
//-----------------------------------------------------------------------------
void SearchWindow::slotClearSelection()
{
    mLbxMatches->clearSelection();
}

//-----------------------------------------------------------------------------
void SearchWindow::slotReplyToMsg()
{
    KMCommand *command = new KMReplyToCommand(this, message());
    command->start();
}

//-----------------------------------------------------------------------------
void SearchWindow::slotReplyAllToMsg()
{
    KMCommand *command = new KMReplyToAllCommand(this, message());
    command->start();
}

//-----------------------------------------------------------------------------
void SearchWindow::slotReplyListToMsg()
{
    KMCommand *command = new KMReplyListCommand(this, message());
    command->start();
}

//-----------------------------------------------------------------------------
void SearchWindow::slotForwardMsg()
{
    KMCommand *command = new KMForwardCommand(this, selectedMessages());
    command->start();
}

//-----------------------------------------------------------------------------
void SearchWindow::slotForwardAttachedMsg()
{
    KMCommand *command = new KMForwardAttachedCommand(this, selectedMessages());
    command->start();
}

//-----------------------------------------------------------------------------
void SearchWindow::slotSaveMsg()
{
    KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand(this,
                                                         selectedMessages());
    if (saveCommand->url().isEmpty())
        delete saveCommand;
    else
        saveCommand->start();
}
//-----------------------------------------------------------------------------
void SearchWindow::slotSaveAttachments()
{
    KMSaveAttachmentsCommand *saveCommand = new KMSaveAttachmentsCommand(this,
                                                                         selectedMessages());
    saveCommand->start();
}


//-----------------------------------------------------------------------------
void SearchWindow::slotPrintMsg()
{
    KMCommand *command = new KMPrintCommand(this, message());
    command->start();
}

void SearchWindow::slotCopyMsgs()
{
#if 0 //Port to akonadi
  QList<quint32> list = MessageCopyHelper::serNumListFromMsgList( selectedMessages() );
  mKMMainWidget->setMessageClipboardContents( list, false );
#endif
}

void SearchWindow::slotCutMsgs()
{
#if 0  //Port to akonadi
  QList<quint32> list = MessageCopyHelper::serNumListFromMsgList( selectedMessages() );
  mKMMainWidget->setMessageClipboardContents( list, true );
#endif
}

void SearchWindow::addRulesToSearchPattern( const KMSearchPattern &pattern )
{
  KMSearchPattern p( mSearchPattern );
  p.purify();
  QList<KMSearchRule::Ptr>::const_iterator it;
  for ( it = pattern.begin() ; it != pattern.end() ; ++it ) {
     p.append( KMSearchRule::createInstance( **it ) );
  }
  mSearchPattern = p;
  mPatternEdit->setSearchPattern( &mSearchPattern );
}

void SearchWindow::setSearchPattern( const KMSearchPattern &pattern )
{
  mSearchPattern = pattern;
  mPatternEdit->setSearchPattern( &mSearchPattern );
}

} // namespace KMail
#include "searchwindow.moc"
