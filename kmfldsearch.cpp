/* kmfldsearch
 * (c) 1999 Stefan Taferner, (c) 2001 Aaron J. Seigo
 * This code is under GPL
 */
// kmfldsearch.cpp

#include <config.h>
#include "kmcommands.h"
#include "kmfldsearch.h"
#include "kmmainwidget.h"
#include "kmmsgdict.h"
#include "kmmsgpart.h"
#include "kmfolderdia.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmfoldersearch.h"
#include "kmfoldertree.h"
#include "kmsearchpatternedit.h"
#include "kmsearchpattern.h"
#include "folderrequester.h"
using KMail::FolderRequester;

#include <kapplication.h>
#include <kdebug.h>
#include <kstatusbar.h>
#include <kwin.h>
#include <kconfig.h>
#include <kstdaction.h>

#include <qcheckbox.h>
#include <qlayout.h>
#include <klineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qobjectlist.h> //for mPatternEdit->queryList( 0, "mRuleField" )->first();
#include <qcursor.h>

#include <mimelib/enum.h>
#include <mimelib/boyermor.h>

#include <assert.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

const int KMFldSearch::MSGID_COLUMN = 4;

//-----------------------------------------------------------------------------
KMFldSearch::KMFldSearch(KMMainWidget* w, const char* name,
                         KMFolder *curFolder, bool modal):
  KDialogBase(0, name, modal, i18n("Find Messages"),
              User1 | User2 | Close, User1, false,
              KGuiItem( i18n("&Search"), "find" ),
              KGuiItem( i18n("S&top"), "cancel" )),
  mStopped(false),
  mCloseRequested(false),
  mSortColumn(0),
  mSortOrder(Ascending),
  mFolder(0),
  mTimer(new QTimer(this)),
  mLastFocus(0),
  mKMMainWidget(w)
{
#if !KDE_IS_VERSION( 3, 2, 91 )
  // HACK - KWin keeps all dialogs on top of their mainwindows, but that's probably
  // wrong (#76026), and should be done only for modals. CVS HEAD should get
  // proper fix in KWin (l.lunak@kde.org)
  XDeleteProperty( qt_xdisplay(), winId(), XA_WM_TRANSIENT_FOR );
#endif
  KWin::setIcons(winId(), kapp->icon(), kapp->miniIcon());

  KConfig* config = KMKernel::config();
  config->setGroup("SearchDialog");

  QWidget* searchWidget = new QWidget(this);
  QVBoxLayout *vbl = new QVBoxLayout( searchWidget, 0, spacingHint(), "kmfs_vbl" );

  QButtonGroup * radioGroup = new QButtonGroup( searchWidget );
  radioGroup->hide();

  mChkbxAllFolders = new QRadioButton(i18n("Search in &all local folders"), searchWidget);
  vbl->addWidget( mChkbxAllFolders );
  radioGroup->insert( mChkbxAllFolders );

  QHBoxLayout *hbl = new QHBoxLayout( vbl, spacingHint(), "kmfs_hbl" );
  mChkbxSpecificFolders = new QRadioButton(i18n("Search &only in:"), searchWidget);
  hbl->addWidget(mChkbxSpecificFolders);
  mChkbxSpecificFolders->setChecked(true);
  radioGroup->insert( mChkbxSpecificFolders );

  mCbxFolders = new FolderRequester( searchWidget, 
      kmkernel->getKMMainWidget()->folderTree() );
  mCbxFolders->setMustBeReadWrite( false );
  mCbxFolders->setFolder(curFolder);
  hbl->addWidget(mCbxFolders);
    
  mChkSubFolders = new QCheckBox(i18n("I&nclude sub-folders"), searchWidget);
  mChkSubFolders->setChecked(true);
  hbl->addWidget(mChkSubFolders);

  QWidget *spacer = new QWidget( searchWidget, "spacer" );
  spacer->setMinimumHeight( 2 );
  vbl->addWidget( spacer );

  mPatternEdit = new KMSearchPatternEdit( "", searchWidget , "spe", false, true );
  mPatternEdit->setFrameStyle( QFrame::NoFrame | QFrame::Plain );
  mPatternEdit->setInsideMargin( 0 );
  mSearchPattern = new KMSearchPattern();
  KMFolderSearch *searchFolder = 0;
  if (curFolder)
      searchFolder = dynamic_cast<KMFolderSearch*>(curFolder->storage());
  if (searchFolder) {
      KConfig config(curFolder->location());
      KMFolder *root = searchFolder->search()->root();
      config.setGroup("Search Folder");
      mSearchPattern->readConfig(&config);
      if (root) {
	  mChkbxSpecificFolders->setChecked(true);
	  mCbxFolders->setFolder(root);
	  mChkSubFolders->setChecked(searchFolder->search()->recursive());
      } else {
	  mChkbxAllFolders->setChecked(true);
      }
      mFolder = searchFolder;
  }
  mPatternEdit->setSearchPattern( mSearchPattern );
  QObjectList *list = mPatternEdit->queryList( 0, "mRuleField" );
  QObject *object = 0;
  if ( list )
      object = list->first();
  delete list;
  if (!searchFolder && object && ::qt_cast<QComboBox*>(object))
      static_cast<QComboBox*>(object)->setCurrentText("Subject");

  vbl->addWidget( mPatternEdit );

  // enable/disable widgets depending on radio buttons:
  connect( mChkbxSpecificFolders, SIGNAL(toggled(bool)),
	   mCbxFolders, SLOT(setEnabled(bool)) );
  connect( mChkbxSpecificFolders, SIGNAL(toggled(bool)),
	   mChkSubFolders, SLOT(setEnabled(bool)) );

  mLbxMatches = new KListView(searchWidget, "Find Messages");

  /*
     Default is to sort by date. TODO: Unfortunately this sorts *while*
     inserting, which looks rather strange - the user cannot read
     the results so far as they are constantly re-sorted --dnaber

     Sorting is now disabled when a search is started and reenabled
     when it stops. Items are appended to the list. This not only
     solves the above problem, but speeds searches with many hits
     up considerably. - till

     TODO: subclass KListViewItem and do proper (and performant)
     comapare functions
  */
  mLbxMatches->setSorting(2, false);
  mLbxMatches->setShowSortIndicator(true);
  mLbxMatches->setAllColumnsShowFocus(true);
  mLbxMatches->setSelectionModeExt(KListView::Extended);
  mLbxMatches->addColumn(i18n("Subject"),
                         config->readNumEntry("SubjectWidth", 150));
  mLbxMatches->addColumn(i18n("Sender/Receiver"),
                         config->readNumEntry("SenderWidth", 120));
  mLbxMatches->addColumn(i18n("Date"),
                         config->readNumEntry("DateWidth", 120));
  mLbxMatches->addColumn(i18n("Folder"),
                         config->readNumEntry("FolderWidth", 100));

  mLbxMatches->addColumn(""); // should be hidden
  mLbxMatches->setColumnWidthMode( MSGID_COLUMN, QListView::Manual );
  mLbxMatches->setColumnWidth(MSGID_COLUMN, 0);
  mLbxMatches->header()->setResizeEnabled(false, MSGID_COLUMN);

  connect(mLbxMatches, SIGNAL(doubleClicked(QListViewItem *)),
          this, SLOT(slotShowMsg(QListViewItem *)));
  connect( mLbxMatches, SIGNAL( contextMenuRequested( QListViewItem*, const QPoint &, int )),
	   this, SLOT( slotContextMenuRequested( QListViewItem*, const QPoint &, int )));
  vbl->addWidget(mLbxMatches);

  QHBoxLayout *hbl2 = new QHBoxLayout( vbl, spacingHint(), "kmfs_hbl2" );
  mSearchFolderLbl = new QLabel(i18n("Search folder &name:"), searchWidget);
  hbl2->addWidget(mSearchFolderLbl);
  mSearchFolderEdt = new KLineEdit(searchWidget);
  if (searchFolder)
    mSearchFolderEdt->setText(searchFolder->folder()->name());
  else
    mSearchFolderEdt->setText(i18n("Last Search"));

  mSearchFolderLbl->setBuddy(mSearchFolderEdt);
  hbl2->addWidget(mSearchFolderEdt);
  mSearchFolderBtn = new QPushButton(i18n("&Rename"), searchWidget);
  mSearchFolderBtn->setEnabled(false);
  hbl2->addWidget(mSearchFolderBtn);
  mSearchFolderOpenBtn = new QPushButton(i18n("Op&en"), searchWidget);
  mSearchFolderOpenBtn->setEnabled(false);
  hbl2->addWidget(mSearchFolderOpenBtn);
  connect( mSearchFolderEdt, SIGNAL( textChanged( const QString &)),
	   this, SLOT( updateCreateButton( const QString & )));
  connect( mSearchFolderBtn, SIGNAL( clicked() ),
	   this, SLOT( renameSearchFolder() ));
  connect( mSearchFolderOpenBtn, SIGNAL( clicked() ),
	   this, SLOT( openSearchFolder() ));
  mStatusBar = new KStatusBar(searchWidget);
  mStatusBar->insertFixedItem(i18n("AMiddleLengthText..."), 0, true);
  mStatusBar->changeItem(i18n("Ready."), 0);
  mStatusBar->setItemAlignment(0, AlignLeft | AlignVCenter);
  mStatusBar->insertItem(QString::null, 1, 1, true);
  mStatusBar->setItemAlignment(1, AlignLeft | AlignVCenter);
  vbl->addWidget(mStatusBar);

  int mainWidth = config->readNumEntry("SearchWidgetWidth", 0);
  int mainHeight = config->readNumEntry("SearchWidgetHeight", 0);

  if (mainWidth || mainHeight)
    resize(mainWidth, mainHeight);

  setMainWidget(searchWidget);
  setButtonBoxOrientation(QWidget::Vertical);

  mBtnSearch = actionButton(KDialogBase::User1);
  mBtnStop = actionButton(KDialogBase::User2);
  mBtnStop->setEnabled(false);

  connect(this, SIGNAL(user1Clicked()), SLOT(slotSearch()));
  connect(this, SIGNAL(user2Clicked()), SLOT(slotStop()));
  connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));

  // give focus to the value field of the first search rule
  object = mPatternEdit->child( "regExpLineEdit" );
  if ( object && object->isWidgetType() ) {
      static_cast<QWidget*>(object)->setFocus();
      //kdDebug(5006) << "KMFldSearch: focus has been given to widget "
      //              << object->name() << endl;
  }
  else
      kdDebug(5006) << "KMFldSearch: regExpLineEdit not found" << endl;

  //set up actions
  KActionCollection *ac = actionCollection();
  mReplyAction = new KAction( i18n("&Reply..."), "mail_reply", 0, this,
			      SLOT(slotReplyToMsg()), ac, "search_reply" );
  mReplyAllAction = new KAction( i18n("Reply to &All..."), "mail_replyall",
				 0, this, SLOT(slotReplyAllToMsg()),
				 ac, "search_reply_all" );
  mReplyListAction = new KAction( i18n("Reply to Mailing-&List..."),
				  "mail_replylist", 0, this,
				  SLOT(slotReplyListToMsg()), ac,
				  "search_reply_list" );
  mForwardActionMenu = new KActionMenu( i18n("Message->","&Forward"),
					"mail_forward", ac,
					"search_message_forward" );
  connect( mForwardActionMenu, SIGNAL(activated()), this,
	   SLOT(slotForwardMsg()) );
  mForwardAction = new KAction( i18n("&Inline..."), "mail_forward",
				0, this, SLOT(slotForwardMsg()),
				ac, "search_message_forward_inline" );
  mForwardActionMenu->insert( mForwardAction );
  mForwardAttachedAction = new KAction( i18n("Message->Forward->","As &Attachment..."),
				       "mail_forward", 0, this,
					SLOT(slotForwardAttachedMsg()), ac,
					"search_message_forward_as_attachment" );
  mForwardActionMenu->insert( mForwardAttachedAction );
  mSaveAsAction = KStdAction::saveAs( this, SLOT(slotSaveMsg()), ac, "search_file_save_as" );
  mSaveAtchAction = new KAction( i18n("Save Attachments..."), "attach", 0,
                                 this, SLOT(slotSaveAttachments()), ac, "search_save_attachments" );

  mPrintAction = KStdAction::print( this, SLOT(slotPrintMsg()), ac, "search_print" );
  mClearAction = new KAction( i18n("Clear Selection"), 0, 0, this,
			      SLOT(slotClearSelection()), ac, "search_clear_selection" );
  connect(mTimer, SIGNAL(timeout()), this, SLOT(updStatus()));
  connect(kmkernel->searchFolderMgr(), SIGNAL(folderInvalidated(KMFolder*)),
	  this, SLOT(folderInvalidated(KMFolder*)));

  connect(mCbxFolders, SIGNAL(folderChanged(KMFolder*)),
          this, SLOT(slotFolderActivated(KMFolder*)));

}

