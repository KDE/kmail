// kmfldsearch.cpp
// TODO: Add search in subfolder checkbox
// TODO: Allow searching in bodies
// TODO: Use msgIdMD5 in MSGID_COLUMN

#include "kmfldsearch.h"
#include "kmglobal.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmmainwin.h"

#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qstring.h>
#include <qlayout.h>
#include <qlabel.h>
#include <assert.h>
#include <klocale.h>

#include <kapp.h>
#include <kbuttonbox.h>
#include <qlistview.h>

//-----------------------------------------------------------------------------
KMFldSearch::KMFldSearch(KMMainWin* w, const char* name, bool modal, WFlags f): 
  KMFldSearchInherited(NULL, name, modal, f)
{
  KButtonBox* bbox;
  QLabel* lbl;
  KMFldSearchRule* rule;
  int i;

  mMainWin = w;

  mNumRules  = 2;
  mSearching = false;

  setCaption(i18n("Search in Folders"));
  mGrid = new QGridLayout(this, mNumRules+4, 5, 4, 4);

  mCbxFolders = createFolderCombo("");
  mCbxFolders->setMinimumSize(mCbxFolders->sizeHint());
  mCbxFolders->setMaximumSize(1024, mCbxFolders->sizeHint().height());
  mGrid->addMultiCellWidget(mCbxFolders, 0, 0, 1, 2);

  mRules = new KMFldSearchRule*[mNumRules];

  for (i=0; i<mNumRules; i++)
  {
    lbl = new QLabel(i ? i18n("and") : i18n("where"), this);
    lbl->setMinimumSize(lbl->sizeHint());
    lbl->setAlignment(AlignRight|AlignVCenter);
    mGrid->addWidget(lbl, i+1, 0);

    rule = new KMFldSearchRule(this, mGrid, i+1, 1);
    mRules[i] = rule;
  }

  lbl = new QLabel(i18n("Search in"), this);
  lbl->setFixedSize(lbl->sizeHint().width(), mCbxFolders->sizeHint().height());
  lbl->setAlignment(AlignRight|AlignVCenter);
  mGrid->addWidget(lbl, 0, 0);

  mLbxMatches = new QListView(this, "Search in Folders");
  mLbxMatches->setSorting(-1);
  mLbxMatches->addColumn(i18n("Subject"), 150);
  mLbxMatches->addColumn(i18n("Sender"), 120);
  mLbxMatches->addColumn(i18n("Date"), 80);
#define LOCATION_COLUMN 3
  mLbxMatches->addColumn(i18n("Folder"), 200);

#define MSGID_COLUMN 4
  //TODO: Use msgIDMD5, and create KMFolder::findIdMD5(QString) method
  mLbxMatches->addColumn(i18n(""));
  mLbxMatches->setColumnWidth( MSGID_COLUMN, 0 );

#define FOLDERID_COLUMN 5
  mLbxMatches->addColumn(i18n(""));
  mLbxMatches->setColumnWidth( FOLDERID_COLUMN, 0 );

  mLbxMatches->setMinimumSize(300, 100);
  mLbxMatches->setMaximumSize(2048, 2048);
  mLbxMatches->resize(300, 400);
  connect(mLbxMatches, SIGNAL(doubleClicked(QListViewItem *)),
	  this, SLOT(slotShowMsg(QListViewItem *)));
  //mLbxMatches->readConfig();
  mGrid->addMultiCellWidget(mLbxMatches, mNumRules+2, mNumRules+2, 0, 4);

  bbox = new KButtonBox(this, KButtonBox::VERTICAL);
  mGrid->addMultiCellWidget(bbox, 0, mNumRules-1, 4, 4);
  mBtnSearch = bbox->addButton(i18n("Search"));
  connect(mBtnSearch, SIGNAL(clicked()), SLOT(slotSearch()));
  mBtnClear  = bbox->addButton(i18n("Clear"));
  connect(mBtnClear, SIGNAL(clicked()), SLOT(slotClear()));
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
#warning Save QListView layout
  //  mLbxMatches->writeConfig();
}


