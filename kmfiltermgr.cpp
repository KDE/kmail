// kmfiltermgr.cpp

#include <kapp.h>
#include <kconfig.h>

#include "kmglobal.h"
#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "kmfilter.h"
#include "kmfilterdlg.h"
#include "kmmessage.h"


//-----------------------------------------------------------------------------
KMFilterMgr::KMFilterMgr(): KMFilterMgrInherited()
{
  setAutoDelete(TRUE);
  mEditDialog = NULL;
}


//-----------------------------------------------------------------------------
KMFilterMgr::~KMFilterMgr()
{
  writeConfig(FALSE);
}


//-----------------------------------------------------------------------------
void KMFilterMgr::readConfig(void)
{
  KConfig* config = kapp->getConfig();
  int i, numFilters;
  QString grpName(64);
  KMFilter* filter;

  clear();

  config->setGroup("General");
  numFilters = config->readNumEntry("filters",0);

  for (i=0; i<numFilters; i++)
  {
    grpName.sprintf("Filter #%d", i);
    config->setGroup(grpName);
    filter = new KMFilter(config);
    append(filter);
  }
}


//-----------------------------------------------------------------------------
void KMFilterMgr::writeConfig(bool withSync)
{
  KConfig* config = kapp->getConfig();
  QString grpName(64);
  KMFilter* filter;
  int i;

  config->setGroup("General");
  config->writeEntry("filters", count());

  for (i=0, filter=first(); filter; filter=next(), i++)
  {
    grpName.sprintf("Filter #%d", i);
    config->setGroup(grpName);
    filter->writeConfig(config);
  }

  if (withSync) config->sync();
}


//-----------------------------------------------------------------------------
bool KMFilterMgr::process(KMMessage* msg)
{
  KMFilter* filter;
  bool stopIt = FALSE;
  bool stillOwner = TRUE;

  for (filter=first(); !stopIt && filter; filter=next())
  {
    if (!filter->matches(msg)) continue;
#ifdef DEBUG
    debug("KMFilterMgr: filter %s matches message %s", filter->name().data(),
	  msg->subject().data());
#endif
    if (!filter->execActions(msg, stopIt)) stillOwner = FALSE;
  }
  return stillOwner;
}


//-----------------------------------------------------------------------------
void KMFilterMgr::cleanup(void)
{
  KMFolder* fld;

  folderMgr->contentsChanged();

  for (fld=mOpenFolders.first(); fld; fld=mOpenFolders.next())
    if (fld) fld->close();

  mOpenFolders.clear();
}


//-----------------------------------------------------------------------------
int KMFilterMgr::tempOpenFolder(KMFolder* aFolder)
{
  assert(aFolder!=NULL);

  int rc = aFolder->open();
  if (rc) return rc;

  mOpenFolders.append(aFolder);
  return rc;
}


//-----------------------------------------------------------------------------
void KMFilterMgr::dialogClosed(void)
{
  mEditDialog = NULL;
}


//-----------------------------------------------------------------------------
void KMFilterMgr::openDialog(void)
{
  if (mEditDialog)
  {
    mEditDialog->show();
    mEditDialog->raise();
  }
  else
  {
    mEditDialog = new KMFilterDlg;
    mEditDialog->show();
  }
}


//-----------------------------------------------------------------------------
bool KMFilterMgr::folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder)
{
  KMFilter* filter;
  bool rem = FALSE;

  for (filter=first(); filter; filter=next())
    if (filter->folderRemoved(aFolder, aNewFolder)) rem=TRUE;

  return rem;
}


//-----------------------------------------------------------------------------
void KMFilterMgr::dump(void)
{
  KMFilter* filter;
  int i;

  for (i=0, filter=first(); filter; filter=next(), i++)
  {
    debug(filter->asString());
  }
}
