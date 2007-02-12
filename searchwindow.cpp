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
#include <config.h>
#include "kmcommands.h"
#include "searchwindow.h"
#include "kmmainwidget.h"
#include "kmmsgdict.h"
#include "kmmsgpart.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmfoldersearch.h"
#include "kmfoldertree.h"
#include "kmsearchpatternedit.h"
#include "kmsearchpattern.h"
#include "folderrequester.h"
#include "regexplineedit.h"

#include <kactionmenu.h>
#include <kdebug.h>
#include <kstatusbar.h>
#include <kwin.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kstandardaction.h>
#include <kpushbutton.h>
#include <kicon.h>
#include <QCheckBox>
#include <QLayout>
//Added by qt3to4:
#include <QCloseEvent>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <klineedit.h>
#include <QPushButton>
#include <QRadioButton>
#include <q3buttongroup.h>
#include <QComboBox>
#include <QObject> //for mPatternEdit->queryList( 0, "mRuleField" )->first();
#include <QCursor>
#include <q3popupmenu.h>

#include <mimelib/enum.h>
#include <mimelib/boyermor.h>

#include <assert.h>
#include <stdlib.h>
#include <kiconloader.h>

namespace KMail {

const int SearchWindow::MSGID_COLUMN = 4;

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
  setButtonGuiItem( User1, KGuiItem( i18n("&Search"), "find" ) );
  setButtonGuiItem( User2, KStandardGuiItem::stop() );
#ifdef Q_OS_UNIX
  KWin::setIcons(winId(), qApp->windowIcon().pixmap(IconSize(K3Icon::Desktop),IconSize(K3Icon::Desktop)), qApp->windowIcon().pixmap(IconSize(K3Icon::Small),IconSize(K3Icon::Small)));
#endif

  KConfig* config = KMKernel::config();
  KConfigGroup group( config, "SearchDialog" );

  QWidget* searchWidget = new QWidget(this);
  QVBoxLayout *vbl = new QVBoxLayout( searchWidget );
  vbl->setObjectName( "kmfs_vbl" );
  vbl->setSpacing( spacingHint() );
  vbl->setMargin( 0 );

  Q3ButtonGroup * radioGroup = new Q3ButtonGroup( searchWidget );
  radioGroup->hide();

  mChkbxAllFolders = new QRadioButton(i18n("Search in &all local folders"), searchWidget);
  vbl->addWidget( mChkbxAllFolders );
  radioGroup->insert( mChkbxAllFolders );

  QHBoxLayout *hbl = new QHBoxLayout();
  vbl->addLayout( hbl );
  hbl->setObjectName( "kmfs_hbl" );
  hbl->setSpacing( spacingHint() );
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

  QWidget *spacer = new QWidget( searchWidget );
  spacer->setObjectName( "spacer" );
  spacer->setMinimumHeight( 2 );
  vbl->addWidget( spacer );

  mPatternEdit = new KMSearchPatternEdit( "", searchWidget , false, true );
  mPatternEdit->setFlat( true );
  mPatternEdit->setContentsMargins( 0, 0, 0, 0 );
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
  mPatternEdit->setSearchPattern( mSearchPattern );
  QObjectList list = mPatternEdit->queryList( 0, "mRuleField" );
  QObject *object = 0;
  if ( !list.isEmpty() )
      object = list.first();
  if (!searchFolder && object && ::qobject_cast<QComboBox*>(object)) {
      QComboBox *box = static_cast<QComboBox*>(object);
      box->setItemText( box->currentIndex(), "Subject" );
  }

  vbl->addWidget( mPatternEdit );

  // enable/disable widgets depending on radio buttons:
  connect( mChkbxSpecificFolders, SIGNAL(toggled(bool)),
           mCbxFolders, SLOT(setEnabled(bool)) );
  connect( mChkbxSpecificFolders, SIGNAL(toggled(bool)),
           mChkSubFolders, SLOT(setEnabled(bool)) );
  connect( mChkbxAllFolders, SIGNAL(toggled(bool)),
           this, SLOT(setEnabledSearchButton(bool)) );

  mLbxMatches = new K3ListView( searchWidget );
  mLbxMatches->setObjectName( "Find Messages" );

