// kmfiltermgr.cpp

#include <kapp.h>
#include <kconfig.h>

#include "kmglobal.h"
#include "kmfiltermgr.h"
#include "kmfilter.h"
#include "kmfilterdlg.h"


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
  QString grpName;
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
  QString grpName;
  KMFilter* filter;
  int i;

  config->setGroup("General");
  config->writeEntry("filters", count());

  for (filter=first(); filter; filter=next())
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
    if (!filter->execActions(msg, stopIt)) stillOwner = FALSE;
  }
  return stillOwner;
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
