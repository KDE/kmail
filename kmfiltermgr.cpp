// kmfiltermgr.cpp

#include <kapplication.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>

#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
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

  KConfigGroupSaver saver(config, "General");
  numFilters = config->readNumEntry("filters",0);

  for (i=0; i<numFilters; i++)
  {
    grpName.sprintf("Filter #%d", i);
    KConfigGroupSaver saver(config, grpName);
    filter = new KMFilter(config);
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

  QPtrListIterator<KMFilter> it(*this);
  it.toFirst();
  while ( it.current() ) {
    if ( !(*it)->isEmpty() ) {
      grpName.sprintf("Filter #%d", i);
      KConfigGroupSaver saver(config, grpName);
      (*it)->writeConfig(config);
      ++i;
    }
    ++it;
  }
  KConfigGroupSaver saver(config, "General");
  config->writeEntry("filters", i);

  if (withSync) config->sync();
}


//-----------------------------------------------------------------------------
int KMFilterMgr::process(KMMessage* msg, FilterSet aSet)
{
  if (!aSet) {
    kdDebug(5006) << "KMFilterMgr: process() called with not filter set selected"
	      << endl;
    return 1;
  }

  bool stopIt = FALSE;
  int status = -1;
  KMFilter::ReturnCode result;

  // here we must treat X-KMail-Fcc on outgoing messages
  if ( ( !msg->fcc().isEmpty() ) && ( aSet == Outbound ) )
  {
      KMFolder *f = kernel->folderMgr()->findIdString( msg->fcc() );

      if ( f != 0 )
      {
          kdDebug(5006) << "KMFilterMgr::process: moving to " << f->label() << endl;
          KMFilterAction::tempOpenFolder( f );
          f->moveMsg( msg );
          status = 0; // Message moved to a folder.
      }
      else
      {
          KMessageBox::information( NULL, i18n( "Could not save to '%1'. Will process filters normally." )
                             .arg( msg->fcc() ) );
          // we remove X-KMail-Fcc so that the filter will take over
          msg->setFcc( QString::null );
      }
  }

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
    mEditDialog = new KMFilterDlg( 0, "filterdialog" );
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
void KMFilterMgr::dump(void)
{
  QPtrListIterator<KMFilter> it(*this);
  for ( it.toFirst() ; it.current() ; ++it )
  {
    kdDebug(5006) << (*it)->asString() << endl;
  }
}
