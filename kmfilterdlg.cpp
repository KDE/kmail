// kmfilterdlg.cpp

#include <qfiledialog.h>

#include "kmfilterdlg.h"
#include "kmfilter.h"
#include "kmfiltermgr.h"
#include "kmglobal.h"
#include "kmfolder.h"
#include "kmfolderdir.h"
#include "kmfoldermgr.h"

#include <kapp.h>
#include <kdebug.h>
#include <kbuttonbox.h>
#include <qbuttongroup.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <assert.h>
#include <qstrlist.h>
#include <klocale.h>
#include <kwin.h>

static QStringList sFilterOpList, sFilterFuncList, sFilterFieldList,
                sFilterActionList;

//=============================================================================
KMFaComboBox::KMFaComboBox(QWidget* p, const char* n):
  KMFaComboBoxInherited(false, p, n)
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
  updown_move_semaphore = 1;

  initMetaObject();

  grid  = new QGridLayout(this, 4, 2, 4, 4);
  mFilter = 0;

  setCaption(i18n("Filter Rules"));
  KWin::setIcons(winId(), kapp->icon(), kapp->miniIcon());

  mFilterList = new QListBox(this);
  mFilterList->setMinimumSize(100, 200);
  grid->addMultiCellWidget(mFilterList, 0, 2, 0, 0);
  connect(mFilterList,SIGNAL(highlighted(int)),SLOT(slotFilterSelected(int)));

  initLists();

  //---------- static filter fields
  fgrid = new QGridLayout(3, 3, 4);
  grid->addLayout(fgrid, 0, 1);

  mRuleFuncA = new QComboBox(false, this);
  sz = mRuleFuncA->sizeHint();
  w = sz.width();
  h = sz.height();
  mCbxHeight = h;
  mRuleFuncA->setMinimumSize(110, h);
  mRuleFuncA->setMaximumSize(32767, h);
  mRuleFuncA->insertStringList(sFilterFuncList);
  fgrid->addWidget(mRuleFuncA, 0, 1);

  mRuleFuncB = new QComboBox(false, this);
  mRuleFuncB->setMinimumSize(110, h);
  mRuleFuncB->setMaximumSize(32767, h);
  mRuleFuncB->insertStringList(sFilterFuncList);
  fgrid->addWidget(mRuleFuncB, 2, 1);

  mRuleFieldA = new QComboBox(true, this);
  mRuleFieldA->insertStringList(sFilterFieldList);
  mRuleFieldA->setMinimumSize(100, h);
  mRuleFieldA->setMaximumSize(32767, h);
  fgrid->addWidget(mRuleFieldA, 0, 0);

  mRuleFieldB = new QComboBox(true, this);
  mRuleFieldB->insertStringList(sFilterFieldList);
  mRuleFieldB->setMinimumSize(100, h);
  mRuleFieldB->setMaximumSize(32767, h);
  fgrid->addWidget(mRuleFieldB, 2, 0);

  mRuleValueA = new QLineEdit(this);
  mRuleValueA->adjustSize();
  mRuleValueA->setMinimumSize(80, mRuleValueA->sizeHint().height());
  fgrid->addWidget(mRuleValueA, 0, 2);

  mRuleValueB = new QLineEdit(this);
  mRuleValueB->adjustSize();
  mRuleValueB->setMinimumSize(80, mRuleValueB->sizeHint().height());
  fgrid->addWidget(mRuleValueB, 2, 2);

  mRuleOp = new QComboBox(false, this);
  mRuleOp->insertStringList(sFilterOpList);
  mRuleOp->setMinimumSize(50, h);
  mRuleOp->setMaximumSize(32767, h);
  fgrid->addMultiCellWidget(mRuleOp, 1, 1, 0, 1);

  //---------- filter action area
  agrid = new QGridLayout(FILTER_MAX_ACTIONS, 3, 4);
  grid->addLayout(agrid, 1, 1);
  for (i=0; i<FILTER_MAX_ACTIONS; i++)
  {
    mFaType[i] = new KMFaComboBox(this);
    mFaType[i]->insertStringList(sFilterActionList);
    mFaType[i]->setMinimumSize(80, h);
    mFaType[i]->setMaximumSize(32767, h);
    agrid->addWidget(mFaType[i], i, 0);
    connect(mFaType[i], SIGNAL(selectType(KMFaComboBox*, int)),
	    SLOT(slotActionTypeSelected(KMFaComboBox*,int)));

    mFaBtnDetails[i] = 0;
    mFaField[i] = 0;
  }
  mActLineHeight = mFaType[0]->size().height();

  //---------- button area
  buttonBox = new KButtonBox(this, Horizontal, 0, 2);
  grid->addMultiCellWidget(buttonBox, 3, 3, 0, 1);

  mBtnUp = buttonBox->addButton(i18n("Up"));
  connect(mBtnUp,SIGNAL(clicked()),SLOT(slotBtnUp()));

  mBtnDown  = buttonBox->addButton(i18n("Down"));
  connect(mBtnDown,SIGNAL(clicked()),SLOT(slotBtnDown()));

  mBtnNew = buttonBox->addButton(i18n("New"));
  connect(mBtnNew,SIGNAL(clicked()),SLOT(slotBtnNew()));

  mBtnDelete = buttonBox->addButton(i18n("Delete"));
  connect(mBtnDelete,SIGNAL(clicked()),SLOT(slotBtnDelete()));

  buttonBox->addStretch();
  mBtnOk = buttonBox->addButton(i18n("OK"));
  connect(mBtnOk,SIGNAL(clicked()),SLOT(slotBtnOk()));

  mBtnCancel = buttonBox->addButton(i18n("Cancel"));
  connect(mBtnCancel,SIGNAL(clicked()),SLOT(slotBtnCancel()));

  buttonBox->addStretch();
  btnHelp = buttonBox->addButton(i18n("Help"));
  connect(btnHelp,SIGNAL(clicked()),SLOT(slotBtnHelp()));

  buttonBox->layout();
  buttonBox->setMaximumSize(32767, buttonBox->sizeHint().height());
  buttonBox->setMinimumSize(buttonBox->sizeHint());

  //----------
  grid->setRowStretch(0, 2);
  grid->setRowStretch(1, 10);
  grid->setRowStretch(2, 2);
  grid->setColStretch(0, 10);
  grid->setColStretch(1, 20);
  grid->setRowStretch(3, 100);
  grid->activate();

  /*
  enableControls();
  resize(buttonBox->sizeHint().width()*1.2, sizeHint().height());
  show();
  */

  reloadFilterList();

  mCurFilterIdx = -1;
  if(mFilterList->count() > 0)
    mFilterList->setCurrentItem(0);

  enableControls();
  resize(static_cast<int>(buttonBox->sizeHint().width()*1.2),
	 sizeHint().height());
  show();

}


