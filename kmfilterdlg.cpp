// kmfilterdlg.cpp

#include <qfiledlg.h>

#include "kmfilterdlg.h"
#include "kmfilter.h"
#include "kmfiltermgr.h"
#include "kmglobal.h"
#include "kmfolder.h"
#include "kmfolderdir.h"
#include "kmfoldermgr.h"

#include <klocale.h>
#include <kmsgbox.h>
#include <ktablistbox.h>
#include <kbuttonbox.h>
#include <qbttngrp.h>
#include <qframe.h>
#include <qgrpbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qpushbt.h>
#include <qradiobt.h>
#include <qcombo.h>
#include <qlined.h>
#include <assert.h>
#include <qstrlist.h>

static QStrList sFilterOpList, sFilterFuncList, sFilterFieldList, 
                sFilterActionList;

//=============================================================================
KMFaComboBox::KMFaComboBox(QWidget* p, const char* n):
  KMFaComboBoxInherited(p, n)
{
  initMetaObject();
  connect(this, SIGNAL(activated(int)), SLOT(slotSelected(int)));
}

void KMFaComboBox::slotSelected(int idx)
{
  emit selectType(this, idx);
}


//=============================================================================
KMFilterDlg::KMFilterDlg(QWidget* parent, const char* name):
  KMFilterDlgInherited(parent, name, FALSE)
{
  KButtonBox *buttonBox;
  QPushButton *btnHelp;
  QGridLayout *fgrid, *grid, *agrid;
  int h, w, i;
  QSize sz;

  initMetaObject();

  grid  = new QGridLayout(this, 3, 2, 4, 4);
  mFilter = NULL;

  setCaption(nls->translate("Filter Rules"));
  //setOKButton(nls->translate("Ok"));
  //setCancelButton(nls->translate("Cancel"));

  mFilterList = new QListBox(this);
  mFilterList->setMinimumSize(100, 200);
  grid->addMultiCellWidget(mFilterList, 0, 1, 0, 0);
  connect(mFilterList,SIGNAL(highlighted(int)),SLOT(slotFilterSelected(int)));

  initLists();

  //---------- static filter fields
  fgrid = new QGridLayout(3, 3, 4);
  grid->addLayout(fgrid, 0, 1);
  mRuleFieldA = new QComboBox(TRUE, this);
  mRuleFieldA->insertStrList(&sFilterFieldList);
  sz = mRuleFieldA->sizeHint();
  w = sz.width();
  h = sz.height();
  mRuleFieldA->setMinimumSize(50, h);
  mRuleFieldA->setMaximumSize(32767, h);
  fgrid->addWidget(mRuleFieldA, 0, 0);

  mRuleFieldB = new QComboBox(TRUE, this);
  mRuleFieldB->insertStrList(&sFilterFieldList);
  mRuleFieldB->setMinimumSize(50, h);
  mRuleFieldB->setMaximumSize(32767, h);
  fgrid->addWidget(mRuleFieldB, 2, 0);

  mRuleFuncA = new QComboBox(this);
  mRuleFuncA->setMinimumSize(40, h);
  mRuleFuncA->setMaximumSize(32767, h);
  mRuleFuncA->insertStrList(&sFilterFuncList);
  fgrid->addWidget(mRuleFuncA, 0, 1);

  mRuleFuncB = new QComboBox(this);
  mRuleFuncB->setMinimumSize(40, h);
  mRuleFuncB->setMaximumSize(32767, h);
  mRuleFuncB->insertStrList(&sFilterFuncList);
  fgrid->addWidget(mRuleFuncB, 2, 1);

  mRuleValueA = new QLineEdit(this);
  mRuleValueA->adjustSize();
  mRuleValueA->setMinimumSize(80, mRuleValueA->sizeHint().height());
  fgrid->addWidget(mRuleValueA, 0, 2);

  mRuleValueB = new QLineEdit(this);
  mRuleValueB->adjustSize();
  mRuleValueB->setMinimumSize(80, mRuleValueB->sizeHint().height());
  fgrid->addWidget(mRuleValueB, 2, 2);

  mRuleOp = new QComboBox(this);
  mRuleOp->insertStrList(&sFilterOpList);
  mRuleOp->setMinimumSize(50, h);
  mRuleOp->setMaximumSize(32767, h);
  fgrid->addMultiCellWidget(mRuleOp, 1, 1, 0, 1);

  //---------- filter action area
  agrid = new QGridLayout(FILTER_MAX_ACTIONS, 3, 4);
  grid->addLayout(agrid, 1, 1);
  for (i=0; i<FILTER_MAX_ACTIONS; i++)
  {
    mFaType[i] = new KMFaComboBox(this);
    mFaType[i]->insertStrList(&sFilterActionList);
    mFaType[i]->setMinimumSize(80, h);
    mFaType[i]->setMaximumSize(32767, h);
    agrid->addWidget(mFaType[i], i, 0);
    connect(mFaType[i], SIGNAL(selectType(KMFaComboBox*, int)),
	    SLOT(slotActionTypeSelected(KMFaComboBox*,int)));

    mFaBtnDetails[i] = NULL;
    mFaField[i] = NULL;
  }
  mActLineHeight = mFaType[0]->size().height();

  //---------- button area
  buttonBox = new KButtonBox(this, KButtonBox::HORIZONTAL, 0, 2);
  grid->addMultiCellWidget(buttonBox, 2, 2, 0, 1);

  mBtnUp = buttonBox->addButton(nls->translate("Up"));
  connect(mBtnUp,SIGNAL(clicked()),SLOT(slotBtnUp()));

  mBtnDown  = buttonBox->addButton(nls->translate("Down"));
  connect(mBtnDown,SIGNAL(clicked()),SLOT(slotBtnDown()));

  mBtnNew = buttonBox->addButton(nls->translate("New"));
  connect(mBtnNew,SIGNAL(clicked()),SLOT(slotBtnNew()));

  mBtnDelete = buttonBox->addButton(nls->translate("Delete"));
  connect(mBtnDelete,SIGNAL(clicked()),SLOT(slotBtnDelete()));

  buttonBox->addStretch();
  mBtnOk = buttonBox->addButton(nls->translate("Ok"));
  connect(mBtnOk,SIGNAL(clicked()),SLOT(slotBtnOk()));

  mBtnCancel = buttonBox->addButton(nls->translate("Cancel"));
  connect(mBtnCancel,SIGNAL(clicked()),SLOT(slotBtnCancel()));

  buttonBox->addStretch();
  btnHelp = buttonBox->addButton(nls->translate("Help"));
  connect(btnHelp,SIGNAL(clicked()),SLOT(slotBtnHelp()));

  buttonBox->layout();
  buttonBox->setMaximumSize(32767, buttonBox->sizeHint().height());

  //----------
  grid->setRowStretch(0, 10);
  grid->setRowStretch(1, 10);
  grid->setRowStretch(2, 0);
  grid->setColStretch(0, 10);
  grid->setColStretch(1, 20);
  grid->activate();

  reloadFilterList();
}