  /*
     Default is to sort by date. TODO: Unfortunately this sorts *while*
     inserting, which looks rather strange - the user cannot read
     the results so far as they are constantly re-sorted --dnaber

     Sorting is now disabled when a search is started and reenabled
     when it stops. Items are appended to the list. This not only
     solves the above problem, but speeds searches with many hits
     up considerably. - till

     TODO: subclass K3ListViewItem and do proper (and performant)
     comapare functions
  */
  mLbxMatches->setSorting(2, false);
  mLbxMatches->setShowSortIndicator(true);
  mLbxMatches->setAllColumnsShowFocus(true);
  mLbxMatches->setSelectionModeExt(K3ListView::Extended);
  mLbxMatches->addColumn(i18n("Subject"),
      group.readEntry( "SubjectWidth", 150 ) );
  mLbxMatches->addColumn(i18n("Sender/Receiver"),
      group.readEntry( "SenderWidth", 120 ) );
  mLbxMatches->addColumn(i18n("Date"),
      group.readEntry( "DateWidth", 120 ) );
  mLbxMatches->addColumn(i18n("Folder"),
      group.readEntry( "FolderWidth", 100 ) );

  mLbxMatches->addColumn(""); // should be hidden
  mLbxMatches->setColumnWidthMode( MSGID_COLUMN, Q3ListView::Manual );
  mLbxMatches->setColumnWidth(MSGID_COLUMN, 0);
  mLbxMatches->header()->setResizeEnabled(false, MSGID_COLUMN);

  connect(mLbxMatches, SIGNAL(doubleClicked(Q3ListViewItem *)),
          this, SLOT(slotShowMsg(Q3ListViewItem *)));
  connect( mLbxMatches, SIGNAL( contextMenuRequested( Q3ListViewItem*, const QPoint &, int )),
           this, SLOT( slotContextMenuRequested( Q3ListViewItem*, const QPoint &, int )));
  vbl->addWidget(mLbxMatches);

  QHBoxLayout *hbl2 = new QHBoxLayout();
  vbl->addLayout( hbl2 );
  hbl2->setObjectName( "kmfs_hbl2" );
  hbl2->setSpacing( spacingHint() );
  mSearchFolderLbl = new QLabel(i18n("Search folder &name:"),searchWidget);
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
  mStatusBar->insertPermanentItem(i18n("AMiddleLengthText..."), 0);
  mStatusBar->changeItem(i18n("Ready."), 0);
  mStatusBar->setItemAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
  mStatusBar->insertPermanentItem(QString(), 1, 1);
  mStatusBar->setItemAlignment(1, Qt::AlignLeft | Qt::AlignVCenter);
  vbl->addWidget(mStatusBar);

  int mainWidth = group.readEntry("SearchWidgetWidth", 0 );
  int mainHeight = group.readEntry("SearchWidgetHeight", 0 );

  if (mainWidth || mainHeight)
    resize(mainWidth, mainHeight);

  setMainWidget(searchWidget);
  setButtonsOrientation(Qt::Vertical);

  enableButton(User2, false);

  connect(this, SIGNAL(user1Clicked()), SLOT(slotSearch()));
  connect(this, SIGNAL(user2Clicked()), SLOT(slotStop()));
  connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
  connect(this,SIGNAL(closeClicked()),this,SLOT(slotClose()));
  // give focus to the value field of the first search rule
  RegExpLineEdit* r = mPatternEdit->findChild<RegExpLineEdit*>( "regExpLineEdit" );
  if ( r )
      r->setFocus();
  else
      kDebug(5006) << "SearchWindow: regExpLineEdit not found" << endl;

