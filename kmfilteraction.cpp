// kmfilteraction.cpp

#include "kmfilteraction.h"
#include "kmmessage.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include <klocale.h>
#include <kconfig.h>


//-----------------------------------------------------------------------------
KMFilterAction::KMFilterAction(const char* label): 
  KMFilterActionInherited(NULL, label)
{
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
  KMFilterActionMove(const char* label);
  virtual bool process(KMMessage* msg);
  virtual void installGUI(KMFilterActionGUI* caller);
  virtual void readConfig(KConfig* config);
  virtual void writeConfig(KConfig* config);

protected:
  KMFolder* mDest;
};

KMFilterActionMove::KMFilterActionMove(const char* aLabel): 
  KMFilterAction(aLabel)
{
  mDest = NULL;
}

bool KMFilterActionMove::process(KMMessage* msg)
{
  if (!mDest) return TRUE;
  mDest->addMsg(msg);
  return FALSE;
}

void KMFilterActionMove::installGUI(KMFilterActionGUI* caller)
{
  caller->addFolderList(nls->translate("To folder:"), &mDest);
}

void KMFilterActionMove::readConfig(KConfig* config)
{
  mDest = (KMFolder*)folderMgr->find(config->readEntry("Destination"));
}

void KMFilterActionMove::writeConfig(KConfig* config)
{
  config->writeEntry("Destination", mDest ? (const char*)mDest->name() : "");
}


//=============================================================================
//
//   Filter  Action  Dictionary
//
//=============================================================================
KMFilterActionDict::KMFilterActionDict(int initSize):
  KMFilterActionDictInherited(initSize, FALSE)
{
  setAutoDelete(TRUE);
  init();
}


//-----------------------------------------------------------------------------
KMFilterActionDict::~KMFilterActionDict()
{
}


//-----------------------------------------------------------------------------
void KMFilterActionDict::init(void)
{
  insert("transfer", new KMFilterActionMove(nls->translate("transfer")));
}