//-----------------------------------------------------------------------------
KMFilterDlg::~KMFilterDlg()
{
}


//-----------------------------------------------------------------------------
void KMFilterDlg::reloadFilterList(void)
{
  KMFilter* filter;
  int i, num;

  mFilterList->clear();

  num = filterMgr->count();
  for (i=0; i<num; i++)
  {
    filter = filterMgr->at(i);
    if (!filter) continue;
    mFilterList->insertItem(filter->name());
  }
}


//-----------------------------------------------------------------------------
void KMFilterDlg::clear(void)
{
  int i;
  for (i=0; i<FILTER_MAX_ACTIONS; i++)
  {
    if (mFaBtnDetails[i])
    {
      delete mFaBtnDetails[i];
      mFaBtnDetails[i] = NULL;
    }
    if (mFaField[i])
    {
      delete mFaField[i];
      mFaField[i] = NULL;
    }
  }
}


//-----------------------------------------------------------------------------
void KMFilterDlg::showFilter(KMFilter* aFilter)
{
  int i;
  assert(aFilter!=NULL);

  clear();

  for (i=0; i<FILTER_MAX_ACTIONS; i++)
  {
  }
}


//-----------------------------------------------------------------------------
void KMFilterDlg::applyFilterChanges(void)
{
  
}


//-----------------------------------------------------------------------------
bool KMFilterDlg::testOpts(const QWidget* w) const
{
  if (!w) debug("KMFilterDlg: no widget given");
  return (w!=NULL);
}


//-----------------------------------------------------------------------------
QPushButton* KMFilterDlg::createDetailsButton(void)
{
  QPushButton* btn;

  btn = mFaBtnDetails[mGridRow];
  if (!btn)
  {
    btn = new QPushButton("...", this);
    mFaBtnDetails[mGridRow] = btn;
  }
  return btn;
}


//-----------------------------------------------------------------------------
QComboBox* KMFilterDlg::createFolderCombo(const QString curFolder)
{
  QComboBox* cbx = new QComboBox(this);
  KMFolderDir* fdir = &(folderMgr->dir());
  KMFolder* cur;
  int i=0, idx=-1;

  for (cur=(KMFolder*)fdir->first(); cur; cur=(KMFolder*)fdir->next(), i++)
  {
    cbx->insertItem(cur->name());
    if (cur->name() == curFolder) idx=i;
  }
  if (idx>=0) cbx->setCurrentItem(idx);

  return cbx;
}


