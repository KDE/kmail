// -*- mode: C++; c-file-style: "gnu" -*-
// kmfiltermgr.cpp

// my header
#include "kmfiltermgr.h"
#include "kmkernel.h"
// other kmail headers
#include "filterlog.h"
using KMail::FilterLog;
#include "kmfilterdlg.h"
#include "filterimporterexporter.h"
using KMail::FilterImporterExporter;
#include "messageproperty.h"
using KMail::MessageProperty;

#include <akonadi/changerecorder.h>
#include <akonadi/itemmovejob.h>
#include <akonadi/itemfetchscope.h>

// other KDE headers
#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <kmime/kmime_message.h>

// other Qt headers

// other headers
#include <boost/bind.hpp>
#include <algorithm>
#include <assert.h>


//-----------------------------------------------------------------------------
KMFilterMgr::KMFilterMgr( bool popFilter )
  : mEditDialog( 0 ),
    bPopFilter( popFilter ),
    mShowLater( false )
{
  if ( bPopFilter ) {
    kDebug() << "pPopFilter set";
  }
  connect( kmkernel->monitor(), SIGNAL( collectionRemoved( const Akonadi::Collection& ) ),
           this, SLOT( slotFolderRemoved( const Akonadi::Collection & ) ) );

  mChangeRecorder = new Akonadi::ChangeRecorder( this );
  mChangeRecorder->setMimeTypeMonitored( KMime::Message::mimeType() );
  mChangeRecorder->setChangeRecordingEnabled( false );
  mChangeRecorder->fetchCollection( true );
  connect( mChangeRecorder, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
           SLOT(itemAdded(Akonadi::Item,Akonadi::Collection)) );
}


//-----------------------------------------------------------------------------
KMFilterMgr::~KMFilterMgr()
{
  writeConfig( false );
  clear();
}

void KMFilterMgr::clear()
{
  qDeleteAll( mFilters );
  mFilters.clear();
}

//-----------------------------------------------------------------------------
void KMFilterMgr::readConfig(void)
{
  beginUpdate();
  KSharedConfig::Ptr config = KMKernel::config();
  clear();

  if ( bPopFilter ) {
    KConfigGroup group = config->group( "General" );
    mShowLater = group.readEntry( "popshowDLmsgs", false );
  }
  mFilters = FilterImporterExporter::readFiltersFromConfig( config, bPopFilter );
  endUpdate();
}

//-----------------------------------------------------------------------------
void KMFilterMgr::writeConfig(bool withSync)
{
  KSharedConfig::Ptr config = KMKernel::config();

  // Now, write out the new stuff:
  FilterImporterExporter::writeFiltersToConfig( mFilters, config, bPopFilter );
  KConfigGroup group = config->group( "General" );
  if ( bPopFilter )
      group.writeEntry("popshowDLmsgs", mShowLater);

 if ( withSync ) group.sync();
}

int KMFilterMgr::processPop( const Akonadi::Item & item ) const {
  for ( QList<KMFilter*>::const_iterator it = mFilters.begin();
        it != mFilters.end() ; ++it )
    if ( (*it)->pattern()->matches( item ) )
      return (*it)->action();

  return NoAction;
}

bool KMFilterMgr::beginFiltering( const Akonadi::Item &item ) const
{
  if (MessageProperty::filtering( item ))
    return false;
  MessageProperty::setFiltering( item, true );
  if ( FilterLog::instance()->isLogging() ) {
    FilterLog::instance()->addSeparator();
  }
  return true;
}

void KMFilterMgr::endFiltering( const Akonadi::Item &item ) const
{
  MessageProperty::setFiltering( item, false );
}

int KMFilterMgr::process( const Akonadi::Item &item, const KMFilter * filter )
{
  bool stopIt = false;
  int result = 1;

  if ( !filter || !item.hasPayload<KMime::Message::Ptr>() )
    return 1;

  if ( isMatching( item, filter ) ) {
    // do the actual filtering stuff
    if ( !beginFiltering( item ) ) {
      return 1;
    }
    if ( filter->execActions( item, stopIt ) == KMFilter::CriticalError ) {
      return 2;
    }

    const Akonadi::Collection targetFolder = MessageProperty::filterFolder( item );
    endFiltering( item );
    if ( targetFolder.isValid() ) {
      new Akonadi::ItemMoveJob( item, targetFolder, this ); // TODO: check result
      result = 0;
    }
  } else {
    result = 1;
  }
  return result;
}

int KMFilterMgr::process( const Akonadi::Item &item, FilterSet set,
                          bool account, const QString& accountId ) {

  if ( bPopFilter )
    return processPop( item );

  if ( set == NoSet ) {
    kDebug() << "KMFilterMgr: process() called with not filter set selected";
    return 1;
  }
  bool stopIt = false;
  bool atLeastOneRuleMatched = false;

  if ( !beginFiltering( item ) )
    return 1;
  for ( QList<KMFilter*>::const_iterator it = mFilters.constBegin();
        !stopIt && it != mFilters.constEnd() ; ++it ) {

    if ( ( ( (set&Inbound) && (*it)->applyOnInbound() ) &&
         ( !account ||
             ( account && (*it)->applyOnAccount( accountId ) ) ) ) ||
         ( (set&Outbound)  && (*it)->applyOnOutbound() ) ||
         ( (set&BeforeOutbound)  && (*it)->applyBeforeOutbound() ) ||
         ( (set&Explicit) && (*it)->applyOnExplicit() ) ) {
        // filter is applicable

      if ( isMatching( item, (*it) ) ) {
        // filter matches
        atLeastOneRuleMatched = true;
        // execute actions:
        if ( (*it)->execActions(item, stopIt) == KMFilter::CriticalError )
          return 2;
      }
    }
  }

  Akonadi::Collection targetFolder = MessageProperty::filterFolder( item );
  /* endFilter does a take() and addButKeepUID() to ensure the changed
   * message is on disk. This is unnessecary if nothing matched, so just
   * reset state and don't update the listview at all. */
  if ( atLeastOneRuleMatched )
    endFiltering( item );
  else
    MessageProperty::setFiltering( item, false );
  if ( targetFolder.isValid() ) {
    new Akonadi::ItemMoveJob( item, targetFolder, this ); // TODO: check result
    return 0;
  }
  return 1;
}

