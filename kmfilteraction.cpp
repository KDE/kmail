// kmfilteraction.cpp

#include "kmmessage.h"
#include "kmfilteraction.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include <klocale.h>
#include <kconfig.h>

static QString resultStr;


//-----------------------------------------------------------------------------
KMFilterAction::KMFilterAction(const QString name, const QString label): 
  KMFilterActionInherited(NULL, name)
{
  mLabel = label.copy();
}


//-----------------------------------------------------------------------------
KMFilterAction::~KMFilterAction()
{
}


//=============================================================================
//
//   Specific  Filter  Actions
//
//=============================================================================

//=============================================================================
// Move message to another mail folder
//
class KMFilterActionMove: public KMFilterAction
{
public:
  KMFilterActionMove(const QString name, const QString label);
  virtual bool process(KMMessage* msg, bool& stopIt);
  virtual void installGUI(KMGFilterDlg* caller);
  virtual void argsFromString(const QString argsStr);
  virtual const QString argsAsString(void) const;

protected:
  KMFolder* mDest;
};

KMFilterActionMove::KMFilterActionMove(const QString n, const QString l):
  KMFilterAction(n, l)
{
  mDest = NULL;
}

bool KMFilterActionMove::process(KMMessage* msg, bool&)
{
  if (!mDest) return TRUE;
  mDest->addMsg(msg);
  return FALSE;
}

void KMFilterActionMove::installGUI(KMGFilterDlg* caller)
{
  caller->addFolderList(nls->translate("To folder:"), &mDest);
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
//
class KMFilterActionSkip: public KMFilterAction
{
public:
  KMFilterActionSkip(const QString name, const QString label);
  virtual bool process(KMMessage* msg, bool& stopIt);
  virtual void installGUI(KMGFilterDlg* caller);
  virtual void argsFromString(const QString argsStr);
  virtual const QString argsAsString(void) const;
};

KMFilterActionSkip::KMFilterActionSkip(const QString n, const QString l):
  KMFilterAction(n, l)
{
}

bool KMFilterActionSkip::process(KMMessage*, bool& stopIt)
{
  stopIt = TRUE;
  return FALSE;
}

void KMFilterActionSkip::installGUI(KMGFilterDlg*)
{
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
KMFilterActionDict::KMFilterActionDict():
  KMFilterActionDictInherited(23, FALSE)
{
  setAutoDelete(TRUE);

  insert("transfer",
	 new KMFilterActionMove("transfer", nls->translate("transfer")));
  insert("skip",
	 new KMFilterActionSkip("skip", nls->translate("skip")));
  // Register custom filter actions below this line.
}


KMFilterActionDict::~KMFilterActionDict()
{
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
