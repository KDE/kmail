// kmfiltermgr.cpp

#include <kapplication.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>

#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmfilterdlg.h"
#include "kmmessage.h"

//todo
#include <kdebug.h>


//-----------------------------------------------------------------------------
KMFilterMgr::KMFilterMgr(bool popFilter): KMFilterMgrInherited(),
  bPopFilter(popFilter)
{
  if (bPopFilter)
    kdDebug(5006) << "pPopFilter set" << endl;
  setAutoDelete(TRUE);
  mEditDialog = NULL;
  mShowLater = false;
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
  QString group;

  clear();

  KConfigGroupSaver saver(config, "General");

  if (bPopFilter) {
    numFilters = config->readNumEntry("popfilters",0);
    mShowLater = config->readNumEntry("popshowDLmsgs",0);
    group = "PopFilter";
  }
  else {
    numFilters = config->readNumEntry("filters",0);
    group = "Filter";
  }

  for (i=0; i<numFilters; i++)
  {
    grpName.sprintf("%s #%d", group.latin1(), i);
    KConfigGroupSaver saver(config, grpName);
    filter = new KMFilter(config, bPopFilter);
    filter->purify();
    if ( filter->isEmpty() ) {
      kdDebug(5006) << "KMFilter::readConfig: filter\n" << filter->asString()
		<< "is empty!" << endl;
      delete filter;
    } else
      append(filter);
  }
}


//-----------------------------------------------------------------------------
void KMFilterMgr::writeConfig(bool withSync)
{
  KConfig* config = kapp->config();
  QString grpName;
  int i = 0;
  QString group;

  if (bPopFilter) 
    group = "PopFilter";
  else 
    group = "Filter";

  QPtrListIterator<KMFilter> it(*this);
  it.toFirst();
  while ( it.current() ) {
    if ( !(*it)->isEmpty() ) {
      grpName.sprintf("%s #%d", group.latin1(), i);
      //grpName.sprintf("Filter #%d", i);
      KConfigGroupSaver saver(config, grpName);
      (*it)->writeConfig(config);
      ++i;
    }
    ++it;
  }
  KConfigGroupSaver saver(config, "General");
  if (bPopFilter) { 
    config->writeEntry("popfilters", i);
    config->writeEntry("popshowDLmsgs", mShowLater);
  }
  else  {
    config->writeEntry("filters", i);
  }

  if (withSync) config->sync();
}


//-----------------------------------------------------------------------------
int KMFilterMgr::process(KMMessage* msg, FilterSet aSet)
{
  if(bPopFilter) {
    QPtrListIterator<KMFilter> it(*this);
    for (it.toFirst() ; it.current() ; ++it)
    {
      if ((*it)->pattern()->matches(msg)) {

        return (*it)->action();
  
      }
    }
    return NoAction;
  }
  else {
    if (!aSet) {
      kdDebug(5006) << "KMFilterMgr: process() called with not filter set selected"
	        << endl;
      return 1;
    }

    bool stopIt = FALSE;
    int status = -1;
    KMFilter::ReturnCode result;
 
    QPtrListIterator<KMFilter> it(*this);
    for (it.toFirst() ; !stopIt && it.current() ; ++it)
    {
      if ( aSet&All
  	   || ( (aSet&Outbound) && (*it)->applyOnOutbound() )
  	   || ( (aSet&Inbound)  && (*it)->applyOnInbound() ) ) {

        if ((*it)->pattern()->matches(msg)) {

  	  result = (*it)->execActions(msg, stopIt);

	  switch ( result ) {
	  case KMFilter::CriticalError:
	    // Critical error - immediate return
  	    return 2;
	  case KMFilter::MsgExpropriated:
	    // Message saved in a folder
	    status = 0;
	  default:
	    break;
	  }
        }
      }
    }

    if (status < 0) // No filters matched, keep copy of message
      status = 1;

    return status;
  }
}



//-----------------------------------------------------------------------------
void KMFilterMgr::cleanup(void)
{
  QPtrListIterator<KMFolder> it(mOpenFolders);
  for ( it.toFirst() ; it.current() ; ++it )
    (*it)->close();
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
void KMFilterMgr::openDialog( QWidget *parent )
{
  if( !mEditDialog )
  {
    //
    // We can't use the parent as long as the dialog is modeless
    // and there is one shared dialog for all top level windows.
    //
    (void)parent;
      mEditDialog = new KMFilterDlg( 0, "filterdialog", bPopFilter );
  }
  mEditDialog->show();
}


//-----------------------------------------------------------------------------
void KMFilterMgr::createFilter( const QCString field, const QString value )
{
  openDialog( 0 );
  mEditDialog->createFilter( field, value );
}


//-----------------------------------------------------------------------------
bool KMFilterMgr::folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder)
{
  bool rem = FALSE;

  QPtrListIterator<KMFilter> it(*this);
  for ( it.toFirst() ; it.current() ; ++it )
    if ( (*it)->folderRemoved(aFolder, aNewFolder) ) rem=TRUE;

  return rem;
}


//-----------------------------------------------------------------------------
void KMFilterMgr::setShowLaterMsgs(bool aShow)
{
  mShowLater = aShow;
}


//-----------------------------------------------------------------------------
bool KMFilterMgr::showLaterMsgs()
{
  return mShowLater;
}


//-----------------------------------------------------------------------------
void KMFilterMgr::dump(void)
{
  QPtrListIterator<KMFilter> it(*this);
  for ( it.toFirst() ; it.current() ; ++it )
  {
    kdDebug(5006) << (*it)->asString() << endl;
  }
}
