// -*- mode: C++; c-file-style: "gnu" -*-
// kmfiltermgr.cpp

// my header
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfiltermgr.h"

// other kmail headers
#include "filterlog.h"
using KMail::FilterLog;
#include "kmfilterdlg.h"
#include "kmfolderindex.h"
#include "messageproperty.h"
using KMail::MessageProperty;

// other KDE headers
#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>

// other Qt headers
#include <qregexp.h>

// other headers
#include <assert.h>


//-----------------------------------------------------------------------------
KMFilterMgr::KMFilterMgr( bool popFilter )
  : QPtrList<KMFilter>(),
    mEditDialog( 0 ),
    bPopFilter( popFilter ),
    mShowLater( false ),
    mRefCount( 0 )
{
  if (bPopFilter)
    kdDebug(5006) << "pPopFilter set" << endl;
  setAutoDelete(TRUE);
  connect( kmkernel, SIGNAL( folderRemoved( KMFolder* ) ),
           this, SLOT( slotFolderRemoved( KMFolder* ) ) );
}


//-----------------------------------------------------------------------------
KMFilterMgr::~KMFilterMgr()
{
  deref(true);
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
#ifndef NDEBUG
      kdDebug(5006) << "KMFilter::readConfig: filter\n" << filter->asString()
		<< "is empty!" << endl;
#endif
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


int KMFilterMgr::processPop( KMMessage * msg ) const {
  for ( QPtrListIterator<KMFilter> it( *this ) ; it.current() ; ++it )
    if ( (*it)->pattern()->matches( msg ) )
      return (*it)->action();
  return NoAction;
}

bool KMFilterMgr::beginFiltering(KMMsgBase *msgBase) const
{
  if (MessageProperty::filtering( msgBase ))
    return false;
  MessageProperty::setFiltering( msgBase, true );
  MessageProperty::setFilterFolder( msgBase, 0 );
  if ( FilterLog::instance()->isLogging() ) {
    FilterLog::instance()->addSeparator();
  }
  return true;
}

int KMFilterMgr::moveMessage(KMMessage *msg) const
{
  if (MessageProperty::filterFolder(msg)->moveMsg( msg ) == 0) {
    if ( kmkernel->folderIsTrash( MessageProperty::filterFolder( msg )))
      KMFilterAction::sendMDN( msg, KMime::MDN::Deleted );
  } else {
    kdDebug(5006) << "KMfilterAction - couldn't move msg" << endl;
    return 2;
  }
  return 0;
}

void KMFilterMgr::endFiltering(KMMsgBase *msgBase) const
{
  KMFolder *parent = msgBase->parent();
  if ( parent ) {
    if ( parent == MessageProperty::filterFolder( msgBase ) ) {
      parent->take( parent->find( msgBase ) );
    }
    else if ( ! MessageProperty::filterFolder( msgBase ) ) {
      int index = parent->find( msgBase );
      KMMessage *msg = parent->getMsg( index );
      parent->take( index );
      parent->addMsgKeepUID( msg );
    }
  }
  MessageProperty::setFiltering( msgBase, false );
}

int KMFilterMgr::process( KMMessage * msg, const KMFilter * filter ) {
  if ( !msg || !filter || !beginFiltering( msg ))
    return 1;
  bool stopIt = false;
  int result = 1;

  if ( FilterLog::instance()->isLogging() ) {
    QString logText( i18n( "<b>Evaluating filter rules:</b> " ) );
    logText.append( filter->pattern()->asString() );
    FilterLog::instance()->add( logText, FilterLog::patternDesc );
  }

  if (filter->pattern()->matches( msg )) {
    if ( FilterLog::instance()->isLogging() ) {
      FilterLog::instance()->add( i18n( "<b>Filter rules have matched.</b>" ), 
                                  FilterLog::patternResult );
    }
    if (filter->execActions( msg, stopIt ) == KMFilter::CriticalError)
      return 2;

    KMFolder *folder = MessageProperty::filterFolder( msg );

    endFiltering( msg );
    if (folder) {
      tempOpenFolder( folder );
      result = folder->moveMsg( msg );
    }
  } else {
    endFiltering( msg );
    result = 1;
  }
  return result;
}

int KMFilterMgr::process( KMMessage * msg, FilterSet set ) {
  if ( bPopFilter )
    return processPop( msg );

  if ( set == NoSet ) {
    kdDebug(5006) << "KMFilterMgr: process() called with not filter set selected"
		  << endl;
    return 1;
  }

  bool stopIt = false;
  bool atLeastOneRuleMatched = false;

  if (!beginFiltering( msg ))
    return 1;
  for ( QPtrListIterator<KMFilter> it(*this) ; !stopIt && it.current() ; ++it ) {

    if ( ( (set&Outbound) && (*it)->applyOnOutbound() ) ||
         ( (set&Inbound)  && (*it)->applyOnInbound() ) ||
         ( (set&Explicit) && (*it)->applyOnExplicit() ) ) {
        // filter is applicable

      if ( FilterLog::instance()->isLogging() ) {
        QString logText( i18n( "<b>Evaluating filter rules:</b> " ) );
        logText.append( (*it)->pattern()->asString() );
        FilterLog::instance()->add( logText, FilterLog::patternDesc );
      }
      if ( (*it)->pattern()->matches( msg ) ) {
        // filter matches
        if ( FilterLog::instance()->isLogging() ) {
          FilterLog::instance()->add( i18n( "<b>Filter rules have matched.</b>" ), 
                                      FilterLog::patternResult );
        }
        atLeastOneRuleMatched = true;
        // execute actions:
        if ( (*it)->execActions(msg, stopIt) == KMFilter::CriticalError )
          return 2;
      }
    }
  }

  KMFolder *folder = MessageProperty::filterFolder( msg );
  /* endFilter does a take() and addButKeepUID() to ensure the changed 
   * message is on disk. This is unnessecary if nothing matched, so just 
   * reset state and don't update the listview at all. */
  if ( atLeastOneRuleMatched )
    endFiltering( msg );
  else
    MessageProperty::setFiltering( msg, false );
  if (folder) {
    tempOpenFolder( folder );
    folder->moveMsg(msg);
    return 0;
  }
  return 1;
}


//-----------------------------------------------------------------------------
void KMFilterMgr::ref(void)
{
  mRefCount++;
}

//-----------------------------------------------------------------------------
void KMFilterMgr::deref(bool force)
{
  if (!force)
    mRefCount--;
  if (mRefCount < 0)
    mRefCount = 0;
  if (mRefCount && !force)
    return;
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
const QString KMFilterMgr::createUniqueName( const QString & name )
{
  QString uniqueName = name;
  int counter = 0;
  bool found = true;
  
  while ( found ) {
    found = false;
    for ( QPtrListIterator<KMFilter> it(*this) ;
          it.current() ; ++it ) {
      if ( !( (*it)->name().compare( uniqueName ) ) ) {
        found = true;
        ++counter;
        uniqueName = name;
        uniqueName += QString( " (" ) + QString::number( counter ) 
                    + QString( ")" );
        break;
      }
    }
  }
  return uniqueName;
}


//-----------------------------------------------------------------------------
void KMFilterMgr::appendFilters( const QPtrList<KMFilter> filters )
{
  beginUpdate();
  QPtrListIterator<KMFilter> it(filters);
  for ( it.toFirst(); it.current() ; ++it )
    append( *it );
  writeConfig( TRUE );
  endUpdate();
}


void KMFilterMgr::slotFolderRemoved( KMFolder * aFolder )
{
  folderRemoved( aFolder, 0 );
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
#ifndef NDEBUG
void KMFilterMgr::dump(void)
{
  QPtrListIterator<KMFilter> it(*this);
  for ( it.toFirst() ; it.current() ; ++it )
  {
    kdDebug(5006) << (*it)->asString() << endl;
  }
}
#endif

//-----------------------------------------------------------------------------
void KMFilterMgr::endUpdate(void)
{
  emit filterListUpdated();
}

#include "kmfiltermgr.moc"
