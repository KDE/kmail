// kmfldsearch.cpp
// TODO: Add search in subfolder checkbox
// TODO: Use msgIdMD5 in MSGID_COLUMN

#include "kmfldsearch.h"
#include "kmglobal.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmfoldertree.h"
#include "kmmainwin.h"
#include "kmacctimap.h"

#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qstring.h>
#include <qlayout.h>
#include <qlabel.h>
#include <assert.h>

#include <klocale.h>
#include <kdebug.h>
#include <kapp.h>
#include <kbuttonbox.h>
#include <qlistview.h>
#include <qheader.h>

#include <stdlib.h>

//-----------------------------------------------------------------------------
KMFldSearch::KMFldSearch(KMMainWin* w, const char* name,
  KMFolder *curFolder, bool modal, WFlags f):
  KMFldSearchInherited(NULL, name, modal, f | Qt::WDestructiveClose)
{
  KButtonBox* bbox;
  QLabel* lbl;
  KMFldSearchRule* rule;
  int i;

  mMainWin = w;

  mNumRules  = 2;
  mSearching = false;
  mCloseRequested = false;

  setCaption(i18n("Search in Folders"));
  mGrid = new QGridLayout(this, mNumRules+4, 5, 4, 4);

  mCbxFolders = createFolderCombo(curFolder);
  mCbxFolders->setMinimumSize(mCbxFolders->sizeHint());
  mCbxFolders->setMaximumSize(1024, mCbxFolders->sizeHint().height());
  mGrid->addMultiCellWidget(mCbxFolders, 0, 0, 1, 2);

  connect(mCbxFolders, SIGNAL(activated(int)),
    SLOT(slotFolderActivated(int)));

  mRules = new KMFldSearchRule*[mNumRules];

  for (i=0; i<mNumRules; i++)
  {
    lbl = new QLabel(i ? i18n("and") : i18n("where"), this);
    lbl->setMinimumSize(lbl->sizeHint());
    lbl->setAlignment(AlignRight|AlignVCenter);
    mGrid->addWidget(lbl, i+1, 0);

    rule = new KMFldSearchRule(this, mGrid, i+1, 1);
    rule->updateFunctions(mCbxFolders, mFolders);
    mRules[i] = rule;
  }

  lbl = new QLabel(i18n("Search in"), this);
  lbl->setFixedSize(lbl->sizeHint().width(), mCbxFolders->sizeHint().height());
  lbl->setAlignment(AlignRight|AlignVCenter);
  mGrid->addWidget(lbl, 0, 0);

  mLastFocus = 0L;	// to remeber the position of the focus

  mLbxMatches = new QListView(this, "Search in Folders");
  /* Default is to sort by date. TODO: Unfortunately this sorts *while*
     inserting, which looks rather strange - the user cannot read
     the results so far as they are constantly re-sorted --dnaber
  */
  mLbxMatches->setSorting(2, false);
  mLbxMatches->setShowSortIndicator(true);
  mLbxMatches->setAllColumnsShowFocus(true);
  mLbxMatches->addColumn(i18n("Subject"), 150);
  mLbxMatches->addColumn(i18n("Sender/Receiver"), 120);
  mLbxMatches->addColumn(i18n("Date"), 120);
#define LOCATION_COLUMN 3
  mLbxMatches->addColumn(i18n("Folder"), 100);

#define MSGID_COLUMN 4
  //TODO: Use msgIDMD5, and create KMFolder::findIdMD5(QString) method
  mLbxMatches->addColumn("");		// should be hidden
  mLbxMatches->setColumnWidthMode( MSGID_COLUMN, QListView::Manual );
  mLbxMatches->header()->setResizeEnabled(false, MSGID_COLUMN);
  mLbxMatches->setColumnWidth( MSGID_COLUMN, 0 );

#define FOLDERID_COLUMN 5
  mLbxMatches->addColumn("");		// should be hidden
  mLbxMatches->setColumnWidthMode( FOLDERID_COLUMN, QListView::Manual );
  mLbxMatches->header()->setResizeEnabled(false, FOLDERID_COLUMN);
  mLbxMatches->setColumnWidth( FOLDERID_COLUMN, 0 );

  connect(mLbxMatches, SIGNAL(doubleClicked(QListViewItem *)),
	  this, SLOT(slotShowMsg(QListViewItem *)));
  //mLbxMatches->readConfig();
  mGrid->addMultiCellWidget(mLbxMatches, mNumRules+2, mNumRules+2, 0, 4);

  bbox = new KButtonBox(this, Vertical);
  mGrid->addMultiCellWidget(bbox, 0, mNumRules, 4, 4);
  mBtnSearch = bbox->addButton(i18n("Search"));
  mBtnSearch->setDefault(true);
  connect(mBtnSearch, SIGNAL(clicked()), SLOT(slotSearch()));
  mBtnStop = bbox->addButton(i18n("Stop"));
  mBtnStop->setEnabled(false);
  connect(mBtnStop, SIGNAL(clicked()), SLOT(slotStop()));
  mBtnClose  = bbox->addButton(i18n("Close"));
  connect(mBtnClose, SIGNAL(clicked()), SLOT(slotClose()));
  bbox->layout();
  bbox->setFixedWidth(bbox->width());

  mLblStatus = new QLabel(" ", this);
  mLblStatus->setFixedHeight(mLblStatus->sizeHint().height());
  mGrid->addMultiCellWidget(mLblStatus, mNumRules+3, mNumRules+3, 0, 3);

  mGrid->setColStretch(0, 0);
  mGrid->setColStretch(1, 2);
  mGrid->setColStretch(2, 1);
  mGrid->setColStretch(3, 100);
  mGrid->setColStretch(4, 0);

  mGrid->setRowStretch(mNumRules+2, 100);
  mGrid->setRowStretch(mNumRules+3, 0);

  mGrid->activate();
  resize(sizeHint());
}