  //set up actions
  KActionCollection *ac = actionCollection();
  mReplyAction  = new KAction(KIcon("mail_reply"), i18n("&Reply..."), this);
  actionCollection()->addAction("search_reply", mReplyAction );
  connect(mReplyAction, SIGNAL(triggered(bool)), SLOT(slotReplyToMsg()));
  mReplyAllAction  = new KAction(KIcon("mail_replyall"), i18n("Reply to &All..."), this);
  actionCollection()->addAction("search_reply_all", mReplyAllAction );
  connect(mReplyAllAction, SIGNAL(triggered(bool) ), SLOT(slotReplyAllToMsg()));
  mReplyListAction  = new KAction(KIcon("mail_replylist"), i18n("Reply to Mailing-&List..."), this);
  actionCollection()->addAction("search_reply_list", mReplyListAction );
  connect(mReplyListAction, SIGNAL(triggered(bool) ), SLOT(slotReplyListToMsg()));
  mForwardActionMenu  = new KActionMenu(KIcon("mail_forward"), i18nc("Message->","&Forward"), this);
  actionCollection()->addAction("search_message_forward", mForwardActionMenu );
  connect( mForwardActionMenu, SIGNAL(activated()), this,
           SLOT(slotForwardMsg()) );
  mForwardAction  = new KAction(KIcon("mail_forward"), i18n("&Inline..."), this);
  actionCollection()->addAction("search_message_forward_inline", mForwardAction );
  connect(mForwardAction, SIGNAL(triggered(bool) ), SLOT(slotForwardMsg()));
  mForwardActionMenu->addAction( mForwardAction );
  mForwardAttachedAction  = new KAction(KIcon("mail_forward"), i18nc("Message->Forward->","As &Attachment..."), this);
  actionCollection()->addAction("search_message_forward_as_attachment", mForwardAttachedAction );
  connect(mForwardAttachedAction, SIGNAL(triggered(bool)), SLOT(slotForwardAttachedMsg()));
  mForwardActionMenu->addAction( mForwardAttachedAction );
  mSaveAsAction = actionCollection()->addAction(KStandardAction::SaveAs,  "search_file_save_as", this, SLOT(slotSaveMsg()));
  mSaveAtchAction  = new KAction(KIcon("attach"), i18n("Save Attachments..."), this);
  actionCollection()->addAction("search_save_attachments", mSaveAtchAction );
  connect(mSaveAtchAction, SIGNAL(triggered(bool)), SLOT(slotSaveAttachments()));

  mPrintAction = actionCollection()->addAction(KStandardAction::Print,  "search_print", this, SLOT(slotPrintMsg()));
  mClearAction  = new KAction(i18n("Clear Selection"), this);
  actionCollection()->addAction("search_clear_selection", mClearAction );
  connect(mClearAction, SIGNAL(triggered(bool)), SLOT(slotClearSelection()));
  connect(mTimer, SIGNAL(timeout()), this, SLOT(updStatus()));
  connect(kmkernel->searchFolderMgr(), SIGNAL(folderInvalidated(KMFolder*)),
          this, SLOT(folderInvalidated(KMFolder*)));

  connect(mCbxFolders, SIGNAL(folderChanged(KMFolder*)),
          this, SLOT(slotFolderActivated()));

}

//-----------------------------------------------------------------------------
SearchWindow::~SearchWindow()
{
  QList<QPointer<KMFolder> >::Iterator fit;
  for ( fit = mFolders.begin(); fit != mFolders.end(); ++fit ) {
    if (!(*fit))
      continue;
    (*fit)->close();
  }

  KConfig* config = KMKernel::config();
  KConfigGroup group( config, "SearchDialog" );
  group.writeEntry("SubjectWidth", mLbxMatches->columnWidth(0));
  group.writeEntry("SenderWidth", mLbxMatches->columnWidth(1));
  group.writeEntry("DateWidth", mLbxMatches->columnWidth(2));
  group.writeEntry("FolderWidth", mLbxMatches->columnWidth(3));
  group.writeEntry("SearchWidgetWidth", width());
  group.writeEntry("SearchWidgetHeight", height());
  config->sync();
}

void SearchWindow::setEnabledSearchButton(bool)
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
        folderName = search->currentFolder();
    }

    if (mFolder && mFolder->search() && !mFolder->search()->running()) {
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

    mSearchFolderOpenBtn->setEnabled(true);
    enableButton(User1, false);
    enableButton(User2, true);

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
        folder = mgr->createFolder(fullName, false, KMFolderTypeSearch,
            &mgr->dir());

      mFolder = dynamic_cast<KMFolderSearch*>( folder->storage() );
    }
    mFolder->stopSearch();
    disconnect(mFolder, SIGNAL(msgAdded(int)),
            this, SLOT(slotAddMsg(int)));
    disconnect(mFolder, SIGNAL(msgRemoved(KMFolder*, quint32)),
            this, SLOT(slotRemoveMsg(KMFolder*, quint32)));
    connect(mFolder, SIGNAL(msgAdded(int)),
            this, SLOT(slotAddMsg(int)));
    connect(mFolder, SIGNAL(msgRemoved(KMFolder*, quint32)),
            this, SLOT(slotRemoveMsg(KMFolder*, quint32)));
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
void SearchWindow::searchDone()
{
    mTimer->stop();
    updStatus();

    QTimer::singleShot(0, this, SLOT(enableGUI()));
    if(mLastFocus)
        mLastFocus->setFocus();
    if (mCloseRequested)
        close();

    mLbxMatches->setSorting(mSortColumn, mSortOrder == Qt::Ascending);
    mLbxMatches->setShowSortIndicator(true);
}

