/* kmfldsearch
 * (c) 1999 Stefan Taferner, (c) 2001 Aaron J. Seigo
 * This code is under GPL
 */
// kmfldsearch.cpp

#include <config.h>
#include "kbusyptr.h"
#include "kmcomposewin.h"
#include "kmfldsearch.h"
#include "kmmainwin.h"
#include "kmmsgdict.h"
#include "kmmsgpart.h"
#include "kmfiltermgr.h"
#include "kmfoldercombobox.h"
#include "kmfolderdir.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmfoldertree.h"
#include "kmsender.h"
#include "kmundostack.h"
#include "mailinglist-magic.h"

#include <assert.h>
#include <qcheckbox.h>
#include <qcursor.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qregexp.h>
#include <qtextcodec.h>

#include <kaction.h>
#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <kwin.h>

#include <mimelib/enum.h>

#include <stdlib.h>

const int KMFldSearch::MSGID_COLUMN = 4;

//-----------------------------------------------------------------------------
KMFldSearch::KMFldSearch(KMMainWin* w, const char* name,
                         KMFolder *curFolder, bool modal):
  KDialogBase(NULL, name, modal, i18n("Search in Folders"),
              User1 | User2 | Close, User1, false,
              KGuiItem( i18n("&Search"), "find" ),
              KGuiItem( i18n("S&top"), "cancel" )),
  mSearching(false),
  mStopped(false),
  mCloseRequested(false),
  mNumMatches(0),
  mCount(0),
  mLastFocus(0),
  mMainWin(w)
{
  int numRules = 2;
  KWin::setIcons(winId(), kapp->icon(), kapp->miniIcon());

  KConfig* config = KGlobal::config();
  config->setGroup("SearchDialog");

  QWidget* searchWidget = new QWidget(this);
  mGrid = new QGridLayout(searchWidget, numRules+5, 5, 0, spacingHint());

  QButtonGroup * radioGroup = new QButtonGroup( searchWidget );
  radioGroup->hide();

  mChkbxAllFolders = new QRadioButton(i18n("Search in &all local folders"), searchWidget);
  mGrid->addMultiCellWidget(mChkbxAllFolders, 0, 0, 0, 3);
  radioGroup->insert( mChkbxAllFolders );

  mChkbxSpecificFolders = new QRadioButton(i18n("Search &only in:"), searchWidget);
  mGrid->addWidget(mChkbxSpecificFolders, 1, 0);
  mChkbxSpecificFolders->setChecked(true);
  radioGroup->insert( mChkbxSpecificFolders );

  mCbxFolders = new KMFolderComboBox(false, searchWidget);
  mCbxFolders->setFolder(curFolder);
  mGrid->addMultiCellWidget(mCbxFolders, 1, 1, 1, 2);

  connect(mCbxFolders, SIGNAL(activated(int)),
          this, SLOT(slotFolderActivated(int)));

  for (int i=0; i<numRules; ++i)
  {
    KMFldSearchRule* rule = new KMFldSearchRule(searchWidget, mGrid, i+2, 0);
    rule->updateFunctions(mCbxFolders->getFolder());
    mRules.append(rule);
  }

  mChkSubFolders = new QCheckBox(i18n("I&nclude sub-folders"), searchWidget);
  mChkSubFolders->setChecked(true);
  mGrid->addWidget(mChkSubFolders, 1, 3);

  // enable/disable widgets depending on radio buttons:
  connect( mChkbxSpecificFolders, SIGNAL(toggled(bool)),
	   mCbxFolders, SLOT(setEnabled(bool)) );
  connect( mChkbxSpecificFolders, SIGNAL(toggled(bool)),
	   mChkSubFolders, SLOT(setEnabled(bool)) );

  mLbxMatches = new QListView(searchWidget, "Search in Folders");
  /* Default is to sort by date. TODO: Unfortunately this sorts *while*
     inserting, which looks rather strange - the user cannot read
     the results so far as they are constantly re-sorted --dnaber
  */
  mLbxMatches->setSorting(2, false);
  mLbxMatches->setShowSortIndicator(true);
  mLbxMatches->setAllColumnsShowFocus(true);
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
  mGrid->addMultiCellWidget(mLbxMatches, numRules+2, numRules+2, 0, 3);

  mStatusBar = new KStatusBar(searchWidget);
  mStatusBar->insertFixedItem(i18n("AMiddleLengthText..."), 0, true);
  mStatusBar->changeItem(i18n("Ready."), 0);
  mStatusBar->setItemAlignment(0, AlignLeft | AlignVCenter);
  mStatusBar->insertItem(QString::null, 1, 1, true);
  mStatusBar->setItemAlignment(1, AlignLeft | AlignVCenter);
  mGrid->addMultiCellWidget(mStatusBar, numRules+4, numRules+4, 0, 3);

  mGrid->setColStretch(0, 0);
  mGrid->setColStretch(1, 2);
  mGrid->setColStretch(2, 1);
  mGrid->setColStretch(3, 100);
  mGrid->setColStretch(4, 0);

  mGrid->setRowStretch(numRules+2, 100);
  mGrid->setRowStretch(numRules+3, 0);

  mGrid->activate();
  mRules.at(0)->setFocus();

  int mainWidth = config->readNumEntry("SearchWidgetWidth", 0);
  int mainHeight = config->readNumEntry("SearchWidgetHeight", 0);

  if (mainWidth || mainHeight)
  {
    resize(mainWidth, mainHeight);
  }

  setMainWidget(searchWidget);
  setButtonBoxOrientation(QWidget::Vertical);

  mBtnSearch = actionButton(KDialogBase::User1);
  mBtnStop = actionButton(KDialogBase::User2);
  mBtnStop->setEnabled(false);

  connect(this, SIGNAL(user1Clicked()), SLOT(slotSearch()));
  connect(this, SIGNAL(user2Clicked()), SLOT(slotStop()));
  connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

//-----------------------------------------------------------------------------
KMFldSearch::~KMFldSearch()
{
  KConfig* config = KGlobal::config();
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

  if (!mSearching)
  {
    if(!mStopped)
    {
      genMsg = i18n("Done");
      detailMsg = i18n("%n match (%1)", "%n matches (%1)", mNumMatches)
                 .arg(i18n("%n message processed",
                           "%n messages processed", mCount));
    }
    else
    {
      genMsg = i18n("Search canceled");
      detailMsg = i18n("%n match so far (%1)",
                       "%n matches so far (%1)", mNumMatches)
                 .arg(i18n("%n message processed",
                           "%n messages processed", mCount));
    }
  }
  else
  {
    genMsg = i18n("%n match", "%n matches", mNumMatches);
    detailMsg = i18n("Searching in %1 (message %2)")
               .arg(mSearchFolder)
               .arg(mCount);
  }

  mStatusBar->changeItem(genMsg, 0);
  mStatusBar->changeItem(detailMsg, 1);
}


//-----------------------------------------------------------------------------
void KMFldSearch::keyPressEvent(QKeyEvent *evt)
{
  if (evt->key() == Key_Escape && mSearching)
  {
    mSearching = false;
    return;
  }

  KDialogBase::keyPressEvent(evt);
}


//-----------------------------------------------------------------------------
bool KMFldSearch::searchInMessage(KMMessage* aMsg, const QCString& aMsgStr)
{
  bool matches = true;

  mCount++;
  if (mCount % 100 == 0) updStatus();
  for(KMFldSearchRuleIt it(mRules); *it && matches; ++it)
  {
    if (!(*it)->matches(aMsg, aMsgStr))
      matches = false;
  }

  return matches;
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotFolderComplete(KMFolderImap *folder, bool success)
{
  disconnect(folder, SIGNAL(folderComplete(KMFolderImap*, bool)),
             this, SLOT(slotFolderComplete(KMFolderImap*, bool)));
  mFetchingInProgress--;

  if (success)
  {
    searchInFolder(folder, mChkSubFolders->isChecked(), FALSE);
    if (mFetchingInProgress == 0) searchDone();
  }
  else
  {
    searchDone();
  }

  folder->close();
}


//-----------------------------------------------------------------------------
void KMFldSearch::searchInFolder(QGuardedPtr<KMFolder> aFld, bool recursive,
  bool fetchHeaders)
{
  //kdDebug(5006) << "Searching folder '" << aFld->name() << "'" << endl;
  if (fetchHeaders && aFld->protocol() == "imap")
  {
    KMFolder *fld = aFld;
    KMFolderImap *imap_folder = static_cast<KMFolderImap*>(fld);
    if (imap_folder &&
        imap_folder->getContentState() == KMFolderImap::imapNoInformation)
    {
      mFetchingInProgress++;
      imap_folder->open();
      connect(imap_folder,
              SIGNAL(folderComplete(KMFolderImap *, bool)),
              SLOT(slotFolderComplete(KMFolderImap *, bool)));
      imap_folder->getFolder();
      return;
    }
  }

  KMMessage* msg=0;
  int i, num, upd;
  QString str;
  QCString cStr;
  bool unget = false, fastMode = true, found;

  assert(!aFld.isNull());

  if (aFld->isSystemFolder())
    mSearchFolder = i18n(aFld->name().utf8());
  else
    mSearchFolder = aFld->name();

  kapp->processEvents();
  if (aFld->open() != 0)
  {
    kdDebug(5006) << "Cannot open folder '" << aFld->name() << "'" << endl;
    return;
  }

  for(KMFldSearchRuleIt it(mRules); *it; ++it)
  {
    if (!(*it)->isHeaderField())
    {
      fastMode = false;
      break;
    }
  }

  num = aFld->count();
  for (i=0, upd=0; i<num && mSearching; i++, upd++)
  {
    if (fastMode)
    {
      found = searchInMessage(NULL, aFld->getMsgString(i, cStr));
      if (found)
      {
        unget = !aFld->isMessage(i);
        msg = aFld->getMsg(i);
      } else unget = false;
    }
    else
    {
      unget = !aFld->isMessage(i);
      msg = aFld->getMsg(i);
      found = msg && searchInMessage(msg, QCString());
    }
    if (found)
    {
      QString from;
      if(aFld->type() == KFolderTreeItem::SentMail)
        from = msg->to();
      else
        from = msg->from();

      (void)new QListViewItem(mLbxMatches,
                              msg->subject(), from, msg->dateIsoStr(),
                              mSearchFolder,
                              QString::number(kernel->msgDict()->getMsgSerNum(aFld, i)));
      mNumMatches++;
    }
    if (upd > 10)
    {
      kapp->processEvents();
      upd = 0;
      if (!aFld) // Folder deleted while searching!
	break;
      num = aFld->count();
    }
    if (unget) aFld->unGetMsg(i);
  }

  if(!mSearching)
    // was stopped
    mStopped = true;

  updStatus();

  aFld->close();

  if (!recursive)
    return;

  KMFolderDir *dir = aFld->child();
  if (dir)
  {
    KMFolder *folder;
    QPtrListIterator<KMFolderNode> it(*dir);
    for ( ; it.current(); ++it )
    {
      KMFolderNode *node = (*it);
      if (!node->isDir())
      {
        folder = static_cast<KMFolder*>(node);
        searchInFolder(folder, recursive);
      }
    }
  }
}


//-----------------------------------------------------------------------------
void KMFldSearch::searchInAllFolders(void)
{
  QValueList<QGuardedPtr<KMFolder> > folders;
  QGuardedPtr<KMFolder> folder;
  QStringList str;

  mMainWin->folderTree()->createFolderList( &str, &folders );
  QValueListConstIterator<QGuardedPtr<KMFolder> > it;
  for (it = folders.begin(); it != folders.end(); ++it)
  {
    folder = *it;
    // Stop pressed?
    if(!mSearching)
    {
      break;
    }

    if (folder && (folder->protocol() != "imap"))
    {
      searchInFolder(folder, false);
    }
  }
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotFolderActivated(int /*nr*/)
{
  KMFolder* folder = mCbxFolders->getFolder();

  mChkbxSpecificFolders->setChecked(true);
  mBtnSearch->setEnabled(folder);

  for(KMFldSearchRuleIt it(mRules); *it; ++it)
  {
    (*it)->updateFunctions(folder);
  }
}


//-----------------------------------------------------------------------------
void KMFldSearch::activateFolder(KMFolder *curFolder)
{
  mChkbxSpecificFolders->setChecked(true);
  mCbxFolders->setFolder(curFolder);
  mRules.at(0)->setFocus();
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotSearch()
{
  mLastFocus = focusWidget();
  mBtnSearch->setFocus();	// set focus so we don't miss key event

  mCount = 0;
  mStopped = false;
  mNumMatches = 0;
  mSearching = true;
  mFetchingInProgress = 0;

  mBtnSearch->setEnabled(false);
  mBtnStop->setEnabled(true);

  enableGUI();
  mLbxMatches->clear();
  kapp->processEvents();

  for(KMFldSearchRuleIt it(mRules); *it; ++it)
  {
    (*it)->prepare();
  }

  if (mChkbxAllFolders->isChecked())
  {
    searchInAllFolders();
  }
  else
  {
    KMFolder *folder = mCbxFolders->getFolder();

    if (!folder)
    {
      return;
    }

    searchInFolder(folder, mChkSubFolders->isChecked());
  }

  if (!mFetchingInProgress) searchDone();
}

//-----------------------------------------------------------------------------
void KMFldSearch::searchDone()
{
  mSearching=false;
  updStatus();

  mBtnSearch->setEnabled(true);
  mBtnStop->setEnabled(false);

  enableGUI();
  if( mLastFocus )
    mLastFocus->setFocus();
  if (mCloseRequested) close();
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotStop()
{
  mSearching = false;
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
  if (mSearching)
  {
    mSearching = false;
    mCloseRequested = true;
    e->ignore();
  }
  else
  {
    KDialogBase::closeEvent(e);
  }
}

//-----------------------------------------------------------------------------
bool KMFldSearch::slotShowMsg(QListViewItem *item)
{
  if(!item)
  {
    return false;
  }

  KMFolder* folder;
  int msgIndex;
  kernel->msgDict()->getLocation(item->text(MSGID_COLUMN).toUInt(),
                                 &folder, &msgIndex);

  if (!folder || msgIndex < 0)
  {
    return false;
  }

  mMainWin->slotSelectFolder(folder);
  KMMessage* message = folder->getMsg(msgIndex);
  if (!message)
  {
    return false;
  }

  mMainWin->slotSelectMessage(message);
  return true;
}


//-----------------------------------------------------------------------------
void KMFldSearch::enableGUI()
{
  actionButton(KDialogBase::Close)->setEnabled(!mSearching);
  mCbxFolders->setEnabled(!mSearching);
  mChkSubFolders->setEnabled(!mSearching);
  mChkbxAllFolders->setEnabled(!mSearching);
  mChkbxSpecificFolders->setEnabled(!mSearching);

  for(KMFldSearchRuleIt it(mRules); *it; ++it)
  {
    (*it)->setEnabled(!mSearching);
  }
}


//=============================================================================
KMFldSearchRule::KMFldSearchRule(QWidget* aParent, QGridLayout* aGrid,
                                 int aRow, int aCol)
{
  assert(aParent!=NULL);
  assert(aGrid!=NULL);

  mRow = aRow;
  mCbxField = new KComboBox(true, aParent);
  insertFieldItems(false);
  mCbxField->setMinimumSize(mCbxField->sizeHint());
  mCbxField->setMaximumSize(1024, mCbxField->sizeHint().height());

  mCbxFunc = new KComboBox(false, aParent);
  mCbxFunc->insertItem(i18n("contains"));
  mCbxFunc->insertItem(i18n("doesn't contain"));
  mCbxFunc->insertItem(i18n("equals"));
  mCbxFunc->insertItem(i18n("doesn't equal"));
  mCbxFunc->insertItem(i18n("matches regular expr."));
  mCbxFunc->insertItem(i18n("doesn't match regular expr."));
  mCbxFunc->setMinimumSize(mCbxFunc->sizeHint());
  mCbxFunc->setMaximumSize(1024, mCbxFunc->sizeHint().height());

  mEdtValue = new KLineEdit(aParent);
  mEdtValue->setMinimumSize(mCbxFunc->sizeHint());
  mEdtValue->setMaximumSize(1024, mCbxFunc->sizeHint().height());

  QLabel* lbl;

  if (mRow > 2)
  {
    lbl = new QLabel(i18n("and"), aParent);
    lbl->setAlignment(QWidget::AlignRight|QWidget::AlignVCenter);
  }
  else
  {
    lbl = new QLabel(i18n("Show messages &where"), aParent);
    lbl->setBuddy(mCbxField);
    lbl->setAlignment(QWidget::AlignVCenter);
  }

  lbl->setMinimumSize(lbl->sizeHint());

  aGrid->addWidget(lbl, aRow, aCol);
  aGrid->addWidget(mCbxField, aRow, aCol + 1);
  aGrid->addWidget(mCbxFunc,  aRow, aCol + 2);
  aGrid->addWidget(mEdtValue, aRow, aCol + 3);
}


//-----------------------------------------------------------------------------
KMFldSearchRule::~KMFldSearchRule()
{
}


//-----------------------------------------------------------------------------
void KMFldSearchRule::insertFieldItems(bool imap)
{
  QString last = mCbxField->currentText();
  mCbxField->clear();
  if (!imap)
    if (mRow > 2) mCbxField->insertItem(QString::null);
  mCbxField->insertItem("Subject");
  mCbxField->insertItem("From");
  mCbxField->insertItem("To");
  mCbxField->insertItem("Cc");
  if (!imap) {
    mCbxField->insertItem("Organization");
    mCbxField->insertItem(i18n("<complete message>"));
  }

  if ( !last.isEmpty() ) {
    bool found = false;
    for ( int i = 0 ; i < mCbxField->count() ; ++ i )
      if ( mCbxField->text(i) == last ) {
	mCbxField->setCurrentItem(i);
	found = true;
      }
    if ( !found && !imap )
      mCbxField->setEditText( last );
  } else {
    mCbxField->setCurrentItem(0);
  }
}


//-----------------------------------------------------------------------------
void KMFldSearchRule::updateFunctions(KMFolder* folder)
{
  bool imap = folder && folder->protocol() == "imap";
  insertFieldItems( imap );
  mCbxField->setEditable( !imap );
}


//-----------------------------------------------------------------------------
void KMFldSearchRule::prepare(void)
{
  mFieldIdx = mCbxField->currentItem();
  mField = mCbxField->currentText();
  mHeaderField = QString("\n" + mField + ": ").latin1();
  mFieldLength = mField.length() + 3;
  mFunc = mCbxFunc->currentItem();
  mValue = mEdtValue->text();
  mNonLatin = mValue.utf8().length() != mValue.length();
}


//-----------------------------------------------------------------------------
bool KMFldSearchRule::matches(const KMMessage* aMsg, const QCString& aMsgStr)
  const
{
  QString value;

  if (mField.isEmpty()) return true;
  if (aMsg)
  {
    if( !isHeaderField() ) {
      value = aMsg->headerAsString();
      QCString charset, content;
      if (aMsg->typeStr().lower().find("multipart/") != -1)
      {
        KMMessagePart mp;
        aMsg->bodyPart(0,&mp);
        charset = mp.charset();
        content = mp.bodyDecoded();
      } else {
        charset = aMsg->charset();
        content = aMsg->bodyDecoded();
      }
      if (!mNonLatin || charset.isEmpty() || charset == "us-ascii"
        || charset == "iso-8859-1")         // Speedup
          value += content;
      else {
        QTextCodec *codec = KMMsgBase::codecForName(charset);
        if (codec) value += codec->toUnicode(content);
          else value += content;
      }
    } else {
      value = aMsg->headerField(mField.latin1());
    }
  } else {
    int start, stop;
    char ch = '\0';
    start = aMsgStr.find(mHeaderField, 0, false /*cis*/ );
    if (start == -1) return false;
    int endOfHeader = aMsgStr.find("\n\n");
    if (endOfHeader == -1) endOfHeader = aMsgStr.find("\n\r\n");
    if (endOfHeader < start) return false;
    start += mFieldLength;
    stop = aMsgStr.find("\n", start);
    while (stop != -1 && (ch = aMsgStr.at(stop + 1)) == ' ' || ch == '\t')
      stop = aMsgStr.find("\n", stop + 1);
    if (stop == -1) value = KMMsgBase::decodeRFC2047String(aMsgStr.mid(start));
    else value = KMMsgBase::decodeRFC2047String(aMsgStr.mid(start,
      stop - start));
  }
  // also see KMFilterRule::matches() for a similar function:
  switch(mFunc)
  {
  case Equal:
    return (value == mValue);
  case NotEqual:
    return (value != mValue);
  case Contains:
    return value.contains(mValue, FALSE);
  case NotContains:
    return ( ! value.contains(mValue, FALSE) );
  case MatchesRegExp:
    return (value.find(QRegExp(mValue, FALSE)) >= 0);
  case NotMatchesRegExp:
    return (value.find(QRegExp(mValue, FALSE)) < 0);
  default:
    kdDebug(5006) << "KMFldSearchRule::matches: wrong rule func #" << mFunc << endl;
    return false;
  }
}


//-----------------------------------------------------------------------------
bool KMFldSearchRule::isHeaderField() const
{
  return mField != i18n("<complete message>");
}


//-----------------------------------------------------------------------------
void KMFldSearchRule::setEnabled(bool b) {
  mCbxField->setEnabled(b);
  mCbxFunc->setEnabled(b);
  mEdtValue->setEnabled(b);
}

//-----------------------------------------------------------------------------
void KMFldSearchRule::setFocus() {
  mEdtValue->setFocus();
}
//-----------------------------------------------------------------------------
#include "kmfldsearch.moc"