//-----------------------------------------------------------------------------
KMFilterDlg::~KMFilterDlg()
{
  //
  // Espen 2000-05-21: Would be better to send a signal but I can't
  // get it to work here.
  //
  kernel->filterMgr()->dialogDestroyed();
}


//-----------------------------------------------------------------------------
void KMFilterDlg::reloadFilterList(void)
{
  KMFilter* filter;
  int i, num;

  mFilterList->clear();

  num = kernel->filterMgr()->count();
  for (i=0; i<num; i++)
  {
    filter = kernel->filterMgr()->at(i);
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
      mFaBtnDetails[i] = 0;
    }
    if (mFaField[i])
    {
      delete mFaField[i];
      mFaField[i] = 0;
    }
  }
}


//-----------------------------------------------------------------------------
void KMFilterDlg::showFilter(KMFilter* aFilter)
{
  int i, w;
  KMFilterAction* action;
  QWidget* pwidg;
  QSize sz;

  assert(aFilter!=0); // Important assert
  disconnect(mRuleFieldA, SIGNAL(textChanged(const QString&)),
	     this, SLOT(updateCurFilterName(const QString&)));
  disconnect(mRuleValueA, SIGNAL(textChanged(const QString&)),
	     this, SLOT(updateCurFilterName(const QString&)));
  clear();

  mRuleOp->setCurrentItem((int)aFilter->oper());

  i = indexOfRuleField(aFilter->ruleA().field());
  if (i < 0)
  {
    mRuleFieldA->changeItem(aFilter->ruleA().field(),0);
    i = 0;
  }
  else mRuleFieldA->changeItem(" ",0);
  mRuleFieldA->setCurrentItem(i);
  mRuleFuncA->setCurrentItem((int)aFilter->ruleA().function());
  mRuleValueA->setText(aFilter->ruleA().contents());

  i = indexOfRuleField(aFilter->ruleB().field());
  if (i < 0)
  {
    mRuleFieldB->changeItem(aFilter->ruleB().field(),0);
    i = 0;
  }
  else mRuleFieldB->changeItem(" ",0);
  mRuleFieldB->setCurrentItem(i);
  mRuleFuncB->setCurrentItem((int)aFilter->ruleB().function());
  mRuleValueB->setText(aFilter->ruleB().contents());

  for (i=0; i<FILTER_MAX_ACTIONS; i++)
  {
    action = aFilter->action(i);
    if (mFaField[i]) delete mFaField[i];
    mFaField[i] = 0;

    if (!action) mFaType[i]->setCurrentItem(0);
    else
    {
      mFaType[i]->setCurrentItem(1+kernel->filterActionDict()->indexOf(action->name()));
      pwidg = action->createParamWidget(this);
      mFaField[i] = pwidg;
      if (pwidg)
      {
	QPoint pos = mFaType[i]->pos();
	pos.setX(pos.x() + mFaType[i]->width() + 4);
        w = width() - pos.x();
        if (w > 300) w = 300;
        sz.setWidth(w);
        sz.setHeight(pwidg->height());
        pwidg->resize(sz);
	pwidg->move(pos);
	pwidg->show();
      }
    }
  }

  mFilter = aFilter;
  mCurFilterIdx = mFilterList->currentItem();
  connect(mRuleFieldA, SIGNAL(textChanged(const QString&)),
	  this, SLOT(updateCurFilterName(const QString&)));
  connect(mRuleValueA, SIGNAL(textChanged(const QString&)),
	  this, SLOT(updateCurFilterName(const QString&)));
}

