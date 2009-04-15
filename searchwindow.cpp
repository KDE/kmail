/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 * Copyright (c) 2001 Aaron J. Seigo <aseigo@kde.org>
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
#include <QComboBox>
#include <QCursor>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QRadioButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <KActionMenu>
#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KIcon>
#include <KIconLoader>
#include <KLineEdit>
#include <KPushButton>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <kstatusbar.h>
#include <KWindowSystem>
#include <kdeversion.h>

#include "folderrequester.h"
#include "kmcommands.h"
#include "kmfoldermgr.h"
#include "kmfoldersearch.h"
#include "kmfoldertree.h"
#include "kmheaders.h"
#include "kmmainwidget.h"
#include "kmmsgdict.h"
#include "kmsearchpatternedit.h"
#include "kmsearchpattern.h"
#include "messagecopyhelper.h"
#include "regexplineedit.h"
#include "textsource.h"

#include <maillistdrag.h>
using namespace KPIM;

#include <mimelib/boyermor.h>
#include <mimelib/enum.h>

#include <assert.h>
#include <stdlib.h>

namespace KMail {

const int SearchWindow::MSGID_COLUMN = 4;

MatchListView::MatchListView( QWidget *parent, SearchWindow* sw ) :
      QTreeWidget( parent ),
      mSearchWindow( sw )
{}

void MatchListView::contextMenuEvent( QContextMenuEvent* event )
{
  emit contextMenuRequested( itemAt( event->pos() ) );
}

void MatchListView::startDrag ( Qt::DropActions supportedActions )
{
  QList<KMMsgBase*> list = mSearchWindow->selectedMessages();
  MailList mailList;
  foreach ( KMMsgBase* msg, list ) {
    if ( !msg )
      continue;
    MailSummary mailSummary( msg->getMsgSerNum(), msg->msgIdMD5(),
                             msg->subject(), msg->fromStrip(),
                             msg->toStrip(), msg->date() );
    mailList.append( mailSummary );
  }
  QDrag *drag = new QDrag( viewport() );
  drag->setMimeData( new MailListMimeData( new KMTextSource() ) );
  mailList.populateMimeData( drag->mimeData() );

  QPixmap pixmap;
  if( mailList.count() == 1 )
    pixmap = QPixmap( DesktopIcon("mail-message", KIconLoader::SizeSmall) );
  else
    pixmap = QPixmap( DesktopIcon("document-multiple", KIconLoader::SizeSmall) );

  drag->setPixmap( pixmap );
  drag->exec( supportedActions );
}

//-----------------------------------------------------------------------------
SearchWindow::SearchWindow(KMMainWidget* w, KMFolder *curFolder):
  KDialog(0),
  mStopped(false),
  mCloseRequested(false),
  mSortColumn(0),
  mSortOrder(Qt::AscendingOrder),
  mFolder(0),
  mTimer(new QTimer(this)),
  mLastFocus(0),
  mKMMainWidget(w)
{
  setCaption( i18n("Find Messages") );
  setButtons( User1 | User2 | Close );
  setDefaultButton( User1 );
  setButtonGuiItem( User1, KGuiItem( i18n("&Search"), "edit-find" ) );
  setButtonGuiItem( User2, KStandardGuiItem::stop() );
  KWindowSystem::setIcons( winId(), qApp->windowIcon().pixmap(
                               IconSize( KIconLoader::Desktop ),
                               IconSize( KIconLoader::Desktop ) ),
                           qApp->windowIcon().pixmap(
                               IconSize( KIconLoader::Small ),
                               IconSize( KIconLoader::Small ) ) );

  KConfig* config = KMKernel::config();
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

  mChkbxSpecificFolders = new QRadioButton(i18n("Search &only in:"), searchWidget );
  mChkbxSpecificFolders->setChecked(true);

  mCbxFolders = new FolderRequester( searchWidget,
      kmkernel->getKMMainWidget()->folderTree() );
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
  mSearchPattern = new KMSearchPattern();
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
      mFolder = searchFolder;
  }
  else
      mSearchPattern->append( KMSearchRule::createInstance( "Subject" ) );

  mPatternEdit->setSearchPattern( mSearchPattern );


