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
  KConfig* config = kapp->config();
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
  KConfig* config = kapp->config();
  QString grpName;
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
int KMFilterMgr::process(KMMessage* msg)
{
  KMFilter* filter;
  bool stopIt = FALSE;
  int status = -1;
  int result;

  for (filter=first(); !stopIt && filter; filter=next())
  {
    if (!filter->matches(msg)) continue;
    //    debug("KMFilterMgr: filter %s matches message %s", filter->name().data(),
    //    msg->subject().data());
    if (status < 0)
      status = 0;
    result = filter->execActions(msg, stopIt);
    if (result == 2) { // Critical error
      status = 2;
      break;
    }
    else if (result == 1) // Small problem encountered, keep copy of message
      status = 1;
  }

  if (status < 0) // No filters matched, keep copy of message
    status = 1;

  return status;
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

  // This only makes problems.....has to be changed later on
/*  if (mEditDialog)
  {
    mEditDialog->show();
    mEditDialog->raise();
  }
  else
  {*/
    mEditDialog = new KMFilterDlg;
    mEditDialog->show();
    debug( "openDialog" );
  //}
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