//-----------------------------------------------------------------------------
void KMFilterDlg::resizeEvent(QResizeEvent *qre)
{
  kdDebug() << "KMFilterDlg::resizeEvent" << endl;
  kdDebug() << QString( "width %1" ).arg( qre->size().width() ) << endl;
  int i, w;
  QWidget* pwidg;
  QSize sz;

  for (i=0; i<FILTER_MAX_ACTIONS; i++)
    if (mFaField[i]) {
      pwidg = mFaField[i];
      QPoint pos = mFaType[i]->pos();
      pos.setX(pos.x() + mFaType[i]->width() + 4);
      w = qre->size().width() - pos.x();
      if (w > 300) w = 300;
      sz.setWidth(w);
      sz.setHeight(pwidg->height());
      pwidg->resize(sz);
      pwidg->move(pos);
      kdDebug() << QString( "posx %1" ).arg( pos.x() ) << endl;
    }
}

//-----------------------------------------------------------------------------
QString KMFilterDlg::ruleFieldToEnglish(const QString & i18nVal)
{
  if (i18nVal == i18n("<message>")) return QString("<message>");
  if (i18nVal == i18n("<body>")) return QString("<body>");
  if (i18nVal == i18n("<any header>")) return QString("<any header>");
  if (i18nVal == i18n("<To or Cc>")) return QString("<To or Cc>");
  return i18nVal;
}

//-----------------------------------------------------------------------------
void KMFilterDlg::applyFilterChanges(void)
{
  KMFilterAction* action;
  int i;

  if (!mFilter || !updown_move_semaphore) return;

  mFilter->ruleA().init(ruleFieldToEnglish(mRuleFieldA->currentText()),
			(KMFilterRule::Function)mRuleFuncA->currentItem(),
			mRuleValueA->text());
  mFilter->ruleB().init(ruleFieldToEnglish(mRuleFieldB->currentText()),
			(KMFilterRule::Function)mRuleFuncB->currentItem(),
			mRuleValueB->text());
  mFilter->setOper((KMFilter::Operator)mRuleOp->currentItem());

  for (i=0; i<FILTER_MAX_ACTIONS; i++)
  {
    action = mFilter->action(i);
    if (!action) continue;
    action->applyParamWidgetValue(mFaField[i]);
  }
}