  // enable/disable widgets depending on radio buttons:
  connect( mChkbxSpecificFolders, SIGNAL(toggled(bool)),
           mCbxFolders, SLOT(setEnabled(bool)) );
  connect( mChkbxSpecificFolders, SIGNAL(toggled(bool)),
           mChkSubFolders, SLOT(setEnabled(bool)) );
  connect( mChkbxAllFolders, SIGNAL(toggled(bool)),
           this, SLOT(setEnabledSearchButton(bool)) );


  mLbxMatches = new MatchListView(searchWidget, this);
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
  mLbxMatches->sortItems( 2, Qt::DescendingOrder );
  mLbxMatches->header()->setSortIndicator( 2, Qt::DescendingOrder );
  mLbxMatches->setAllColumnsShowFocus( true );
  mLbxMatches->setSelectionMode( QAbstractItemView::ExtendedSelection );
  QStringList headerNames;
  headerNames << i18n("Subject") << i18n("Sender/Receiver") << i18n("Date")
              << i18n("Folder") << "";
  mLbxMatches->setHeaderLabels( headerNames );
  mLbxMatches->header()->setStretchLastSection( false );
  mLbxMatches->header()->setResizeMode( 3, QHeaderView::Stretch );
  mLbxMatches->setColumnWidth( 0, group.readEntry( "SubjectWidth", 150 ) );
  mLbxMatches->setColumnWidth( 1, group.readEntry( "SenderWidth", 120 ) );
  mLbxMatches->setColumnWidth( 2, group.readEntry( "DateWidth", 120 ) );
  mLbxMatches->setColumnWidth( 3, group.readEntry( "FolderWidth", 100 ) );
  mLbxMatches->setColumnWidth( 4, 0 );

  connect(mLbxMatches, SIGNAL(itemDoubleClicked(QTreeWidgetItem *,int)),
          this, SLOT(slotShowMsg(QTreeWidgetItem *,int)));
  connect(mLbxMatches, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
          this, SLOT(slotCurrentChanged(QTreeWidgetItem *)));
  connect( mLbxMatches, SIGNAL( contextMenuRequested( QTreeWidgetItem*) ),
           this, SLOT( slotContextMenuRequested( QTreeWidgetItem* ) ) );
  mLbxMatches->setDragEnabled( true );


  QHBoxLayout *hbl2 = new QHBoxLayout();
  hbl2->setObjectName( "kmfs_hbl2" );
  mSearchFolderLbl = new QLabel(i18n("Search folder &name:"),searchWidget);
  mSearchFolderEdt = new KLineEdit(searchWidget);
  if (searchFolder)
    mSearchFolderEdt->setText(searchFolder->folder()->name());
  else
    mSearchFolderEdt->setText(i18n("Last Search"));

  mSearchFolderLbl->setBuddy(mSearchFolderEdt);
  mSearchFolderOpenBtn = new KPushButton(i18n("Op&en Search Folder"), searchWidget);
  mSearchFolderOpenBtn->setEnabled(false);
  connect( mSearchFolderEdt, SIGNAL( textChanged( const QString &)),
           this, SLOT( scheduleRename( const QString & )));
  connect( &mRenameTimer, SIGNAL( timeout() ),
           this, SLOT( renameSearchFolder() ));
  connect( mSearchFolderOpenBtn, SIGNAL( clicked() ),
           this, SLOT( openSearchFolder() ));

  mSearchResultOpenBtn = new KPushButton(i18n("Open &Message"), searchWidget);
  mSearchResultOpenBtn->setEnabled(false);
  connect( mSearchResultOpenBtn, SIGNAL( clicked() ),
           this, SLOT( slotShowSelectedMsg() ));

  hbl2->addWidget(mSearchFolderLbl);
  hbl2->addWidget(mSearchFolderEdt);
  hbl2->addWidget(mSearchFolderOpenBtn);
  hbl2->addWidget(mSearchResultOpenBtn);

  mStatusBar = new KStatusBar(searchWidget);
  mStatusBar->insertPermanentItem(i18n("AMiddleLengthText..."), 0);
  mStatusBar->changeItem(i18n("Ready."), 0);
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
      kDebug(5006) <<"SearchWindow: regExpLineEdit not found";

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

