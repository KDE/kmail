// kmfilteraction.cpp

#include "kmmessage.h"
#include "kmfilteraction.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include <klocale.h>
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
  static KMFilterAction* newAction(void);
protected:
  KMFolder* mDest;
};

KMFilterAction* KMFilterActionMove::newAction(void)
{
  return (new KMFilterActionMove);
}

const QString KMFilterActionMove::label(void) const
{
  return nls->translate("transfer");
}

KMFilterActionMove::KMFilterActionMove(): KMFilterAction("transfer")
{
  mDest = NULL;
}

bool KMFilterActionMove::process(KMMessage* msg, bool&)
{
  if (!mDest) return TRUE;
  mDest->addMsg(msg);
  return FALSE;
}

QWidget* KMFilterActionMove::createParamWidget(KMGFilterDlg* aParent)
{
  QString name;
  if (mDest) name = mDest->name();
  return aParent->createFolderCombo(name);
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

KMFilterActionSkip::KMFilterActionSkip(): KMFilterAction("skip")
{
}

const QString KMFilterActionSkip::label(void) const
{
  return nls->translate("skip rest");
}

KMFilterAction* KMFilterActionSkip::newAction(void)
{
  return (new KMFilterActionSkip);
}

bool KMFilterActionSkip::process(KMMessage*, bool& stopIt)
{
  stopIt = TRUE;
  return FALSE;
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
  insert("transfer", nls->translate("transfer"),
	 KMFilterActionMove::newAction);
  insert("skip rest", nls->translate("skip rest"),
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

const QString KMFilterActionDict::labelOf(const QString name)
{
  KMFilterActionDesc* desc = find(name);
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