//-----------------------------------------------------------------------------
QComboBox* KMFldSearch::createFolderCombo(const QString curFolder)
{
 QList<KMFolder> folders;
 QComboBox* cbx = new QComboBox(false, this);
 QStringList str;
 
 kernel->folderMgr()->createFolderList( &str, &folders );
 cbx->setFixedHeight(cbx->sizeHint().height());
 
 cbx->insertItem("Search all folders");
 QStringList::Iterator st;
 for( st = str.begin(); st != str.end(); ++st)
   cbx->insertItem(*st);
 
#warning TODO: preselect current folder
 return cbx; 
}


//-----------------------------------------------------------------------------
void KMFldSearch::updStatus(void)
{
  QString str;

  if (!mSearching) {
    if(!mStopped)
      str = i18n("Done, %1 matches").arg(mNumMatches);
    else
      str = i18n("Search cancelled, %1 matches so far").arg(mNumMatches);    
  } else
    str = i18n("%1 matches, searching in %2")
		.arg(mNumMatches)
		.arg(mSearchFolder);
  mLblStatus->setText(str);
}


//-----------------------------------------------------------------------------
bool KMFldSearch::searchInMessage(KMMessage* aMsg)
{
  int i;
  bool matches = true;

  assert(aMsg!=NULL);

  for(i=0; matches && i<mNumRules; i++)
    if (!mRules[i]->matches(aMsg)) 
      matches = false;

  return matches;
}


//-----------------------------------------------------------------------------
void KMFldSearch::searchInFolder(KMFolder* aFld, int fldNum)
{
  KMMessage* msg;
  int i, num, upd;
  QString str;

  assert(aFld!=NULL);
  
  mBtnSearch->setText("Stop");
  mSearchFolder=aFld->name();
  kapp->processEvents();
  if (aFld->open() != 0)
  {
    debug("Cannot open folder '%s'", (const char*)aFld->name());
    return;
  }

  num = aFld->count();
  for (i=0, upd=0; i<num && mSearching; i++, upd++)
  {
    msg = aFld->getMsg(i);
    if (msg && searchInMessage(msg))
    {
      (void)new QListViewItem(mLbxMatches,
			      msg->subject(), 
			      msg->from(),
			      msg->dateStr(),
			      aFld->name(),
			      QString("%1").arg(i),
			      QString("%1").arg(fldNum)
			      );
      mNumMatches++;
      updStatus();
    }
    if (upd > 10)
    {
      kapp->processEvents();
      upd = 0;
    }
    aFld->unGetMsg(i);
  }

  if(!mSearching)
    // was stopped
    mStopped = true;

  updStatus();

  aFld->close();
  mBtnSearch->setText("Search");
}