//-----------------------------------------------------------------------------
KMFldSearch::~KMFldSearch()
{
  //TODO Save QListView layout
  delete [] mRules;
}


//-----------------------------------------------------------------------------
QComboBox* KMFldSearch::createFolderCombo(KMFolder *curFolder)
{
  QComboBox* cbx = new QComboBox(false, this);

  mMainWin->folderTree()->createFolderList( &mFolderNames, &mFolders );
  cbx->setFixedHeight(cbx->sizeHint().height());

  cbx->insertItem(i18n("<Search all local folders>"));
  cbx->insertStringList(mFolderNames);
  if (curFolder && !curFolder->isDir())
    cbx->setCurrentItem(mFolders.findIndex(curFolder) + 1);
  return cbx;
}


//-----------------------------------------------------------------------------
void KMFldSearch::updStatus(void)
{
  QString str;

  if (!mSearching) {
    if(!mStopped)
      str = i18n("Done, %n match (%1)", "Done, %n matches (%1)", mNumMatches)
        .arg(i18n("%n message processed",
        "%n messages processed", mCount));
    else
      str = i18n("Search cancelled, %n match so far (%1)",
                 "Search cancelled, %n matches so far (%1)", mNumMatches)
        .arg(i18n("%n message processed",
        "%n messages processed", mCount));
  } else
    str = i18n("%n match, searching in %1 (message %2)",
               "%n matches, searching in %1 (message %2)", mNumMatches)
		.arg(mSearchFolder)
		.arg(mCount);
  mLblStatus->setText(str);
}


//-----------------------------------------------------------------------------
void KMFldSearch::keyPressEvent(QKeyEvent *evt)
{

  if(!evt)
    return;

  switch (evt->key()) {
      case Key_Return:
      case Key_Enter:
        KMFldSearchInherited::keyPressEvent(evt);
        break;
      case Key_Escape:
        if( mSearching )
          mSearching = false;
	else
          slotClose();
        break;
  }
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
void KMFldSearch::slotFolderComplete(KMFolderTreeItem *fti, bool success)
{
  disconnect(fti->folder->account(),
    SIGNAL(folderComplete(KMFolderTreeItem*, bool)),
    this, SLOT(slotFolderComplete(KMFolderTreeItem*, bool)));
  if (success) searchInFolder(fti->folder, mFolders.findIndex(fti->folder));
  else searchDone();
  fti->folder->close();
}


//-----------------------------------------------------------------------------
void KMFldSearch::searchInFolder(QGuardedPtr<KMFolder> aFld, int fldNum)
{
  KMMessage* msg;
  int i, num, upd;
  QString str;
  QCString cStr;
  bool unget = false, fastMode = true, found;

  assert(!aFld.isNull());

  if (aFld->isSystemFolder()) mSearchFolder = i18n(aFld->name().utf8());
    else mSearchFolder = aFld->name();
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
    } else {
      unget = !aFld->isMessage(i);
      msg = aFld->getMsg(i);
      found = msg && searchInMessage(msg, QCString());
    }
    if (found)
    {
      QString from;
      if( mSearchFolder == i18n("sent-mail") ) {
        from = msg->to();
      } else {
        from = msg->from();
      }
      (void)new QListViewItem(mLbxMatches,
			      msg->subject(),
			      from,
			      msg->dateIsoStr(),
			      mSearchFolder,
			      QString("%1").arg(i),
			      QString("%1").arg(fldNum)
			      );
      mNumMatches++;
    }
    if (upd > 10)
    {
      kapp->processEvents();
      upd = 0;
      if (!aFld) // Folder deleted while searching!
	break;
    }
    if (unget) aFld->unGetMsg(i);
  }

  if(!mSearching)
    // was stopped
    mStopped = true;

  updStatus();

  aFld->close();
  if (aFld->account()) searchDone();
}