//-----------------------------------------------------------------------------
KMFldSearch::~KMFldSearch()
{
  QValueListIterator<QGuardedPtr<KMFolder> > fit;
  for ( fit = mFolders.begin(); fit != mFolders.end(); ++fit ) {
    if (!(*fit))
      continue;
    (*fit)->close();
  }

  KConfig* config = KMKernel::config();
  config->setGroup("SearchDialog");
  config->writeEntry("SubjectWidth", mLbxMatches->columnWidth(0));
  config->writeEntry("SenderWidth", mLbxMatches->columnWidth(1));
  config->writeEntry("DateWidth", mLbxMatches->columnWidth(2));
  config->writeEntry("FolderWidth", mLbxMatches->columnWidth(3));
  config->writeEntry("SearchWidgetWidth", width());
  config->writeEntry("SearchWidgetHeight", height());
  config->sync();
}

//-----------------------------------------------------------------------------
void KMFldSearch::updStatus(void)
{
    QString genMsg, detailMsg;
    int numMatches = 0, count = 0;
    KMSearch const *search = (mFolder) ? (mFolder->search()) : 0;
    QString folderName;
    if (search) {
	numMatches = search->foundCount();
	count = search->searchedCount();
	folderName = search->currentFolder();
    }

    if (mFolder && mFolder->search() && !mFolder->search()->running()) {
	if(!mStopped) {
	    genMsg = i18n("Done");
	    detailMsg = i18n("%n match (%1)", "%n matches (%1)", numMatches)
			.arg(i18n("%n message processed",
				  "%n messages processed", count));
	} else {
	    genMsg = i18n("Search canceled");
	    detailMsg = i18n("%n match so far (%1)",
			     "%n matches so far (%1)", numMatches)
			.arg(i18n("%n message processed",
				  "%n messages processed", count));
	}
    } else {
	genMsg = i18n("%n match", "%n matches", numMatches);
	detailMsg = i18n("Searching in %1 (message %2)")
		    .arg(folderName)
		    .arg(count);
    }

    mStatusBar->changeItem(genMsg, 0);
    mStatusBar->changeItem(detailMsg, 1);
}