bool KMFilterMgr::isMatching( const Akonadi::Item& item, const KMFilter * filter )
{
  bool result = false;
  if ( FilterLog::instance()->isLogging() ) {
    QString logText( i18n( "<b>Evaluating filter rules:</b> " ) );
    logText.append( filter->pattern()->asString() );
    FilterLog::instance()->add( logText, FilterLog::patternDesc );
  }
  if ( filter->pattern()->matches( item ) ) {
    if ( FilterLog::instance()->isLogging() ) {
      FilterLog::instance()->add( i18n( "<b>Filter rules have matched.</b>" ),
                                  FilterLog::patternResult );
    }
    result = true;
  }
  return result;
}

bool KMFilterMgr::atLeastOneFilterAppliesTo( const QString& accountID ) const
{
  QList<KMFilter*>::const_iterator it = mFilters.constBegin();
  for ( ; it != mFilters.constEnd() ; ++it ) {
    if ( (*it)->applyOnAccount( accountID ) ) {
      return true;
    }
  }
  return false;
}

bool KMFilterMgr::atLeastOneIncomingFilterAppliesTo( const QString& accountID ) const
{
  QList<KMFilter*>::const_iterator it = mFilters.constBegin();
  for ( ; it != mFilters.constEnd() ; ++it ) {
    if ( (*it)->applyOnInbound() && (*it)->applyOnAccount( accountID ) ) {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void KMFilterMgr::openDialog( QWidget *, bool checkForEmptyFilterList )
{
  if( !mEditDialog )
  {
    //
    // We can't use the parent as long as the dialog is modeless
    // and there is one shared dialog for all top level windows.
    //
    mEditDialog = new KMFilterDlg( 0, bPopFilter, checkForEmptyFilterList );
    mEditDialog->setObjectName( "filterdialog" );
  }
  mEditDialog->show();
}


//-----------------------------------------------------------------------------
void KMFilterMgr::createFilter( const QByteArray & field, const QString & value )
{
  openDialog( 0, false );
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
    for ( QList<KMFilter*>::const_iterator it = mFilters.constBegin();
          it != mFilters.constEnd(); ++it ) {
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
void KMFilterMgr::appendFilters( const QList<KMFilter*> &filters,
                                 bool replaceIfNameExists )
{
  beginUpdate();
  if ( replaceIfNameExists ) {
    QList<KMFilter*>::const_iterator it1 = filters.constBegin();
    for ( ; it1 != filters.constEnd() ; ++it1 ) {
      for ( int i = 0; i < mFilters.count(); i++ ) {
        KMFilter *filter = mFilters[i];
        if ( (*it1)->name() == filter->name() ) {
          mFilters.removeAll( filter );
          i = 0;
        }
      }
    }
  }
  mFilters += filters;
  writeConfig( true );
  endUpdate();
}

void KMFilterMgr::setFilters( const QList<KMFilter*> &filters )
{
  beginUpdate();
  clear();
  mFilters = filters;
  writeConfig( true );
  endUpdate();
}

void KMFilterMgr::slotFolderRemoved( const Akonadi::Collection & aFolder )
{
  folderRemoved( aFolder, Akonadi::Collection() );
}

//-----------------------------------------------------------------------------
bool KMFilterMgr::folderRemoved(const Akonadi::Collection & aFolder, const Akonadi::Collection & aNewFolder)
{
  bool rem = false;
  QList<KMFilter*>::const_iterator it = mFilters.constBegin();
  for ( ; it != mFilters.constEnd() ; ++it )
    if ( (*it)->folderRemoved(aFolder, aNewFolder) )
      rem = true;

  return rem;
}


//-----------------------------------------------------------------------------
#ifndef NDEBUG
void KMFilterMgr::dump(void) const
{
  QList<KMFilter*>::const_iterator it = mFilters.constBegin();
  for ( ; it != mFilters.constEnd() ; ++it ) {
    kDebug() << (*it)->asString();
  }
}
#endif

//-----------------------------------------------------------------------------
void KMFilterMgr::endUpdate(void)
{
  const bool requiresBody = std::find_if( mFilters.constBegin(), mFilters.constEnd(),
      boost::bind( &KMFilter::requiresBody, _1 ) ) != mFilters.constEnd();
  mChangeRecorder->itemFetchScope().fetchFullPayload( requiresBody );

  emit filterListUpdated();
}

void KMFilterMgr::itemAdded(const Akonadi::Item& item, const Akonadi::Collection &collection)
{
  process( item, Inbound, true, collection.resource() );
}

#include "kmfiltermgr.moc"
