// kmfilteraction.cpp

#include "kmmessage.h"
#include "kmfilteraction.h"
#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include "kmsender.h"
#include <kapp.h>
#include <kconfig.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h> //for alarm (sven)
#include <klocale.h>

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
  virtual int process(KMMessage* msg, bool& stopIt);
  virtual QWidget* createParamWidget(KMGFilterDlg* parent);
  virtual void applyParamWidgetValue(QWidget* paramWidget);
  virtual void argsFromString(const QString argsStr);
  virtual const QString argsAsString(void) const;
  virtual bool folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder);
  static KMFilterAction* newAction(void);
protected:
  KMFolder* mDest;
  QList<KMFolder> folders;
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

int KMFilterActionMove::process(KMMessage* msg, bool&stopIt)
{
  if (!mDest) return TRUE;
  KMFilterAction::tempOpenFolder(mDest);
  if (mDest->moveMsg(msg) == 0) 
    return 0; // ok, added
  else
  {
    debug ("KMfilteraction - couldn't move msg");
    stopIt = TRUE;
    return 2; // critical error: couldn't add
  }
}

QWidget* KMFilterActionMove::createParamWidget(KMGFilterDlg* aParent)
{
  QString name;
  QComboBox* cbx;
  QStringList str;

  folders.clear();
  folderMgr->createFolderList( &str, &folders );
  cbx = aParent->createFolderCombo( &str, &folders, mDest );
  return cbx;
}

void KMFilterActionMove::applyParamWidgetValue(QWidget* aParamWidget)
{
  QComboBox* cbx = (QComboBox*)aParamWidget;
  if ((cbx->currentItem() >= 0) && (cbx->currentItem() < (int)folders.count()))
    mDest = folders.at(cbx->currentItem());
  else
    mDest = 0;
}

void KMFilterActionMove::argsFromString(const QString argsStr)
{
  mDest = (KMFolder*)folderMgr->findIdString(argsStr);
}

const QString KMFilterActionMove::argsAsString(void) const
{
  if (mDest) resultStr = mDest->idString();
  else resultStr = "";
  return resultStr;
}

//=============================================================================
// Forward message to another user
//=============================================================================
class KMFilterActionForward: public KMFilterAction
{
public:
  KMFilterActionForward();
  virtual const QString label(void) const;
  virtual int process(KMMessage* msg, bool& stopIt);
  virtual QWidget* createParamWidget(KMGFilterDlg* parent);
  virtual void applyParamWidgetValue(QWidget* paramWidget);
  virtual void argsFromString(const QString argsStr);
  virtual const QString argsAsString(void) const;
  static KMFilterAction* newAction(void);
protected:
  QString mTo;
};

KMFilterAction* KMFilterActionForward::newAction(void)
{
  return (new KMFilterActionForward);
}
 
const QString KMFilterActionForward::label(void) const
{
  return i18n("forward to");
}

KMFilterActionForward::KMFilterActionForward(): KMFilterAction("forward")
{
}
 
int KMFilterActionForward::process(KMMessage* aMsg, bool& /*stop*/)
{
  KMMessage* msg;
  if (mTo.isEmpty()) return TRUE;
  msg = aMsg->createForward();
  msg->setTo(mTo);
  if (!msgSender->send(msg))
  {
    debug("KMFilterActionForward: could not forward message (sending failed)");
    return 1; // error: couldn't send
  }
  return 0;
}
 
QWidget* KMFilterActionForward::createParamWidget(KMGFilterDlg* aParent)
{
  QLineEdit* edt;
  edt = aParent->createEdit(mTo);
  return edt;
}
 
void KMFilterActionForward::applyParamWidgetValue(QWidget* aParamWidget)
{
  QLineEdit* w = (QLineEdit*)aParamWidget;
  mTo = w->text();
}
 
void KMFilterActionForward::argsFromString(const QString argsStr)
{
  mTo = argsStr;
}
 
const QString KMFilterActionForward::argsAsString(void) const
{
  return mTo;
}
 
//=============================================================================
// Execute a shell command
//=============================================================================
class KMFilterActionExec:public KMFilterAction
{
public:
  KMFilterActionExec();
  virtual const QString label(void) const;
  virtual int process(KMMessage* msg, bool& stopIt);
  virtual QWidget* createParamWidget(KMGFilterDlg* parent);
  virtual void applyParamWidgetValue(QWidget* paramWidget);
  virtual void argsFromString(const QString argsStr);
  virtual const QString argsAsString(void) const;
  static KMFilterAction* newAction(void);
  static void dummySigHandler(int);
protected:
  QString mCmd;
};
 
KMFilterAction* KMFilterActionExec::newAction(void)
{
  return (new KMFilterActionExec);
}
 
const QString KMFilterActionExec::label(void) const
{
  return i18n("forward to");
}
 
KMFilterActionExec::KMFilterActionExec(): KMFilterAction("execute")
{
}
 
void KMFilterActionExec::dummySigHandler(int)
{
}
 
int KMFilterActionExec::process(KMMessage* /*aMsg*/, bool& /*stop*/)
{
  void (*oldSigHandler)(int);
  int rc;
  if (mCmd.isEmpty()) return TRUE;
  oldSigHandler = signal(SIGALRM, &KMFilterActionExec::dummySigHandler);
  alarm(30);
  rc = system(mCmd);
  alarm(0);
  signal(SIGALRM, oldSigHandler);
  if (rc & 255) // sanders: I don't get this it seems to be the wrong way
    return 0;   //          around to me.
  else
    return 1;
}

QWidget* KMFilterActionExec::createParamWidget(KMGFilterDlg* aParent)
{
  QLineEdit* edt;
  edt = aParent->createEdit(mCmd);
  return edt;
}
 
void KMFilterActionExec::applyParamWidgetValue(QWidget* aParamWidget)
{
  QLineEdit* w = (QLineEdit*)aParamWidget;
  mCmd = w->text();
}
 
void KMFilterActionExec::argsFromString(const QString argsStr)
{
  mCmd = argsStr;
}
 
const QString KMFilterActionExec::argsAsString(void) const
{
  return mCmd;
}

//=============================================================================
// Skip all other filter rules
//=============================================================================
class KMFilterActionSkip: public KMFilterAction
{
public:
  KMFilterActionSkip();
  virtual const QString label(void) const;
  virtual int process(KMMessage* msg, bool& stopIt);
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

int KMFilterActionSkip::process(KMMessage*, bool& stopIt)
{
  stopIt = TRUE;
  return 1;
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
  insert("forward", i18n("forward to"),
         KMFilterActionForward::newAction);
  insert("execute", i18n("execute"),
         KMFilterActionExec::newAction);
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