//-----------------------------------------------------------------------------
void KMFldSearch::searchInAllFolders(void)
{
  QList<KMFolder> folders;
  KMFolder *folder;
  QStringList str;
  int i = 0;

  kernel->folderMgr()->createFolderList( &str, &folders );
  for(folder = folders.first(); folder != 0; folder = folders.next()) {
    // Stop pressed?
    if(!mSearching)
      break;
    searchInFolder(folder,i);
    ++i;
  }
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotSearch()
{
  int i;

  if (mSearching)
  {
    mSearching = false;
    return;
  }
  
  mStopped = false;
  mNumMatches = 0;
  mSearching  = true;

  mBtnSearch->setCaption(i18n("Stop"));
  enableGUI();
  mLbxMatches->clear();
  kapp->processEvents();

  for (i=0; i<mNumRules; i++)
    mRules[i]->prepare();
  
  if (mCbxFolders->currentItem() <= 0) 
    searchInAllFolders();
  else {
    QList<KMFolder> folders;
    QStringList str;
    kernel->folderMgr()->createFolderList( &str, &folders );
    if (str[mCbxFolders->currentItem()-1] == mCbxFolders->currentText()) {
      searchInFolder(folders.at(mCbxFolders->currentItem()-1),
		     mCbxFolders->currentItem()-1);
    }
  }

  mSearching=false;
  updStatus();

  mBtnSearch->setCaption(i18n("Search"));
  enableGUI();
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotClose()
{
  accept();
  fprintf(stderr, "here\n");
  delete this;
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotShowMsg(QListViewItem *item)
{
  KMFolder* fld;
  KMMessage* msg;

  if(!item)
    return;

  QString fldName = item->text(LOCATION_COLUMN);
  QList<KMFolder> folders;
  QStringList str;
  int idx = atoi(item->text(FOLDERID_COLUMN));
  kernel->folderMgr()->createFolderList( &str, &folders );
  fld = folders.at(idx);
  if (!fld) 
    return;
  // This could goto the wrong folder if the folder list has been modified

  mMainWin->slotSelectFolder(fld);
  msg = fld->getMsg(atoi(item->text(MSGID_COLUMN)));
  if (!msg) 
    return;

  //  mMainWin->slotMsgSelected(msg);
  mMainWin->slotSelectMessage(msg);
  fprintf(stderr, "still here 3\n");
}


//-----------------------------------------------------------------------------
void KMFldSearch::slotClear()
{
  int i;

  for (i=0; i<mNumRules; i++)
    mRules[i]->clear();

  mLbxMatches->clear();
}

void KMFldSearch::enableGUI() {
  mBtnClose->setEnabled(!mSearching);
  mBtnClear->setEnabled(!mSearching);
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

  mCbxField = new QComboBox(true, aParent);
  mCbxField->insertItem("");
  mCbxField->insertItem("Subject");
  mCbxField->insertItem("From");
  mCbxField->insertItem("To");
  mCbxField->insertItem("Cc");
  mCbxField->insertItem("Reply-To");
  mCbxField->insertItem("Organization");
  mCbxField->insertItem("Resent-From");
  mCbxField->insertItem("X-Loop");
  mCbxField->setMinimumSize(mCbxField->sizeHint());
  mCbxField->setMaximumSize(1024, mCbxField->sizeHint().height());

  mCbxFunc = new QComboBox(false, aParent);
  mCbxFunc->insertItem(i18n("contains"));
  mCbxFunc->insertItem(i18n("doesn't contain"));
  mCbxFunc->insertItem(i18n("equals"));
  mCbxFunc->insertItem(i18n("not equal"));
  mCbxFunc->insertItem(i18n("less or equal"));
  mCbxFunc->insertItem(i18n("greater or equal"));
  mCbxFunc->setMinimumSize(mCbxFunc->sizeHint());
  mCbxFunc->setMaximumSize(1024, mCbxFunc->sizeHint().height());

  mEdtValue = new QLineEdit(aParent);
  mEdtValue->setMinimumSize(mCbxFunc->sizeHint());
  mEdtValue->setMaximumSize(1024, mCbxFunc->sizeHint().height());

  aGrid->addWidget(mCbxField, aRow, aCol);
  aGrid->addWidget(mCbxFunc,  aRow, aCol+1);
  aGrid->addWidget(mEdtValue, aRow, aCol+2);
}


//-----------------------------------------------------------------------------
KMFldSearchRule::~KMFldSearchRule()
{
}


//-----------------------------------------------------------------------------
void KMFldSearchRule::prepare(void)
{
  mFieldIdx = mCbxField->currentItem();
  mField = mCbxField->currentText();
  mFunc = mCbxFunc->currentItem();
  mValue = mEdtValue->text();
}


//-----------------------------------------------------------------------------
bool KMFldSearchRule::matches(const KMMessage* aMsg) const
{
  QString value;

  if (mFieldIdx <= 0 || !aMsg) return true;
  value = aMsg->headerField(mField);

  switch(mFunc)
  {
  case Equal: 
    return (stricmp(value, mValue) == 0);
  case NotEqual:
    return (stricmp(value, mValue) != 0);
  case Contains:
    return (value.find(mValue) >= 0);
  case NotContains:
    return (value.find(mValue) >= 0);
  case GreaterEqual: 
    return (stricmp(value, mValue) >= 0);
  case LessEqual: 
    return (stricmp(value, mValue) <= 0);
  default:
    debug("KMFldSearchRule::matches: wrong rule func #%d", mFunc);
    return false;
  }
}


//-----------------------------------------------------------------------------
void KMFldSearchRule::clear(void)
{
  mCbxField->setCurrentItem(1);
  mCbxField->setCurrentItem(0);
  mCbxFunc->setCurrentItem(0);
  mEdtValue->setText("");  
}


void KMFldSearchRule::setEnabled(bool b) {
  mCbxField->setEnabled(b);
  mCbxFunc->setEnabled(b);
  mEdtValue->setEnabled(b);
}

//-----------------------------------------------------------------------------
#include "kmfldsearch.moc"