//-----------------------------------------------------------------------------
void KMFilterDlg::slotActionTypeSelected(KMFaComboBox* cbx, int idx)
{
  KMFilterAction* action;
  int i;

  for (i=FILTER_MAX_ACTIONS-1; i>0; i--)
    if (cbx == mFaType[i]) break;
  if (i < 0) return;

  if (mFaField[i]) delete mFaField[i];
  mFaField[i] = NULL;
  mGridRow = i;

  action = filterActionDict->create(sFilterActionList.at(idx));
  if (!action || idx <= 0) return;

  if (mFilter->action(idx)) delete mFilter->action(idx);
  mFilter->setAction(idx, action);

  action->createParamWidget(this);
}


//-----------------------------------------------------------------------------
void KMFilterDlg::slotFilterSelected(int idx)
{
  KMFilter* filter;

  if (mFilter) applyFilterChanges();
  filter = filterMgr->at(idx);
  if (filter) showFilter(filter);
}


//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnUp()
{
  int idx = mFilterList->currentItem();
  KMFilter* filter;

  if (idx < 1) return;

  filter = filterMgr->take(idx);
  assert(filter != NULL);
  filterMgr->insert(idx-1, filter);

  mFilterList->removeItem(idx);
  mFilterList->insertItem(filter->name(), idx-1);

  mFilterList->setCurrentItem(idx-1);
}


//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnDown()
{
  int idx = mFilterList->currentItem();
  KMFilter* filter;

  if (idx < 0 || idx >= (int)mFilterList->count()-1) return;

  filter = filterMgr->take(idx);
  assert(filter != NULL);
  filterMgr->insert(idx+1, filter);

  mFilterList->removeItem(idx);
  mFilterList->insertItem(filter->name(), idx+1);

  mFilterList->setCurrentItem(idx+1);
}


//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnNew()
{
  int idx;
  KMFilter* filter = new KMFilter;
  filter->setName(nls->translate("Unnamed"));

  idx = mFilterList->currentItem();

  if (idx >= 0) filterMgr->insert(idx, filter);
  else filterMgr->append(filter);

  mFilterList->insertItem(filter->name());

  if (idx < 0) idx = ((int)mFilterList->count()) - 1;
  slotFilterSelected(idx);
}


//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnDelete()
{
  int idx = mFilterList->currentItem();
  if (idx < 0) return;

  filterMgr->remove(idx);
  mFilterList->removeItem(idx);

  if (idx >= (int)filterMgr->count())
    idx = (int)filterMgr->count()-1;

  if (idx >= 0) mFilterList->setCurrentItem(idx);
}


//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnOk()
{
  accept();
}


//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnCancel()
{
  reject();
}


//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnHelp()
{
}


//-----------------------------------------------------------------------------
void KMFilterDlg::initLists(void)
{
  QString name;

  //---------- initialize list of filter actions
  if (sFilterActionList.count() <= 0)
  {
    sFilterActionList.append(nls->translate("<nothing>"));
    for (name=filterActionDict->first(); !name.isEmpty(); 
	 name=filterActionDict->next())
    {
      sFilterActionList.append(filterActionDict->currentLabel());
    }
  }

  //---------- initialize list of filter functions
  if (sFilterFuncList.count() <= 0)
  {
    sFilterFuncList.append(nls->translate("ignore"));
    sFilterFuncList.append(nls->translate("and"));
    sFilterFuncList.append(nls->translate("unless"));
    sFilterFuncList.append(nls->translate("or"));
  }

  //---------- initialize list of filter operators
  if (sFilterOpList.count() <= 0)
  {
    sFilterOpList.append(nls->translate("equals"));
    sFilterOpList.append(nls->translate("not equal"));
    sFilterOpList.append(nls->translate("contains"));
    sFilterOpList.append(nls->translate("doesn't contain"));
    sFilterOpList.append(nls->translate("regular expression"));
  }

  //---------- initialize list of filter operators
  if (sFilterFieldList.count() <= 0)
  {
    sFilterFieldList.append(nls->translate("<message>"));
    sFilterFieldList.append(nls->translate("<body>"));
    sFilterFieldList.append(nls->translate("<any header>"));
    sFilterFieldList.append("Subject");
    sFilterFieldList.append("From");
    sFilterFieldList.append("To");
    sFilterFieldList.append("Reply-To");
    sFilterFieldList.append("Organization");
    sFilterFieldList.append("Resent-From");
    sFilterFieldList.append("X-Loop");
  }
}


//-----------------------------------------------------------------------------
#include "kmfilterdlg.moc"
