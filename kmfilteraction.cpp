// kmfilteraction.cpp

#include "kmmessage.h"
#include "kmfilteraction.h"
#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include <kapp.h>
#include <kconfig.h>
#include <qcombo.h>

static QString resultStr;


//-----------------------------------------------------------------------------
KMFilterAction::KMFilterAction(const char* name): 
  KMFilterActionInherited(NULL, name)
{
}

KMFilterAction::~KMFilterAction()
{
}

KMFilterAction* KMFilterAction::newAction(void)
{
  return NULL;
}

QWidget* KMFilterAction::createParamWidget(KMGFilterDlg*)
{
  return NULL;
}

void KMFilterAction::applyParamWidgetValue(QWidget*)
{
}

bool KMFilterAction::folderRemoved(KMFolder*, KMFolder*)
{
  return FALSE;
}

int KMFilterAction::tempOpenFolder(KMFolder* aFolder)
{
  return filterMgr->tempOpenFolder(aFolder);
}



//=============================================================================
//
//   Specific  Filter  Actions
//
//=============================================================================

//=============================================================================
// Move message to another mail folder
//=============================================================================
class KMFilterActionMove: public KMFilterAction
{
public:
  KMFilterActionMove();
  virtual const QString label(void) const;
  virtual bool process(KMMessage* msg, bool& stopIt);
  virtual QWidget* createParamWidget(KMGFilterDlg* parent);
  virtual void applyParamWidgetValue(QWidget* paramWidget);
  virtual void argsFromString(const QString argsStr);
  virtual const QString argsAsString(void) const;
  virtual bool folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder);
  static KMFilterAction* newAction(void);
protected:
  KMFolder* mDest;
};

bool KMFilterActionMove::folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder)
{
  if (aFolder==mDest)
  {
    mDest = aNewFolder;
  }
  return TRUE;
}

KMFilterAction* KMFilterActionMove::newAction(void)
{
  return (new KMFilterActionMove);
}

const QString KMFilterActionMove::label(void) const
{
  return i18n("transfer");
}

KMFilterActionMove::KMFilterActionMove(): KMFilterAction("transfer")
{
  mDest = NULL;
}

bool KMFilterActionMove::process(KMMessage* msg, bool&stop)
{
  if (!mDest) return TRUE;
  KMFilterAction::tempOpenFolder(mDest);
  mDest->moveMsg(msg);
  //stop = TRUE;  //Stefan: no, we do not want to stop here!

  return FALSE;
}

QWidget* KMFilterActionMove::createParamWidget(KMGFilterDlg* aParent)
{
  QString name;
  QComboBox* cbx;

  if (mDest) name = mDest->name();
  cbx = aParent->createFolderCombo(name);
  return cbx;
}

void KMFilterActionMove::applyParamWidgetValue(QWidget* aParamWidget)
{
  QComboBox* cbx = (QComboBox*)aParamWidget;
  mDest = folderMgr->find(cbx->currentText());
}

void KMFilterActionMove::argsFromString(const QString argsStr)
{
  mDest = (KMFolder*)folderMgr->find(argsStr);
}

const QString KMFilterActionMove::argsAsString(void) const
{
  if (mDest) resultStr = mDest->name();
  else resultStr = "";
  return resultStr;
}


//=============================================================================
// Skip all other filter rules
//=============================================================================
class KMFilterActionSkip: public KMFilterAction
{
public:
  KMFilterActionSkip();
  virtual const QString label(void) const;
  virtual bool process(KMMessage* msg, bool& stopIt);
  virtual void argsFromString(const QString argsStr);
  virtual const QString argsAsString(void) const;
  static KMFilterAction* newAction(void);
};

KMFilterActionSkip::KMFilterActionSkip(): KMFilterAction("skip rest")
{
}

const QString KMFilterActionSkip::label(void) const
{
  return i18n("skip rest");
}

KMFilterAction* KMFilterActionSkip::newAction(void)
{
  return (new KMFilterActionSkip);
}

bool KMFilterActionSkip::process(KMMessage*, bool& stopIt)
{
  stopIt = TRUE;
  return TRUE;
}

void KMFilterActionSkip::argsFromString(const QString)
{
}

const QString KMFilterActionSkip::argsAsString(void) const
{
  return "";
}


//=============================================================================
//
//   Filter  Action  Dictionary
//
//=============================================================================
void KMFilterActionDict::init(void)
{
  insert("transfer", i18n("transfer"),
	 KMFilterActionMove::newAction);
  insert("skip rest", i18n("skip rest"),
	 KMFilterActionSkip::newAction);
  // Register custom filter actions below this line.
}

KMFilterActionDict::KMFilterActionDict(): mList()
{
  init();
}

KMFilterActionDict::~KMFilterActionDict()
{
}

void KMFilterActionDict::insert(const QString aName, const QString aLabel,
				KMFilterActionNewFunc aFunc)
{
  KMFilterActionDesc* desc = new KMFilterActionDesc;
  desc->name = aName;
  desc->label = aLabel;
  desc->func = aFunc;
  mList.append(desc);
}

KMFilterAction* KMFilterActionDict::create(const QString name)
{
  KMFilterActionDesc* desc = find(name);
  if (desc) return desc->func();
  return NULL;
}

int KMFilterActionDict::indexOf(const QString aName)
{
  KMFilterActionDesc* desc;
  int i;

  for (i=0,desc=mList.first(); desc; i++,desc=mList.next())
    if (desc->name==aName) return i;
  return -1;
}

const QString KMFilterActionDict::labelOf(const QString aName)
{
  KMFilterActionDesc* desc = find(aName);
  if (desc) return desc->label;
  return "";
}

const QString KMFilterActionDict::nameOf(const QString aLabel)
{
  KMFilterActionDesc* desc;
  for (desc=mList.first(); desc; desc=mList.next())
    if (desc->label==aLabel) return desc->name;
  return "";
}

KMFilterActionDesc* KMFilterActionDict::find(const QString aName)
{
  KMFilterActionDesc* desc;
  for (desc=mList.first(); desc; desc=mList.next())
    if (desc->name==aName) return desc;
  return NULL;
}

const QString KMFilterActionDict::first(void)
{
  KMFilterActionDesc* desc = mList.first();
  if (!desc) return "";
  return desc->name;
}

const QString KMFilterActionDict::next(void)
{
  KMFilterActionDesc* desc = mList.next();
  if (!desc) return "";
  return desc->name;
}

const QString KMFilterActionDict::currentName(void)
{
  return mList.current()->name;
}

const QString KMFilterActionDict::currentLabel(void)
{
  return mList.current()->label;
}

KMFilterAction* KMFilterActionDict::currentCreate(void)
{
  return mList.current()->func();
}


//=============================================================================
//
//   Generic Filter  Action  Dialog
//
//=============================================================================
KMGFilterDlg::KMGFilterDlg(QWidget* parent, const char* name, bool modal,
			   WFlags f):
  KMGFilterDlgInherited(parent, name, modal, f)
{
  initMetaObject();
}

//-----------------------------------------------------------------------------
KMGFilterDlg::~KMGFilterDlg()
{
}


//-----------------------------------------------------------------------------
#include "kmfilteraction.moc"
