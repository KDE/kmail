/* kmfldsearch
 * (c) 1999 Stefan Taferner, (c) 2001 Aaron J. Seigo
 * This code is under GPL
 */
// kmfldsearch.cpp

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
#include <qregexp.h>

#include <kaction.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <kwin.h>

#include <mimelib/enum.h>

#include <stdlib.h>

//-----------------------------------------------------------------------------
KMFldSearch::KMFldSearch(KMMainWin* w, const char* name,
                         KMFolder *curFolder, bool modal):
  KDialogBase(NULL, name, modal, i18n("Search in Folders"),
              KDialogBase::User1 | KDialogBase::User2 | KDialogBase::Close,
              KDialogBase::User1, false, i18n("&Search"), i18n("S&top")),
  mSearchAllFolders(false),
  mSearching(false),
  mStopped(false),
  mCloseRequested(false),
  mNumRules(2),
  mNumMatches(0),
  mCount(0),
  mLastFocus(0),
  mMainWin(w)
{
  KWin::setIcons(winId(), kapp->icon(), kapp->miniIcon());
  
  KConfig* config = KGlobal::config();
  config->setGroup("SearchDialog");

  QWidget* searchWidget = new QWidget(this);
  mGrid = new QGridLayout(searchWidget, mNumRules+5, 5, spacingHint());

  mChkbxAllFolders = new QRadioButton(i18n("Search in &all local folders"), searchWidget);
  mGrid->addMultiCellWidget(mChkbxAllFolders, 0, 0, 0, 3);
  connect(mChkbxAllFolders, SIGNAL(toggled(bool)),
          this, SLOT(slotSearchAllFolders(bool)));

  mChkbxSpecificFolders = new QRadioButton(i18n("Search &only in:"), searchWidget);
  mGrid->addWidget(mChkbxSpecificFolders, 1, 0);
  mChkbxSpecificFolders->setChecked(true);
  connect(mChkbxSpecificFolders, SIGNAL(toggled(bool)),
          this, SLOT(slotSearchSpecificFolder(bool)));

  mCbxFolders = new KMFolderComboBox(false, searchWidget);
  mCbxFolders->setMinimumSize(mCbxFolders->sizeHint());
  mCbxFolders->setMaximumSize(1024, mCbxFolders->sizeHint().height());
  mCbxFolders->setFolder(curFolder);
  mGrid->addMultiCellWidget(mCbxFolders, 1, 1, 1, 2);

  connect(mCbxFolders, SIGNAL(activated(int)),
          this, SLOT(slotFolderActivated(int)));

  mRules = new KMFldSearchRule*[mNumRules];

  KMFldSearchRule* rule = 0;
  for (int i=0; i<mNumRules; ++i)
  {
    rule = new KMFldSearchRule(searchWidget, mGrid, i+2, 0);
    rule->updateFunctions(mCbxFolders->getFolder());
    mRules[i] = rule;
  }

  mChkSubFolders = new QCheckBox(i18n("I&nclude sub-folders"), searchWidget);
  mChkSubFolders->setChecked(true);
  mGrid->addWidget(mChkSubFolders, 1, 3);

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
  mGrid->addMultiCellWidget(mLbxMatches, mNumRules+2, mNumRules+2, 0, 3);

  mStatusBar = new KStatusBar(searchWidget);
  mStatusBar->insertFixedItem(i18n("AMiddleLengthText..."), 0, true);
  mStatusBar->changeItem(i18n("Ready."), 0);
  mStatusBar->setItemAlignment(0, AlignLeft | AlignVCenter);
  mStatusBar->insertItem(QString::null, 1, 1, true);
  mStatusBar->setItemAlignment(1, AlignLeft | AlignVCenter);
  mGrid->addMultiCellWidget(mStatusBar, mNumRules+4, mNumRules+4, 0, 3);

  mGrid->setColStretch(0, 0);
  mGrid->setColStretch(1, 2);
  mGrid->setColStretch(2, 1);
  mGrid->setColStretch(3, 100);
  mGrid->setColStretch(4, 0);

  mGrid->setRowStretch(mNumRules+2, 100);
  mGrid->setRowStretch(mNumRules+3, 0);

  mGrid->activate();
  mRules[0]->setFocus();
  
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
  connect(this, SIGNAL(finished()), this, SLOT(slotDelayedDestruct()));
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
  delete [] mRules;
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
      genMsg = i18n("Search cancelled"); 
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
  int i;
  bool matches = true;

  mCount++;
  if (mCount % 100 == 0) updStatus();
  for(i=0; matches && i<mNumRules; i++)
    if (!mRules[i]->matches(aMsg, aMsgStr))
      matches = false;

  return matches;
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotFolderComplete(KMFolderImap *folder, bool success)
{
  disconnect(folder, SIGNAL(folderComplete(KMFolderImap*, bool)),
             this, SLOT(slotFolderComplete(KMFolderImap*, bool)));
  
  if (success)
  {
    searchInFolder(folder);
  }
  else 
  {
    searchDone();
  }

  folder->close();
}


//-----------------------------------------------------------------------------
void KMFldSearch::searchInFolder(QGuardedPtr<KMFolder> aFld, bool recursive)
{
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

  for(i=0; i<mNumRules; i++)
    if (!mRules[i]->isHeaderField()) fastMode = false;

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
      if(mSearchFolder == i18n("sent-mail"))
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
  if (aFld->protocol() == "imap") 
    searchDone();

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
  int i = 0;

  mMainWin->folderTree()->createFolderList( &str, &folders );
  for (; folders.at(i) != folders.end(); ++i)
  {
    folder = *folders.at(i);
    // Stop pressed?
    if(!mSearching)
    {
      break;
    }

    if (folder && (folder->protocol() != "imap"))
    {
      searchInFolder(folder);
    }
  }
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotFolderActivated(int nr)
{
  KMFolder* folder = mCbxFolders->getFolder();
  
  mChkbxSpecificFolders->setChecked(true);
  mBtnSearch->setEnabled(folder);

  for (int i = 0; i < mNumRules; i++)
  {
    mRules[i]->updateFunctions(folder);
  }
}


//-----------------------------------------------------------------------------
void KMFldSearch::activateFolder(KMFolder *curFolder)
{
  mChkbxSpecificFolders->setChecked(true);
  mCbxFolders->setFolder(curFolder);
  mRules[0]->setFocus();
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

  mBtnSearch->setEnabled(false);
  mBtnStop->setEnabled(true);

  enableGUI();
  mLbxMatches->clear();
  kapp->processEvents();

  for (int i = 0; i < mNumRules; ++i)
    mRules[i]->prepare();

  if (mSearchAllFolders)
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

    if (folder->protocol() == "imap")
    {
      KMFolderImap *imap_folder = static_cast<KMFolderImap*>(folder);
      if (imap_folder &&
          imap_folder->getImapState()== KMFolderImap::imapNoInformation)
      {
        imap_folder->open();
        connect(imap_folder,
                SIGNAL(folderComplete(KMFolderImap *, bool)),
                SLOT(slotFolderComplete(KMFolderImap *, bool)));
        imap_folder->getFolder();
        return;
      }
    }

    searchInFolder(folder, mChkSubFolders->isChecked());
  }

  searchDone();
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
void KMFldSearch::slotSearchAllFolders(bool on)
{
  if (on)
  {
    mChkbxSpecificFolders->setChecked(false);
    mCbxFolders->setEnabled(false);
    mChkSubFolders->setEnabled(false);
    mSearchAllFolders = true;
  }
  else if (!mChkbxSpecificFolders->isChecked())
  {
    mChkbxAllFolders->setChecked(true);
//    mChkbxSpecificFolders->setChecked(true);
  }
}

//-----------------------------------------------------------------------------
void KMFldSearch::slotSearchSpecificFolder(bool on)
{
  if (on)
  {
    mChkbxAllFolders->setChecked(false);
    mCbxFolders->setEnabled(true);
    mChkSubFolders->setEnabled(true);
    mSearchAllFolders = false;
  }
  else if (!mChkbxAllFolders->isChecked())
  {
    mChkbxSpecificFolders->setChecked(true);
//    mChkbxAllFolders->setChecked(true);
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

  for(int i = 0; i < mNumRules; i++)
    mRules[i]->setEnabled(!mSearching);
}


//=============================================================================
KMFldSearchRule::KMFldSearchRule(QWidget* aParent, QGridLayout* aGrid,
                                 int aRow, int aCol)
{
  assert(aParent!=NULL);
  assert(aGrid!=NULL);

  mRow = aRow;
  mCbxField = new QComboBox(true, aParent);
  insertFieldItems(TRUE);
  mCbxField->setMinimumSize(mCbxField->sizeHint());
  mCbxField->setMaximumSize(1024, mCbxField->sizeHint().height());

  mCbxFunc = new QComboBox(false, aParent);
  mCbxFunc->insertItem(i18n("contains"));
  mCbxFunc->insertItem(i18n("doesn't contain"));
  mCbxFunc->insertItem(i18n("equals"));
  mCbxFunc->insertItem(i18n("not equal"));
  mCbxFunc->insertItem(i18n("matches regular expr."));
  mCbxFunc->insertItem(i18n("doesn't match regular expr."));
  mCbxFunc->setMinimumSize(mCbxFunc->sizeHint());
  mCbxFunc->setMaximumSize(1024, mCbxFunc->sizeHint().height());

  mEdtValue = new QLineEdit(aParent);
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
void KMFldSearchRule::insertFieldItems(bool all)
{
  int last = mCbxField->currentItem();
  mCbxField->clear();
  if (mRow > 2) mCbxField->insertItem(QString::null);
  mCbxField->insertItem("Subject");
  mCbxField->insertItem("From");
  mCbxField->insertItem("To");
  mCbxField->insertItem("Cc");
  if (all) {
    mCbxField->insertItem("Organization");
    mCbxField->insertItem(i18n("<complete message>"));
  }

  if (last < mCbxField->count())
    mCbxField->setCurrentItem(last);
}


//-----------------------------------------------------------------------------
void KMFldSearchRule::updateFunctions(KMFolder* folder)
{
  insertFieldItems(folder && (folder->protocol() != "imap"));
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
    start = aMsgStr.find(mHeaderField);
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
