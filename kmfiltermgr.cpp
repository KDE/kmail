// kmfiltermgr.cpp

// my header
#include "kmfiltermgr.h"

// other kmail headers
#include "kmfilterdlg.h"
#include "kmfolderindex.h"

// other KDE headers
#include <kdebug.h>
#include <klocale.h>

// other Qt headers
#include <qregexp.h>

// other headers
#include <assert.h>


//-----------------------------------------------------------------------------
KMFilterMgr::KMFilterMgr( bool popFilter )
  : QPtrList<KMFilter>(),
    mEditDialog( 0 ),
    bPopFilter( popFilter ),
    mShowLater( false )
{
  if (bPopFilter)
    kdDebug(5006) << "pPopFilter set" << endl;
  setAutoDelete(TRUE);
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
  KConfig* config = KMKernel::config();
  int numFilters;
  QString grpName;

  clear();

  KConfigGroupSaver saver(config, "General");

  if (bPopFilter) {
    numFilters = config->readNumEntry("popfilters",0);
    mShowLater = config->readNumEntry("popshowDLmsgs",0);
  } else {
    numFilters = config->readNumEntry("filters",0);
  }

  for ( int i=0 ; i < numFilters ; ++i ) {
    grpName.sprintf("%s #%d", (bPopFilter ? "PopFilter" : "Filter") , i);
    KConfigGroupSaver saver(config, grpName);
    KMFilter * filter = new KMFilter(config, bPopFilter);
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
  KConfig* config = KMKernel::config();

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
int KMFilterMgr::process(KMMessage* msg, FilterSet aSet, KMFilter *filter)
{
/*
QFile fileD0( "testdat_xx-kmfiltermngr-0" );
if( fileD0.open( IO_WriteOnly ) ) {
    QDataStream ds( &fileD0 );
    ds.writeRawBytes( msg->asString(), msg->asString().length() );
    fileD0.close();  // If data is 0 we just create a zero length file.
}
*/
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

      if ( (filter && (*it == filter)) ||
	   (!filter && (*it)->pattern()->matches( msg ) )) {
	  // filter matches

	  // remove msg from parent in case we want to move it; make
	  // sure we only do these things once:
	  if ( !msgTaken ) {
	    parent = msg->parent();
	    if (msg->parent())
		msg->parent()->removeMsg( msg->parent()->find( msg ) );
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
    int rc = 0;
    if ( msgTaken && parent && !msg->parent() ) {
      rc = parent->addMsg( msg );
    if (rc)
      kernel->emergencyExit( i18n("Unable to process messages (message locking synchronization failure?)" ))   ;
    }
/*
QFile fileD1( "testdat_xx-kmfiltermngr-1" );
if( fileD1.open( IO_WriteOnly ) ) {
    QDataStream ds( &fileD1 );
    ds.writeRawBytes( msg->asString(), msg->asString().length() );
    fileD1.close();  // If data is 0 we just create a zero length file.
}
*/
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
  assert( aFolder );

  int rc = aFolder->open();
  if (rc) return rc;

  mOpenFolders.append( aFolder );
  return 0;
}


//-----------------------------------------------------------------------------
void KMFilterMgr::openDialog( QWidget * )
{
  if( !mEditDialog )
  {
    //
    // We can't use the parent as long as the dialog is modeless
    // and there is one shared dialog for all top level windows.
    //
    mEditDialog = new KMFilterDlg( 0, "filterdialog", bPopFilter );
  }
  mEditDialog->show();
}


//-----------------------------------------------------------------------------
void KMFilterMgr::createFilter( const QCString & field, const QString & value )
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
void KMFilterMgr::dump(void)
{
  QPtrListIterator<KMFilter> it(*this);
  for ( it.toFirst() ; it.current() ; ++it )
  {
    kdDebug(5006) << (*it)->asString() << endl;
  }
}


//-----------------------------------------------------------------------------
void KMFilterMgr::endUpdate(void)
{
  emit filterListUpdated();
}

#include "kmfiltermgr.moc"