//-----------------------------------------------------------------------------
void KMFldSearch::searchInAllFolders(void)
{
  QValueList<QGuardedPtr<KMFolder> > folders;
  QGuardedPtr<KMFolder> folder;
  QStringList str;
  int i = 0;

  mMainWin->folderTree()->createFolderList( &str, &folders );
  while (folders.at(i) != folders.end()) {
    folder = *folders.at(i);
    // Stop pressed?
    if(!mSearching)
      break;
    if (folder && !folder->account()) searchInFolder(folder,i);
    ++i;
  }
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotFolderActivated(int nr)
{
  mBtnSearch->setEnabled(nr == 0 || *mFolders.at(nr - 1));
  for (int i = 0; i < mNumRules; i++)
    mRules[i]->updateFunctions(mCbxFolders, mFolders);
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotSearch()
{
  int i;
  mLastFocus = focusWidget();
  mBtnSearch->setFocus();	// set focus so we don't miss key event

  mCount = 0;
  mStopped = false;
  mNumMatches = 0;
  mSearching  = true;

  mBtnSearch->setEnabled(false);
  mBtnStop->setEnabled(true);
  enableGUI();
  mLbxMatches->clear();
  kapp->processEvents();

  for (i=0; i<mNumRules; i++)
    mRules[i]->prepare();

  if (mCbxFolders->currentItem() <= 0)
    searchInAllFolders();
  else {
    QValueList<QGuardedPtr<KMFolder> > folders;
    QStringList str;
    mMainWin->folderTree()->createFolderList( &str, &folders );
    if (str[mCbxFolders->currentItem()-1] == mCbxFolders->currentText()) {
      KMFolder *folder = *folders.at(mCbxFolders->currentItem()-1);
      KMFolderTreeItem *fti;
      if (folder->account() && (fti = static_cast<KMFolderTreeItem*>
        (mMainWin->folderTree()->indexOfFolder(folder)))
        ->mImapState == KMFolderTreeItem::imapNoInformation)
      {
        folder->open();
        connect(folder->account(),
          SIGNAL(folderComplete(KMFolderTreeItem *, bool)),
          SLOT(slotFolderComplete(KMFolderTreeItem *, bool)));
        folder->account()->getFolder(fti);
        return;
      }
      searchInFolder(folder, mCbxFolders->currentItem()-1);
    }
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
  else e->accept();
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotShowMsg(QListViewItem *item)
{
  QGuardedPtr<KMFolder> fld;
  KMMessage* msg;

  if(!item)
    return;

  QString fldName = item->text(LOCATION_COLUMN);
  QValueList<QGuardedPtr<KMFolder> > folders;
  QStringList str;
  int idx = item->text(FOLDERID_COLUMN).toInt();
  mMainWin->folderTree()->createFolderList( &str, &folders );
  if (folders.at(idx) == folders.end())
    return;
  fld = *folders.at(idx);
  if (!fld)
    return;
  // This could goto the wrong folder if the folder list has been modified
  kdDebug(5006) << "fld " << endl;

  mMainWin->slotSelectFolder(fld);
  msg = fld->getMsg(item->text(MSGID_COLUMN).toInt());
  if (!msg)
    return;

  mMainWin->slotSelectMessage(msg);
}


//-----------------------------------------------------------------------------
void KMFldSearch::enableGUI() {
  mBtnClose->setEnabled(!mSearching);
  mCbxFolders->setEnabled(!mSearching);

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
  if( aRow == 1 ) {
	mEdtValue->setFocus();
  }
  aGrid->addWidget(mCbxField, aRow, aCol);
  aGrid->addWidget(mCbxFunc,  aRow, aCol+1);
  aGrid->addWidget(mEdtValue, aRow, aCol+2);
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
  if (mRow > 1) mCbxField->insertItem(QString::null);
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
void KMFldSearchRule::updateFunctions(QComboBox *cbx,
  const QValueList<QGuardedPtr<KMFolder> > &folders)
{
  int cur = cbx->currentItem();
  insertFieldItems(!cur || (*folders.at(cur-1) &&
    !(*folders.at(cur-1))->account()));
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
      value = aMsg->headerField(mField);
    }
  } else {
    int start, stop;
    char ch;
    start = aMsgStr.find(mHeaderField);
    if (start == -1) return false;
    if (aMsgStr.find("\n\n") < start) return false;
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
#include "kmfldsearch.moc"
