// kmfilteraction.cpp

#include "kmmessage.h"
#include "kmfilteraction.h"
#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include "kmsender.h"
#include "kmidentity.h"
#include <kapp.h>
#include <kconfig.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qguardedptr.h>
#include <ktempfile.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h> //for alarm (sven)
#include <klocale.h>
#include <sys/types.h>
#include <sys/stat.h>

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
  return kernel->filterMgr()->tempOpenFolder(aFolder);
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
  QGuardedPtr<KMFolder> mDest;
  QValueList<QGuardedPtr<KMFolder> > gfolders;
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
  if (msg->parent())
    return 0;
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

  gfolders.clear();

  kernel->folderMgr()->createFolderList( &str, &gfolders );
  cbx = aParent->createFolderCombo( &str, &gfolders, mDest );

  return cbx;
}

void KMFilterActionMove::applyParamWidgetValue(QWidget* aParamWidget)
{
  QComboBox* cbx = (QComboBox*)aParamWidget;
  if ((cbx->currentItem() >= 0) && (cbx->currentItem() < (int)gfolders.count()))
    mDest = (*gfolders.at(cbx->currentItem()));
  else
    mDest = 0;
}

void KMFilterActionMove::argsFromString(const QString argsStr)
{
  mDest = (KMFolder*)kernel->folderMgr()->findIdString(argsStr);
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
  if (!kernel->msgSender()->send(msg))
  {
    debug("KMFilterActionForward: could not forward message (sending failed)");
    return 1; // error: couldn't send
  }
  return -1;
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
  KMFilterActionExec(const char* name = "execute");
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
  return i18n("execute");
}
 
KMFilterActionExec::KMFilterActionExec(const char* aName)
  : KMFilterAction(aName)
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
  if (rc & 255)
    return 1; 
  else
    return -1;
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
// External message filter: executes a shell command with message
// on stdin; altered message is expected on stdout.
//=============================================================================
class KMFilterActionExtFilter: public KMFilterActionExec
{
public:
  KMFilterActionExtFilter(const char* name = "filter app");
  virtual const QString label(void) const;
  static KMFilterAction* newAction(void);
  virtual int process(KMMessage* msg, bool& stopIt);
};
 
KMFilterAction* KMFilterActionExtFilter::newAction(void)
{
  return (new KMFilterActionExtFilter);
}
 
const QString KMFilterActionExtFilter::label(void) const
{
  return i18n("filter app");
}
 
KMFilterActionExtFilter::KMFilterActionExtFilter(const char* aName)
  : KMFilterActionExec(aName)
{
}
 
int KMFilterActionExtFilter::process(KMMessage* aMsg, bool& stop)
{
  int rc=0, len;
  QString msgText, origCmd;
  char buf[8192];
  FILE *fh;
  bool ok = TRUE;
  KTempFile inFile("/tmp/kmail-filter", "in");
  KTempFile outFile("/tmp/kmail-filter", "out");

  if (mCmd.isEmpty()) return 1;

  // write message to file
  fh = inFile.fstream();
  if (fh)
  {
    msgText = aMsg->asString();
    if (!fwrite(msgText, msgText.length(), 1, fh)) ok = FALSE;
    inFile.close();
  }
  else ok = FALSE;

  outFile.close();
  if (ok)
  {
    // execute filter
    origCmd = mCmd;
    mCmd = mCmd + " <" + inFile.name() + " >" + outFile.name();
    rc = KMFilterActionExec::process(aMsg, stop);
    mCmd = origCmd;

    // read altered message
    fh = fopen(outFile.name(), "r");
    if (fh)
    {
      msgText = "";
      while (1)
      {
	len = fread(buf, 1, 1023, fh);
	if (len <= 0) break;
	buf[len] = 0;
	msgText += buf;
      }
      outFile.close();
      if (!msgText.isEmpty()) aMsg->fromString(msgText);
    }
    else ok = FALSE;
  }

  inFile.unlink();
  outFile.unlink();

  return rc;
}