//-----------------------------------------------------------------------------
void KMFilterDlg::updateCurFilterName(const QString &/*text*/)
{
  if (mCurFilterIdx < 0)
    return;
  mFilter->setName(QString("<") + mRuleFieldA->currentText() + ">:"
		   + mRuleValueA->text());
  QObject::disconnect(mFilterList,SIGNAL(highlighted(int)),this,SLOT(slotFilterSelected(int)));
  mFilterList->changeItem(mFilter->name(), mCurFilterIdx);
  connect(mFilterList,SIGNAL(highlighted(int)),SLOT(slotFilterSelected(int)));
}

//-----------------------------------------------------------------------------
bool KMFilterDlg::testOpts(const QWidget* w) const
{
  if (!w) kdDebug() << "KMFilterDlg: no widget given" << endl;
  return (w!=0);
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
QLineEdit* KMFilterDlg::createEdit(const QString &aTxt)
{
  QLineEdit* edt = new QLineEdit(this);

  edt->setText(aTxt);
  return edt;
}

//-----------------------------------------------------------------------------
QComboBox* KMFilterDlg::createFolderCombo( QStringList *str,
				  QValueList< QGuardedPtr<KMFolder> > *folders,
				  QGuardedPtr<KMFolder> curFolder )
{
  QComboBox* cbx = new QComboBox(false, this);
  int i=0,idx=-1;

  cbx->setFixedHeight(mCbxHeight);

  QStringList::Iterator st;
  for( st = str->begin(); st != str->end(); ++st)
    cbx->insertItem(*st);

  QGuardedPtr<KMFolder> folder;
  while (folders->at(i) != folders->end()) {
    folder = *folders->at(i);
    if (folder == curFolder)
      idx = i;
    ++i;
  }
  if (idx>=0) cbx->setCurrentItem(idx);
  return cbx;
}

//-----------------------------------------------------------------------------
QComboBox* KMFilterDlg::createCombo( QStringList *str,
				     QString curItem )
{
  QComboBox* cbx = new QComboBox(false, this);
  cbx->setFixedHeight(mCbxHeight);

  QStringList::Iterator st;
  for( st = str->begin(); st != str->end(); ++st) {
    cbx->insertItem(*st);
    if (*st == curItem)
      cbx->setCurrentItem( cbx->count() - 1 );
  }

  return cbx;
}

//-----------------------------------------------------------------------------
void KMFilterDlg::slotActionTypeSelected(KMFaComboBox* cbx, int idx)
{
  KMFilterAction* action;
  QWidget* widg;
  QPoint pos;
  QSize sz;
  int i, w;

  if (!mFilter) return;

  for (i=FILTER_MAX_ACTIONS-1; i>0; i--)
    if (cbx == mFaType[i]) break;
  if (i < 0) return;

  if (mFaField[i]) delete mFaField[i];
  mFaField[i] = 0;
  mGridRow = i;

  if (mFilter->action(i)) delete mFilter->action(i);
  action = kernel->filterActionDict()->create(
              kernel->filterActionDict()->nameOf(*(sFilterActionList.at(idx))));
  mFilter->setAction(i, action);
  if (!action || idx < 0)
  {
    kdDebug() << "no action selected" << endl;
    return;
  }

  widg = action->createParamWidget(this);
  mFaField[i] = widg;
  if (!widg) return;

  pos = mFaType[i]->pos();
  pos.setX(pos.x() + mFaType[idx]->width() + 4);
  widg->move(pos);
  w = width() - pos.x();
  if (w > 300) w = 300;
  sz.setWidth(w);
  sz.setHeight(widg->height());
  widg->resize(sz);
  widg->show();
}

//-----------------------------------------------------------------------------
void KMFilterDlg::slotFilterSelected(int idx)
{
  KMFilter* filter;

  kdDebug() << QString( "mCurFilterIdx %1 idx %2" ).arg( mCurFilterIdx ).arg( idx ) << endl;
  if (mCurFilterIdx == idx)
    return;

  if (mFilter) applyFilterChanges();
  if ((uint)idx < kernel->filterMgr()->count())
  {
    filter = kernel->filterMgr()->at(idx);
    if (filter) showFilter(filter);
  }
  else
  {
    clear();
    mFilter = 0;
  }
  enableControls();
}

//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnUp()
{
  int idx = mFilterList->currentItem();
  KMFilter* filter;

  if (idx < 1) return;
  applyFilterChanges();
  mCurFilterIdx = -1;
  updown_move_semaphore = 0;

  filter = kernel->filterMgr()->take(idx);
  assert(filter != 0);
  kernel->filterMgr()->insert(idx-1, filter);

  // This next line is to work around a QT 2.0 CVS bug
  // If it is omitted the listbox is refreshed incorrectly
  // and looks like it contains duplicate highlighted items
  mFilterList->setCurrentItem(idx-1);
  mFilterList->removeItem(idx);
  mFilterList->insertItem(filter->name(), idx-1);
  mFilterList->setCurrentItem(idx-1);

  enableControls();
  updown_move_semaphore = 1;
}

//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnDown()
{
  int idx = mFilterList->currentItem();
  KMFilter* filter;

  if (idx < 0 || idx >= (int)mFilterList->count()-1) return;
  applyFilterChanges();
  mCurFilterIdx = -1;

  updown_move_semaphore = 0;

  filter = kernel->filterMgr()->take(idx);
  assert(filter != 0);
  kernel->filterMgr()->insert(idx+1, filter);

  mFilterList->removeItem(idx);
  mFilterList->insertItem(filter->name(), idx+1);
  mFilterList->setCurrentItem(idx+1);

  enableControls();
  updown_move_semaphore = 1;
}

//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnNew()
{
  int idx;
  KMFilter* filter = new KMFilter;
  filter->setName(i18n("Unnamed"));
  applyFilterChanges();
  mCurFilterIdx = -1;

  idx = mFilterList->currentItem();
  if (idx >= 0) kernel->filterMgr()->insert(idx, filter);
  else kernel->filterMgr()->append(filter);
  idx = kernel->filterMgr()->find(filter);
  mFilterList->insertItem(filter->name(), idx);
  mFilterList->setCurrentItem(idx);
  slotFilterSelected(idx);
  enableControls();
}

//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnDelete()
{
  int idx = mFilterList->currentItem();
  if (idx < 0) return;

  mFilter = 0;
  mCurFilterIdx = -1;

  QObject::disconnect(mFilterList,SIGNAL(highlighted(int)),this,SLOT(slotFilterSelected(int)));
  mFilterList->removeItem(idx);
  kernel->filterMgr()->remove(idx);

  if (idx >= (int)kernel->filterMgr()->count())
    idx = (int)kernel->filterMgr()->count()-1;

  if (idx >= 0) {
    mFilterList->setCurrentItem(idx);
    // workaround QT bug where current item is not selected
    // after item is deleted
    mFilterList->setSelected(idx, TRUE);
  }
  else
    mCurFilterIdx = -1;

  enableControls();
  connect(mFilterList,SIGNAL(highlighted(int)),SLOT(slotFilterSelected(int)));
  slotFilterSelected( idx ); // workaround another QT bug
}

//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnOk()
{
  if (mFilter) applyFilterChanges();
  kernel->filterMgr()->writeConfig();

  KMFilterDlgInherited::close();
}

//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnCancel()
{
  kernel->filterMgr()->readConfig();
  KMFilterDlgInherited::close();
}

//-----------------------------------------------------------------------------
void KMFilterDlg::closeEvent( QCloseEvent *e )
{
  kernel->filterMgr()->readConfig();
  KMFilterDlgInherited::closeEvent(e);
}

//-----------------------------------------------------------------------------
void KMFilterDlg::slotBtnHelp()
{
  kapp->invokeHelp("FILTERS_ID");
}

//-----------------------------------------------------------------------------
int KMFilterDlg::indexOfRuleField(const QString &aName) const
{
  int i;

  for (i=sFilterFieldList.count()-1; i>=0; i--)
  {
    if (*(sFilterFieldList.at(i))==i18n(aName)) break;
  }
  return i;
}

//-----------------------------------------------------------------------------
void KMFilterDlg::initLists()
{
  QString name;

  //---------- initialize list of filter actions
  if (sFilterActionList.count() <= 0)
  {
    sFilterActionList.append(i18n("<nothing>"));
    for (name=kernel->filterActionDict()->first(); !name.isEmpty();
	 name=kernel->filterActionDict()->next())
    {
      sFilterActionList.append(kernel->filterActionDict()->currentLabel());
    }
  }

  //---------- initialize list of filter functions
  if (sFilterOpList.count() <= 0)
  {
    sFilterOpList.append(i18n("ignore"));
    sFilterOpList.append(i18n("and"));
    sFilterOpList.append(i18n("unless"));
    sFilterOpList.append(i18n("or"));
  }

  //---------- initialize list of filter operators
  if (sFilterFuncList.count() <= 0)
  {
    sFilterFuncList.append(i18n("equals"));
    sFilterFuncList.append(i18n("not equal"));
    sFilterFuncList.append(i18n("contains"));
    sFilterFuncList.append(i18n("doesn't contain"));
    sFilterFuncList.append(i18n("regular expression"));
  }

  //---------- initialize list of filter operators
  if (sFilterFieldList.count() <= 0)
  {
    sFilterFieldList.append(" ");
    // also see KMFilterRule::matches() and KMFilterDlg::ruleFieldToEnglish()
    // if you change the following strings!
    sFilterFieldList.append(i18n("<message>"));
    sFilterFieldList.append(i18n("<body>"));
    sFilterFieldList.append(i18n("<any header>"));
    sFilterFieldList.append(i18n("<To or Cc>"));
    sFilterFieldList.append("Subject");
    sFilterFieldList.append("From");
    sFilterFieldList.append("To");
    sFilterFieldList.append("Cc");
    sFilterFieldList.append("Reply-To");
    sFilterFieldList.append("Organization");
    sFilterFieldList.append("Resent-From");
    sFilterFieldList.append("X-Loop");
  }
}

void KMFilterDlg::enableControls()
{
  bool upEnabled = FALSE;
  bool downEnabled = FALSE;
  bool deleteEnabled = FALSE;
  int i;

  if (mFilterList->count() > 0)
    deleteEnabled = TRUE;
  if (deleteEnabled && (mFilterList->currentItem() != 0))
    upEnabled = TRUE;
  if (deleteEnabled &&
      (mFilterList->currentItem() != (int)mFilterList->count() - 1))
    downEnabled = TRUE;
  mBtnUp->setEnabled( upEnabled );
  mBtnDown->setEnabled( downEnabled );
  mBtnDelete->setEnabled( deleteEnabled );

  for (i=0; i<FILTER_MAX_ACTIONS; i++)
    if (mFaType[i])
      mFaType[i]->setEnabled( deleteEnabled );
  mRuleFieldA->setEnabled( deleteEnabled );
  mRuleFieldB->setEnabled( deleteEnabled );
  mRuleFuncA->setEnabled( deleteEnabled );
  mRuleFuncB->setEnabled( deleteEnabled );
  mRuleValueA->setEnabled( deleteEnabled );
  mRuleValueB->setEnabled( deleteEnabled );
  mRuleOp->setEnabled( deleteEnabled );
}


//-----------------------------------------------------------------------------
void KMFilterDlg::createFilter(const QString &field, const QString &value)
{
  int idx;
  KMFilter* filter = new KMFilter;
  filter->setName(i18n("Unnamed"));
  filter->ruleA().init( field, KMFilterRule::FuncEquals, value );
  applyFilterChanges();
  mCurFilterIdx = -1;

  idx = mFilterList->currentItem();
  if (idx >= 0) kernel->filterMgr()->insert(idx, filter);
  else kernel->filterMgr()->append(filter);
  idx = kernel->filterMgr()->find(filter);
  mFilterList->insertItem(filter->name(), idx);
  mFilterList->setCurrentItem(idx);
  slotFilterSelected(idx);
  updateCurFilterName( "" );
  mFaType[0]->setCurrentItem( 1 ); //transfer type
  slotActionTypeSelected( mFaType[0], 1 );
  enableControls();
}


//-----------------------------------------------------------------------------
#include "kmfilterdlg.moc"