//-----------------------------------------------------------------------------
void KMFldSearch::keyPressEvent(QKeyEvent *evt)
{
    KMSearch const *search = (mFolder) ? mFolder->search() : 0;
    bool searching = (search) ? search->running() : false;
    if (evt->key() == Key_Escape && searching) {
	mFolder->stopSearch();
	return;
    }

    KDialogBase::keyPressEvent(evt);
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotFolderActivated( KMFolder* folder )
{
    mChkbxSpecificFolders->setChecked(true);
    mBtnSearch->setEnabled(folder);
}

//-----------------------------------------------------------------------------
void KMFldSearch::activateFolder(KMFolder *curFolder)
{
    mChkbxSpecificFolders->setChecked(true);
    mCbxFolders->setFolder(curFolder);
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotSearch()
{
    mLastFocus = focusWidget();
    mBtnSearch->setFocus();	// set focus so we don't miss key event

    mStopped = false;
    mFetchingInProgress = 0;

    mSearchFolderOpenBtn->setEnabled(true);
    mBtnSearch->setEnabled(false);
    mBtnStop->setEnabled(true);

    mLbxMatches->clear();

    mSortColumn = mLbxMatches->sortColumn();
    mSortOrder = mLbxMatches->sortOrder();
    mLbxMatches->setSorting(-1);
    mLbxMatches->setShowSortIndicator(false);

    // If we haven't openend an existing search folder, find or
    // create one.
    if (!mFolder) {
      KMFolderMgr *mgr = kmkernel->searchFolderMgr();
      if (mSearchFolderEdt->text().isEmpty())
	  mSearchFolderEdt->setText(i18n("Last Search"));
      QString baseName = mSearchFolderEdt->text();
      QString fullName = baseName;
      int count = 0;
      KMFolder *folder;
      while ((folder = mgr->find(fullName))) {
        if (folder->storage()->inherits("KMFolderSearch"))
          break;
        fullName = QString("%1 %2").arg(baseName).arg(++count);
      }

      if (!folder)
        folder = mgr->createFolder(fullName, FALSE, KMFolderTypeSearch,
            &mgr->dir());

      mFolder = dynamic_cast<KMFolderSearch*>( folder->storage() );
    }
    mFolder->stopSearch();
    disconnect(mFolder, SIGNAL(msgAdded(int)),
	    this, SLOT(slotAddMsg(int)));
    disconnect(mFolder, SIGNAL(msgRemoved(KMFolder*, Q_UINT32)),
	    this, SLOT(slotRemoveMsg(KMFolder*, Q_UINT32)));
    connect(mFolder, SIGNAL(msgAdded(int)),
	    this, SLOT(slotAddMsg(int)));
    connect(mFolder, SIGNAL(msgRemoved(KMFolder*, Q_UINT32)),
	    this, SLOT(slotRemoveMsg(KMFolder*, Q_UINT32)));
    KMSearch *search = new KMSearch();
    connect(search, SIGNAL(finished(bool)),
	    this, SLOT(searchDone()));
    if (mChkbxAllFolders->isChecked()) {
	search->setRecursive(true);
    } else {
	search->setRoot(mCbxFolders->folder());
	search->setRecursive(mChkSubFolders->isChecked());
    }

    mPatternEdit->updateSearchPattern();
    KMSearchPattern *searchPattern = new KMSearchPattern();
    *searchPattern = *mSearchPattern; //deep copy
    searchPattern->purify();
    search->setSearchPattern(searchPattern);
    mFolder->setSearch(search);
    enableGUI();

    if (mFolder && !mFolders.contains(mFolder.operator->()->folder())) {
	mFolder->open();
	mFolders.append(mFolder.operator->()->folder());
    }
    mTimer->start(200);
}

//-----------------------------------------------------------------------------
void KMFldSearch::searchDone()
{
    mTimer->stop();
    updStatus();

    QTimer::singleShot(0, this, SLOT(enableGUI()));
    if(mLastFocus)
	mLastFocus->setFocus();
    if (mCloseRequested)
	close();

    mLbxMatches->setSorting(mSortColumn, mSortOrder == Ascending);
    mLbxMatches->setShowSortIndicator(true);
}

void KMFldSearch::slotAddMsg(int idx)
{
    if (!mFolder)
	return;
    bool unget = !mFolder->isMessage(idx);
    KMMessage *msg = mFolder->getMsg(idx);
    QString from, fName;
    KMFolder *pFolder = msg->parent();
    if (!mFolders.contains(pFolder)) {
	mFolders.append(pFolder);
	pFolder->open();
    }
    if(pFolder->whoField() == "To")
        from = msg->to();
    else
        from = msg->from();
    if (pFolder->isSystemFolder())
	fName = i18n(pFolder->name().utf8());
    else
        fName = pFolder->name();

    (void)new KListViewItem(mLbxMatches, mLbxMatches->lastItem(),
			    msg->subject(), from, msg->dateIsoStr(),
			    fName,
			    QString::number(mFolder->serNum(idx)));
    if (unget)
	mFolder->unGetMsg(idx);
}

void KMFldSearch::slotRemoveMsg(KMFolder *, Q_UINT32 serNum)
{
    if (!mFolder)
	return;
    QListViewItemIterator it(mLbxMatches);
    while (it.current()) {
	QListViewItem *item = *it;
	if (serNum == (*it)->text(MSGID_COLUMN).toUInt()) {
	    delete item;
	    return;
	}
	++it;
    }
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotStop()
{
    if (mFolder)
	mFolder->stopSearch();
    mStopped = true;
    mBtnStop->setEnabled(false);
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotClose()
{
    accept();
}


//-----------------------------------------------------------------------------
void KMFldSearch::closeEvent(QCloseEvent *e)
{
    if (mFolder && mFolder->search() && mFolder->search()->running()) {
	mCloseRequested = true;
	//Cancel search in progress by setting the search folder search to
	//the null search
	mFolder->setSearch(new KMSearch());
	QTimer::singleShot(0, this, SLOT(slotClose()));
    } else {
	KDialogBase::closeEvent(e);
    }
}

//-----------------------------------------------------------------------------
void KMFldSearch::updateCreateButton( const QString &s)
{
    mSearchFolderBtn->setEnabled(s != i18n("Last Search") && mSearchFolderOpenBtn->isEnabled());
}

//-----------------------------------------------------------------------------
void KMFldSearch::renameSearchFolder()
{
    if (mFolder && (mFolder->folder()->name() != mSearchFolderEdt->text())) {
	int i = 1;
	QString name =  mSearchFolderEdt->text();
	while (i < 100) {
	    if (!kmkernel->searchFolderMgr()->find( name )) {
		mFolder->rename( name );
		kmkernel->searchFolderMgr()->contentsChanged();
		break;
	    }
	    name.setNum( i );
	    name = mSearchFolderEdt->text() + " " + name;
	    ++i;
	}
    }
}

void KMFldSearch::openSearchFolder()
{
    renameSearchFolder();
    KMFolderTree *folderTree = mKMMainWidget->folderTree();
    QListViewItem *index = folderTree->indexOfFolder(mFolder->folder());
    if (index) {
	folderTree->ensureItemVisible(index);
	folderTree->doFolderSelected(index);
	slotClose();
    }
}

//-----------------------------------------------------------------------------
void KMFldSearch::folderInvalidated(KMFolder *folder)
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
bool KMFldSearch::slotShowMsg(QListViewItem *item)
{
    if(!item)
	return false;

    KMFolder* folder;
    int msgIndex;
    kmkernel->msgDict()->getLocation(item->text(MSGID_COLUMN).toUInt(),
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
void KMFldSearch::enableGUI()
{
    KMSearch const *search = (mFolder) ? (mFolder->search()) : 0;
    bool searching = (search) ? (search->running()) : false;
    actionButton(KDialogBase::Close)->setEnabled(!searching);
    mCbxFolders->setEnabled(!searching);
    mChkSubFolders->setEnabled(!searching);
    mChkbxAllFolders->setEnabled(!searching);
    mChkbxSpecificFolders->setEnabled(!searching);
    mPatternEdit->setEnabled(!searching);
    mBtnSearch->setEnabled(!searching);
    mBtnStop->setEnabled(searching);
}


//-----------------------------------------------------------------------------
KMMessageList KMFldSearch::selectedMessages()
{
    KMMessageList msgList;
    KMFolder* folder = 0;
    int msgIndex = -1;
    for (QListViewItemIterator it(mLbxMatches); it.current(); it++)
	if (it.current()->isSelected()) {
	    kmkernel->msgDict()->getLocation((*it)->text(MSGID_COLUMN).toUInt(),
					   &folder, &msgIndex);
	    if (folder && msgIndex >= 0)
		msgList.append(folder->getMsgBase(msgIndex));
	}
    return msgList;
}

//-----------------------------------------------------------------------------
KMMessage* KMFldSearch::message()
{
    QListViewItem *item = mLbxMatches->currentItem();
    KMFolder* folder = 0;
    int msgIndex = -1;
    if (!item)
	return 0;
    kmkernel->msgDict()->getLocation(item->text(MSGID_COLUMN).toUInt(),
				   &folder, &msgIndex);
    if (!folder || msgIndex < 0)
	return 0;

    return folder->getMsg(msgIndex);
}

//-----------------------------------------------------------------------------
void KMFldSearch::moveSelectedToFolder( int menuId )
{
    KMFolder *dest = mMenuToFolder[menuId];
    if (!dest)
	return;

    KMMessageList msgList = selectedMessages();
    KMCommand *command = new KMMoveCommand( dest, msgList );
    command->start();
}

//-----------------------------------------------------------------------------
void KMFldSearch::copySelectedToFolder( int menuId )
{
    KMFolder *dest = mMenuToFolder[menuId];
    if (!dest)
	return;

    KMMessageList msgList = selectedMessages();
    KMCommand *command = new KMCopyCommand( dest, msgList );
    command->start();
}

//-----------------------------------------------------------------------------
void KMFldSearch::updateContextMenuActions()
{
    int count = selectedMessages().count();
    bool single_actions = count == 1;
    mReplyAction->setEnabled( single_actions );
    mReplyAllAction->setEnabled( single_actions );
    mReplyListAction->setEnabled( single_actions );
    mPrintAction->setEnabled( single_actions );
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotContextMenuRequested( QListViewItem *lvi, const QPoint &, int )
{
    if (!lvi)
	return;
    mLbxMatches->setSelected( lvi, TRUE );
    mLbxMatches->setCurrentItem( lvi );
    if (!message())
	return;
    QPopupMenu *menu = new QPopupMenu(this);
    updateContextMenuActions();

    mMenuToFolder.clear();
    QPopupMenu *msgMoveMenu = new QPopupMenu(menu);
    mKMMainWidget->folderTree()->folderToPopupMenu( TRUE, this, &mMenuToFolder, msgMoveMenu );
    QPopupMenu *msgCopyMenu = new QPopupMenu(menu);
    mKMMainWidget->folderTree()->folderToPopupMenu( FALSE, this, &mMenuToFolder, msgCopyMenu );

    // show most used actions
    mReplyAction->plug(menu);
    mReplyAllAction->plug(menu);
    mReplyListAction->plug(menu);
    mForwardActionMenu->plug(menu);
    menu->insertSeparator();
    menu->insertItem(i18n("&Copy To"), msgCopyMenu);
    menu->insertItem(i18n("&Move To"), msgMoveMenu);
    mSaveAsAction->plug(menu);
    mSaveAtchAction->plug(menu);
    mPrintAction->plug(menu);
    menu->insertSeparator();
    mClearAction->plug(menu);
    menu->exec (QCursor::pos(), 0);
    delete menu;
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotClearSelection()
{
    mLbxMatches->clearSelection();
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotReplyToMsg()
{
    KMCommand *command = new KMReplyToCommand(this, message());
    command->start();
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotReplyAllToMsg()
{
    KMCommand *command = new KMReplyToAllCommand(this, message());
    command->start();
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotReplyListToMsg()
{
    KMCommand *command = new KMReplyListCommand(this, message());
    command->start();
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotForwardMsg()
{
    KMCommand *command = new KMForwardCommand(this, selectedMessages());
    command->start();
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotForwardAttachedMsg()
{
    KMCommand *command = new KMForwardAttachedCommand(this, selectedMessages());
    command->start();
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotSaveMsg()
{
    KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand(this,
							 selectedMessages());
    if (saveCommand->url().isEmpty())
	delete saveCommand;
    else
	saveCommand->start();
}
//-----------------------------------------------------------------------------
void KMFldSearch::slotSaveAttachments()
{
    KMSaveAttachmentsCommand *saveCommand = new KMSaveAttachmentsCommand(this,
                                                                         selectedMessages());
    saveCommand->start();
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotPrintMsg()
{
    KMCommand *command = new KMPrintCommand(this, message());
    command->start();
}

#include "kmfldsearch.moc"