//=============================================================================
// Specify Identity to be used when replying to a message
//=============================================================================
class KMFilterActionIdentity: public KMFilterAction
{
public:
  KMFilterActionIdentity();
  virtual const QString label(void) const;
  virtual int process(KMMessage* msg, bool& stopIt);
  virtual QWidget* createParamWidget(KMGFilterDlg* parent);
  virtual void applyParamWidgetValue(QWidget* paramWidget);
  virtual void argsFromString(const QString argsStr);
  virtual const QString argsAsString(void) const;
  static KMFilterAction* newAction(void);
protected:
  QStringList ids;
  QString id;
};

KMFilterAction* KMFilterActionIdentity::newAction(void)
{
  return (new KMFilterActionIdentity);
}

const QString KMFilterActionIdentity::label(void) const
{
  return i18n("set identity");
}

KMFilterActionIdentity::KMFilterActionIdentity(): KMFilterAction("set identity")
{
  id = i18n( "unknown" );
}

int KMFilterActionIdentity::process(KMMessage* msg, bool& )
{
  msg->setHeaderField( "X-KMail-Identity", id );
  return -1;
}

QWidget* KMFilterActionIdentity::createParamWidget(KMGFilterDlg* aParent)
{
  QStringList ids = KMIdentity::identities();
  QComboBox *cbx = aParent->createCombo( &ids, id );

  return cbx;
}

void KMFilterActionIdentity::applyParamWidgetValue(QWidget* aParamWidget)
{
  QComboBox* cbx = (QComboBox*)aParamWidget;
  id = cbx->currentText();
}

void KMFilterActionIdentity::argsFromString(const QString argsStr)
{
  id = argsStr;
}

const QString KMFilterActionIdentity::argsAsString(void) const
{
  return id;
}


//=============================================================================
// Specify mail transport (smtp server) to be used when replying to a message
//=============================================================================
class KMFilterActionTransport: public KMFilterAction
{
public:
  KMFilterActionTransport();
  virtual const QString label(void) const;
  virtual int process(KMMessage* msg, bool& stopIt);
  virtual QWidget* createParamWidget(KMGFilterDlg* parent);
  virtual void applyParamWidgetValue(QWidget* paramWidget);
  virtual void argsFromString(const QString argsStr);
  virtual const QString argsAsString(void) const;
  static KMFilterAction* newAction(void);
protected:
  QString mTransport;
};

KMFilterAction* KMFilterActionTransport::newAction(void)
{
  return (new KMFilterActionTransport);
}

const QString KMFilterActionTransport::label(void) const
{
  return i18n("set transport");
}

KMFilterActionTransport::KMFilterActionTransport(): KMFilterAction("set transport")
{
  mTransport = kernel->msgSender()->transportString();
}

int KMFilterActionTransport::process(KMMessage* msg, bool& )
{
  msg->setHeaderField( "X-KMail-Transport", mTransport );
  return -1;
}

QWidget* KMFilterActionTransport::createParamWidget(KMGFilterDlg* aParent)
{
  QLineEdit* edt;
  edt = aParent->createEdit(mTransport);
  return edt;
}
 
void KMFilterActionTransport::applyParamWidgetValue(QWidget* aParamWidget)
{
  QLineEdit* w = (QLineEdit*)aParamWidget;
  mTransport = w->text();
}

void KMFilterActionTransport::argsFromString(const QString argsStr)
{
  mTransport = argsStr;
}

const QString KMFilterActionTransport::argsAsString(void) const
{
  return mTransport;
}


//=============================================================================
//
//   Filter  Action  Dictionary
//
//=============================================================================
void KMFilterActionDict::init(void)
{
  insert("set identity", i18n("set identity"),
	 KMFilterActionIdentity::newAction);
  insert("set transport", i18n("set transport"),
	 KMFilterActionTransport::newAction);
  insert("transfer", i18n("transfer"),
	 KMFilterActionMove::newAction);
  insert("forward", i18n("forward to"),
         KMFilterActionForward::newAction);
  insert("execute", i18n("execute"),
         KMFilterActionExec::newAction);
  insert("filter app", i18n("filter app"),
         KMFilterActionExtFilter::newAction);
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