  mForwardActionMenu  = new KActionMenu(KIcon("mail-forward"), i18nc("Message->","&Forward"), this);
  actionCollection()->addAction("search_message_forward", mForwardActionMenu );
  connect( mForwardActionMenu, SIGNAL(triggered(bool)), this,
           SLOT(slotForwardMsg()) );

  mForwardInlineAction  = new KAction(KIcon("mail-forward"), i18n("&Inline..."), this);
  actionCollection()->addAction("search_message_forward_inline", mForwardInlineAction );
  connect(mForwardInlineAction, SIGNAL(triggered(bool) ), SLOT(slotForwardMsg()));

  mForwardAttachedAction  = new KAction(KIcon("mail-forward"), i18nc("Message->Forward->","As &Attachment..."), this);
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

  connect(mTimer, SIGNAL(timeout()), this, SLOT(updStatus()));
  connect(kmkernel->searchFolderMgr(), SIGNAL(folderInvalidated(KMFolder*)),
          this, SLOT(folderInvalidated(KMFolder*)));

  connect(mCbxFolders, SIGNAL(folderChanged(KMFolder*)),
          this, SLOT(slotFolderActivated()));

  ac->addAssociatedWidget( this );
  foreach (QAction* action, ac->actions())
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

//-----------------------------------------------------------------------------
SearchWindow::~SearchWindow()
{
  QList<QPointer<KMFolder> >::Iterator fit;
  for ( fit = mFolders.begin(); fit != mFolders.end(); ++fit ) {
    if ( !(*fit) ) {
      continue;
    }
    (*fit)->close( "searchwindow" );
  }

  KConfig *config = KMKernel::config();
  KConfigGroup group( config, "SearchDialog" );
  group.writeEntry( "SubjectWidth", mLbxMatches->columnWidth( 0 ) );
  group.writeEntry( "SenderWidth", mLbxMatches->columnWidth( 1 ) );
  group.writeEntry( "DateWidth", mLbxMatches->columnWidth( 2 ) );
  group.writeEntry( "FolderWidth", mLbxMatches->columnWidth( 3 ) );
  group.writeEntry( "SearchWidgetWidth", width());
  group.writeEntry( "SearchWidgetHeight", height());
  config->sync();
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
void SearchWindow::updStatus(void)
{
    QString genMsg, detailMsg;
    int numMatches = 0, count = 0;
    KMSearch const *search = (mFolder) ? (mFolder->search()) : 0;
    QString folderName;
    if (search) {
        numMatches = search->foundCount();
        count = search->searchCount();
        folderName = search->currentFolder();
    }

    if (search && !search->running()) {
        if(!mStopped) {
            genMsg = i18n("Done");
            detailMsg = i18np("%1 match (%2)", "%1 matches (%2)", numMatches,
                         i18np("%1 message processed",
                                  "%1 messages processed", count));
        } else {
            genMsg = i18n("Search canceled");
            detailMsg = i18np("%1 match so far (%2)",
                             "%1 matches so far (%2)", numMatches,
                         i18np("%1 message processed",
                                  "%1 messages processed", count));
        }
    } else {
        genMsg = i18np("%1 match", "%1 matches", numMatches);
        detailMsg = i18n("Searching in %1 (message %2)",
                     folderName,
                     count);
    }

    mStatusBar->changeItem(genMsg, 0);
    mStatusBar->changeItem(detailMsg, 1);
}


//-----------------------------------------------------------------------------
void SearchWindow::keyPressEvent(QKeyEvent *evt)
{
    KMSearch const *search = (mFolder) ? mFolder->search() : 0;
    bool searching = (search) ? search->running() : false;
    if (evt->key() == Qt::Key_Escape && searching) {
        mFolder->stopSearch();
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
void SearchWindow::activateFolder(KMFolder *curFolder)
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
  mFetchingInProgress = 0;

  mSearchFolderOpenBtn->setEnabled( true );
  if ( mSearchFolderEdt->text().isEmpty() ) {
    mSearchFolderEdt->setText( i18n("Last Search") );
  }
  enableButton( User1, false );
  enableButton( User2, true );

  mLbxMatches->clear();

  mSortColumn = mLbxMatches->sortColumn();
  mSortOrder = mLbxMatches->header()->sortIndicatorOrder();
  mLbxMatches->setSortingEnabled( false );

  // If we haven't openend an existing search folder, find or create one.
  if ( !mFolder ) {
    KMFolderMgr *mgr = kmkernel->searchFolderMgr();
    QString baseName = mSearchFolderEdt->text();
    QString fullName = baseName;
    int count = 0;
    KMFolder *folder;
    while ( ( folder = mgr->find( fullName ) ) ) {
      if ( folder->storage()->inherits( "KMFolderSearch" ) ) {
        break;
      }
      fullName = QString( "%1 %2" ).arg( baseName ).arg( ++count );
    }

    if ( !folder ) {
      folder =
        mgr->createFolder( fullName, false, KMFolderTypeSearch, &mgr->dir() );
    }
    mFolder = dynamic_cast<KMFolderSearch*>( folder->storage() );
  }
  mFolder->stopSearch();
  disconnect( mFolder, SIGNAL( msgAdded( int ) ),
              this, SLOT( slotAddMsg( int ) ) );
  disconnect( mFolder, SIGNAL( msgRemoved( KMFolder*, quint32  )),
              this, SLOT( slotRemoveMsg( KMFolder*, quint32 ) ) );
  connect( mFolder, SIGNAL( msgAdded( int ) ),
           this, SLOT( slotAddMsg( int ) ) );
  connect( mFolder, SIGNAL( msgRemoved( KMFolder*, quint32 ) ),
           this, SLOT( slotRemoveMsg( KMFolder*, quint32 ) ) );
  mSearchFolderEdt->setEnabled(false);
  KMSearch *search = new KMSearch();
  connect( search, SIGNAL( finished( bool ) ),
           this, SLOT( searchDone() ) );
  if ( mChkbxAllFolders->isChecked() ) {
    search->setRecursive( true );
  } else {
    search->setRoot( mCbxFolders->folder() );
    search->setRecursive( mChkSubFolders->isChecked() );
  }

  mPatternEdit->updateSearchPattern();
  KMSearchPattern *searchPattern = new KMSearchPattern();
  *searchPattern = *mSearchPattern; //deep copy
  searchPattern->purify();
  search->setSearchPattern( searchPattern );
  mFolder->setSearch( search );
  enableGUI();

  mTimer->start( 200 );
}

//-----------------------------------------------------------------------------
void SearchWindow::searchDone()
{
    mTimer->stop();
    updStatus();

    QTimer::singleShot(0, this, SLOT(enableGUI()));
    if(mLastFocus)
        mLastFocus->setFocus();
    if (mCloseRequested)
        close();

    mLbxMatches->setSortingEnabled( true );
    mLbxMatches->sortByColumn( mSortColumn, mSortOrder );

    mSearchFolderEdt->setEnabled(true);
}

void SearchWindow::slotAddMsg( int idx )
{
  if ( !mFolder ) {
    return;
  }
  bool unget = !mFolder->isMessage( idx );
  KMMessage *msg = mFolder->getMsg( idx );
  QString from, fName;
  KMFolder *pFolder = msg->parent();
  if ( !mFolders.contains( pFolder ) ) {
    mFolders.append( pFolder );
    pFolder->open( "searchwindow" );
  }
  if( pFolder->whoField() == "To" ) {
    from = msg->to();
  } else {
    from = msg->from();
  }
  if ( pFolder->isSystemFolder() ) {
    fName = i18n( pFolder->name().toUtf8() );
  } else {
    fName = pFolder->name();
  }

  QTreeWidgetItem *newItem = new QTreeWidgetItem( mLbxMatches );
  newItem->setText( 0, msg->subject() );
  newItem->setText( 1, from );
  newItem->setText( 2, msg->dateIsoStr() );
  newItem->setText( 3, fName );
  newItem->setText( 4,QString::number( mFolder->serNum( idx ) ) );
  mLbxMatches->addTopLevelItem( newItem );
  if ( unget ) {
    mFolder->unGetMsg( idx );
  }
}

void SearchWindow::slotRemoveMsg(KMFolder *, quint32 serNum)
{
    if (!mFolder)
        return;
    QTreeWidgetItemIterator it(mLbxMatches);
    while ( (*it) ) {
        QTreeWidgetItem *item = *it;
        if (serNum == (*it)->text(MSGID_COLUMN).toUInt()) {
            delete mLbxMatches->takeTopLevelItem(
                mLbxMatches->indexOfTopLevelItem( item ) );
            return;
        }
        ++it;
    }
}

//-----------------------------------------------------------------------------
void SearchWindow::slotStop()
{
  if ( mFolder ) {
    mFolder->stopSearch();
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
  if ( mFolder && mFolder->search() && mFolder->search()->running() ) {
    mCloseRequested = true;
    //Cancel search in progress by setting the search folder search to
    //the null search
    mFolder->setSearch( new KMSearch() );
    QTimer::singleShot( 0, this, SLOT( slotClose() ) );
  } else {
    KDialog::closeEvent( e );
  }
}

//-----------------------------------------------------------------------------
void SearchWindow::scheduleRename(const QString &s)
{
  if (!s.isEmpty() && s != i18n("Last Search")) {
    mRenameTimer.start(250, true);
    mSearchFolderOpenBtn->setEnabled(false);
  } else {
    mRenameTimer.stop();
    mSearchFolderOpenBtn->setEnabled(!s.isEmpty());
  }
}

//-----------------------------------------------------------------------------
void SearchWindow::renameSearchFolder()
{
  if ( mFolder && ( mFolder->folder()->name() != mSearchFolderEdt->text() ) ) {
    int i = 1;
    QString name =  mSearchFolderEdt->text();
    while ( i < 100 ) {
      if ( !kmkernel->searchFolderMgr()->find( name ) ) {
        mFolder->rename( name );
        kmkernel->searchFolderMgr()->contentsChanged();
        break;
      }
      name.setNum( i );
      name = mSearchFolderEdt->text() + ' ' + name;
      ++i;
    }
  }
  if ( mFolder )
    mSearchFolderOpenBtn->setEnabled( true );
}

void SearchWindow::openSearchFolder()
{
  Q_ASSERT( mFolder );
  renameSearchFolder();
  mKMMainWidget->slotSelectFolder( mFolder->folder() );
  slotClose();
}

//-----------------------------------------------------------------------------
void SearchWindow::folderInvalidated(KMFolder *folder)
{
    if (folder->storage() == mFolder) {
        mLbxMatches->clear();
        if (mFolder->search())
            connect(mFolder->search(), SIGNAL(finished(bool)),
                    this, SLOT(searchDone()));
        mTimer->start(200);
        enableGUI();
    }
}

//-----------------------------------------------------------------------------
bool SearchWindow::slotShowMsg(QTreeWidgetItem *item,int)
{
    if(!item)
        return false;

    KMFolder* folder;
    int msgIndex;
    KMMsgDict::instance()->getLocation(item->text(MSGID_COLUMN).toUInt(),
                                   &folder, &msgIndex);

    if (!folder || msgIndex < 0)
        return false;

    mKMMainWidget->slotSelectFolder(folder);
    KMMessage* message = folder->getMsg(msgIndex);
    if (!message)
        return false;

    mKMMainWidget->slotSelectMessage(message);
    return true;
}

//-----------------------------------------------------------------------------
void SearchWindow::slotShowSelectedMsg()
{
  slotShowMsg( mLbxMatches->currentItem(), 0 );
}

//-----------------------------------------------------------------------------
void SearchWindow::slotCurrentChanged(QTreeWidgetItem *item)
{
  mSearchResultOpenBtn->setEnabled( item!=0 );
}

//-----------------------------------------------------------------------------
void SearchWindow::enableGUI()
{
    KMSearch const *search = (mFolder) ? (mFolder->search()) : 0;
    bool searching = (search) ? (search->running()) : false;
    enableButton(KDialog::Close, !searching);
    mCbxFolders->setEnabled(!searching);
    mChkSubFolders->setEnabled(!searching);
    mChkbxAllFolders->setEnabled(!searching);
    mChkbxSpecificFolders->setEnabled(!searching);
    mPatternEdit->setEnabled(!searching);
    enableButton(User1, !searching);
    enableButton(User2, searching);
}


//-----------------------------------------------------------------------------
QList<KMMsgBase*> SearchWindow::selectedMessages()
{
    QList<KMMsgBase*> msgList;
    KMFolder* folder = 0;
    int msgIndex = -1;
    for (QTreeWidgetItemIterator it(mLbxMatches); (*it); it++)
        if ((*it)->isSelected()) {
            KMMsgDict::instance()->getLocation((*it)->text(MSGID_COLUMN).toUInt(),
                                           &folder, &msgIndex);
            if (folder && msgIndex >= 0)
                msgList.append(folder->getMsgBase(msgIndex));
        }
    return msgList;
}

//-----------------------------------------------------------------------------
KMMessage* SearchWindow::message()
{
    QTreeWidgetItem *item = mLbxMatches->currentItem();
    KMFolder* folder = 0;
    int msgIndex = -1;
    if (!item)
        return 0;
    KMMsgDict::instance()->getLocation(item->text(MSGID_COLUMN).toUInt(),
                                   &folder, &msgIndex);
    if (!folder || msgIndex < 0)
        return 0;

    return folder->getMsg(msgIndex);
}

//-----------------------------------------------------------------------------
void SearchWindow::moveSelectedToFolder( QAction* act )
{
    KMFolder *dest = mMenuToFolder[act];
    if (!dest)
        return;

    QList<KMMsgBase*> msgList = selectedMessages();
    KMCommand *command = new KMMoveCommand( dest, msgList );
    command->start();
}

//-----------------------------------------------------------------------------
void SearchWindow::copySelectedToFolder( QAction* act )
{
    KMFolder *dest = mMenuToFolder[act];
    if (!dest)
        return;

    QList<KMMsgBase*> msgList = selectedMessages();
    KMCommand *command = new KMCopyCommand( dest, msgList );
    command->start();
}

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

//-----------------------------------------------------------------------------
void SearchWindow::slotContextMenuRequested( QTreeWidgetItem *lvi )
{
    if (!lvi)
        return;
    if ( !lvi->isSelected() ) {
      lvi->setSelected( lvi );
      mLbxMatches->setCurrentItem( lvi );
    }

    // FIXME is this ever unGetMsg()'d?
    if (!message())
        return;
    QMenu *menu = new QMenu(this);
    updateContextMenuActions();

    mMenuToFolder.clear();
    QMenu *msgMoveMenu = new QMenu(menu);
    mKMMainWidget->folderTree()->folderToPopupMenu( KMFolderTree::MoveMessage,
        this, &mMenuToFolder, msgMoveMenu );
    QMenu *msgCopyMenu = new QMenu(menu);
    mKMMainWidget->folderTree()->folderToPopupMenu( KMFolderTree::CopyMessage,
        this, &mMenuToFolder, msgCopyMenu );

    // show most used actions
    menu->addAction( mReplyAction );
    menu->addAction( mReplyAllAction );
    menu->addAction( mReplyListAction );
    menu->addAction( mForwardActionMenu );
    menu->addSeparator();
    menu->addAction( mCopyAction );
    menu->addAction( mCutAction );
    msgCopyMenu->setTitle( i18n( "&Copy To" ) );
    msgCopyMenu->setIcon( KIcon( "edit-copy" ) );
    menu->addMenu( msgCopyMenu );
    msgMoveMenu->setTitle( i18n( "&Move To" ) );
    msgMoveMenu->setIcon( KIcon( "go-jump" ) );
    menu->addMenu( msgMoveMenu );
    menu->addSeparator();
    menu->addAction( mSaveAsAction );
    menu->addAction( mSaveAtchAction );
    menu->addAction( mPrintAction );
    menu->addSeparator();
    menu->addAction( mClearAction );
    menu->exec( QCursor::pos(), 0 );
    delete menu;
}

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
  QList<quint32> list = MessageCopyHelper::serNumListFromMsgList( selectedMessages() );
  mKMMainWidget->headers()->setCopiedMessages( list, false );
}

void SearchWindow::slotCutMsgs()
{
  QList<quint32> list = MessageCopyHelper::serNumListFromMsgList( selectedMessages() );
  mKMMainWidget->headers()->setCopiedMessages( list, true );
}


void SearchWindow::setSearchPattern( const KMSearchPattern &pattern )
{
  *mSearchPattern = pattern;
  mPatternEdit->setSearchPattern( mSearchPattern );
}

} // namespace KMail
#include "searchwindow.moc"