void SearchWindow::slotAddMsg(int idx)
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
        fName = i18n(pFolder->name().toUtf8());
    else
        fName = pFolder->name();

    (void)new K3ListViewItem(mLbxMatches, mLbxMatches->lastItem(),
                            msg->subject(), from, msg->dateIsoStr(),
                            fName,
                            QString::number(mFolder->serNum(idx)));
    if (unget)
        mFolder->unGetMsg(idx);
}

void SearchWindow::slotRemoveMsg(KMFolder *, quint32 serNum)
{
    if (!mFolder)
        return;
    Q3ListViewItemIterator it(mLbxMatches);
    while (it.current()) {
        Q3ListViewItem *item = *it;
        if (serNum == (*it)->text(MSGID_COLUMN).toUInt()) {
            delete item;
            return;
        }
        ++it;
    }
}

//-----------------------------------------------------------------------------
void SearchWindow::slotStop()
{
    if (mFolder)
      mFolder->stopSearch();
    mStopped = true;
    enableButton(User2, false);
}

//-----------------------------------------------------------------------------
void SearchWindow::slotClose()
{
    accept();
}


//-----------------------------------------------------------------------------
void SearchWindow::closeEvent(QCloseEvent *e)
{
    if (mFolder && mFolder->search() && mFolder->search()->running()) {
      mCloseRequested = true;
      //Cancel search in progress by setting the search folder search to
      //the null search
      mFolder->setSearch(new KMSearch());
      QTimer::singleShot(0, this, SLOT(slotClose()));
    } else {
      KDialog::closeEvent(e);
    }
}

//-----------------------------------------------------------------------------
void SearchWindow::updateCreateButton( const QString &s)
{
    mSearchFolderBtn->setEnabled(s != i18n("Last Search") && mSearchFolderOpenBtn->isEnabled());
}

//-----------------------------------------------------------------------------
void SearchWindow::renameSearchFolder()
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
            name = mSearchFolderEdt->text() + ' ' + name;
            ++i;
        }
    }
}

void SearchWindow::openSearchFolder()
{
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
bool SearchWindow::slotShowMsg(Q3ListViewItem *item)
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
    for (Q3ListViewItemIterator it(mLbxMatches); it.current(); it++)
        if (it.current()->isSelected()) {
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
    Q3ListViewItem *item = mLbxMatches->currentItem();
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
}

//-----------------------------------------------------------------------------
void SearchWindow::slotContextMenuRequested( Q3ListViewItem *lvi, const QPoint &, int )
{
    if (!lvi)
        return;
    mLbxMatches->setSelected( lvi, true );
    mLbxMatches->setCurrentItem( lvi );
    // FIXME is this ever unGetMsg()'d?
    if (!message())
        return;
    Q3PopupMenu *menu = new Q3PopupMenu(this);
    updateContextMenuActions();

    mMenuToFolder.clear();
    Q3PopupMenu *msgMoveMenu = new Q3PopupMenu(menu);
    mKMMainWidget->folderTree()->folderToPopupMenu( KMFolderTree::MoveMessage,
        this, &mMenuToFolder, msgMoveMenu );
    Q3PopupMenu *msgCopyMenu = new Q3PopupMenu(menu);
    mKMMainWidget->folderTree()->folderToPopupMenu( KMFolderTree::CopyMessage,
        this, &mMenuToFolder, msgCopyMenu );

    // show most used actions
    menu->addAction( mReplyAction );
    menu->addAction( mReplyAllAction );
    menu->addAction( mReplyListAction );
    menu->addAction( mForwardActionMenu );
    menu->addSeparator();
    msgCopyMenu->setTitle(i18n("&Copy To"));
    menu->addMenu( msgCopyMenu );
    msgMoveMenu->setTitle(i18n("&Move To"));
    menu->addMenu( msgMoveMenu );
    menu->addAction( mSaveAsAction );
    menu->addAction( mSaveAtchAction );
    menu->addAction( mPrintAction );
    menu->addSeparator();
    menu->addAction( mClearAction );
    menu->exec (QCursor::pos(), 0);
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

} // namespace KMail
#include "searchwindow.moc"
