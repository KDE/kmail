// kmfiltermgr.cpp

// my header
#include "kmfiltermgr.h"

// other kmail headers
#include "kmfilter.h"
#include "kmfilterdlg.h"

// other KDE headers
#include <kapplication.h>
#include <kdebug.h>

// other Qt headers
#include <qregexp.h>
#include <qstringlist.h>
#include <qstring.h>

// other headers
#include <assert.h>


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
  cleanup();
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

  KConfigGroupSaver saver(config, "General");

  if (bPopFilter) {
    numFilters = config->readNumEntry("popfilters",0);
    mShowLater = config->readNumEntry("popshowDLmsgs",0);
  } else {
    numFilters = config->readNumEntry("filters",0);
  }

  for (i=0; i<numFilters; i++) {
    grpName.sprintf("%s #%d", (bPopFilter ? "PopFilter" : "Filter") , i);
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

  // first, delete all groups:
  QStringList filterGroups =
    config->groupList().grep( QRegExp( bPopFilter ? "PopFilter #\\d+" : "Filter #\\d+" ) );
  for ( QStringList::Iterator it = filterGroups.begin() ;
	it != filterGroups.end() ; ++it )
    config->deleteGroup( *it );

  // Now, write out the new stuff:
  int i = 0;
  QString grpName;
  for ( QPtrListIterator<KMFilter> it(*this) ; it.current() ; ++it )
    if ( !(*it)->isEmpty() ) {
      if ( bPopFilter )
	grpName.sprintf("PopFilter #%d", i);
      else
	grpName.sprintf("Filter #%d", i);
      KConfigGroupSaver saver(config, grpName);
      (*it)->writeConfig(config);
      ++i;
    }

  KConfigGroupSaver saver(config, "General");
  if (bPopFilter) { 
    config->writeEntry("popfilters", i);
    config->writeEntry("popshowDLmsgs", mShowLater);
  } else
    config->writeEntry("filters", i);

  if (withSync) config->sync();
}


//-----------------------------------------------------------------------------
int KMFilterMgr::process(KMMessage* msg, FilterSet aSet)
{
  if(bPopFilter) {
    QPtrListIterator<KMFilter> it(*this);
    for (it.toFirst() ; it.current() ; ++it) {
      if ((*it)->pattern()->matches(msg))
        return (*it)->action();
    }
    return NoAction;
  } else {
    if ( aSet == NoSet ) {
      kdDebug(5006) << "KMFilterMgr: process() called with not filter set selected"
		    << endl;
      return 1;
    }

    bool stopIt = false;
    int status = -1;
    bool msgTaken = false; // keep track of whether we removeMsg'ed already

    KMFolder * parent=0;

    QPtrListIterator<KMFilter> it(*this);
    for ( it.toFirst() ; !stopIt && it.current() ; ++it ) {

      if ( ( (aSet&Outbound) && (*it)->applyOnOutbound() ) ||
  	   ( (aSet&Inbound)  && (*it)->applyOnInbound() ) ||
	   ( (aSet&Explicit) && (*it)->applyOnExplicit() ) ) {
	// filter is applicable

        if ( (*it)->pattern()->matches( msg ) ) {
	  // filter matches

	  // remove msg from parent in case we want to move it; make
	  // sure we only do these things once:
	  if ( !msgTaken ) {
	    parent = msg->parent();
	    if ( parent )
	      parent->removeMsg( msg );
	    msg->setParent( 0 );
	    msgTaken = true;
	  }

	  // execute actions:
	  switch ( (*it)->execActions(msg, stopIt) ) {
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

    // readd the message if it wasn't moved:
    if ( msgTaken && parent && !msg->parent() )
      parent->addMsg( msg );

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
