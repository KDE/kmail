// -*- mode: C++; c-file-style: "gnu" -*-
// kmfiltermgr.cpp

// my header
#include "kmfiltermgr.h"

// other kmail headers
#include "filterlog.h"
using KMail::FilterLog;
#include "kmfilterdlg.h"
#include "kmfolderindex.h"
#include "filterimporterexporter.h"
using KMail::FilterImporterExporter;
#include "kmfoldermgr.h"
#include "kmmsgdict.h"
#include "messageproperty.h"
using KMail::MessageProperty;

// other KDE headers
#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <kmime/kmime_message.h>

// other Qt headers
#include <QRegExp>

// other headers
#include <assert.h>


//-----------------------------------------------------------------------------
KMFilterMgr::KMFilterMgr( bool popFilter )
  : mEditDialog( 0 ),
    bPopFilter( popFilter ),
    mShowLater( false ),
    mDirtyBufferedFolderTarget( true ),
    mBufferedFolderTarget( true ),
    mRefCount( 0 )
{
  if ( bPopFilter ) {
    kDebug() << "pPopFilter set";
  }
  connect( kmkernel, SIGNAL( folderRemoved( KMFolder* ) ),
           this, SLOT( slotFolderRemoved( KMFolder* ) ) );
}


//-----------------------------------------------------------------------------
KMFilterMgr::~KMFilterMgr()
{
  deref( true );
  writeConfig( false );
  clear();
}

void KMFilterMgr::clear()
{
  mDirtyBufferedFolderTarget = true;
  qDeleteAll( mFilters );
  mFilters.clear();
}

//-----------------------------------------------------------------------------
void KMFilterMgr::readConfig(void)
{
  KSharedConfig::Ptr config = KMKernel::config();
  clear();

  if ( bPopFilter ) {
    KConfigGroup group = config->group( "General" );
    mShowLater = group.readEntry( "popshowDLmsgs", false );
  } 
  mFilters = FilterImporterExporter::readFiltersFromConfig( config, bPopFilter );
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

int KMFilterMgr::processPop( KMime::Message * msg ) const {
#if 0 //TODO port to akonadi
  for ( QList<KMFilter*>::const_iterator it = mFilters.begin();
        it != mFilters.end() ; ++it )
    if ( (*it)->pattern()->matches( msg ) )
      return (*it)->action();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return NoAction;
}

bool KMFilterMgr::beginFiltering(KMime::Content *msgBase) const
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

int KMFilterMgr::moveMessage(KMime::Message *msg) const
{
#if 0 //TODO port to akonadi
  if (MessageProperty::filterFolder(msg)->moveMsg( msg ) == 0) {
    if ( kmkernel->folderIsTrash( MessageProperty::filterFolder( msg )))
      KMFilterAction::sendMDN( msg, KMime::MDN::Deleted );
  } else {
    kDebug() << "KMfilterAction - couldn't move msg";
    return 2;
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return 0;
}

void KMFilterMgr::endFiltering(KMime::Content *msgBase) const
{
#if 0 //TODO port to akonadi
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
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  MessageProperty::setFiltering( msgBase, false );
}

int KMFilterMgr::process( KMime::Message * msg, const KMFilter * filter ) {
  int result = 1;
#if 0 //TODO port to akonadi
 bool stopIt = false;

  if ( !msg || !filter || !beginFiltering( msg ))
    return 1;

  if ( isMatching( msg, filter ) ) {
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
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return result;
}

int KMFilterMgr::process( quint32 serNum, const KMFilter * filter ) {
  bool stopIt = false;
  int result = 1;

  if ( !filter)
    return 1;

#if 0 //TODO port to akonadi
  if ( isMatching( serNum, filter ) ) {
    KMFolder *folder = 0;
    int idx = -1;
    // get the message with the serNum
    KMMsgDict::instance()->getLocation(serNum, &folder, &idx);
    if ( !folder || ( idx == -1 ) || ( idx >= folder->count() ) ) {
      return 1;
    }
    KMFolderOpener openFolder( folder, "filtermgr" );
    KMMsgBase *msgBase = folder->getMsgBase( idx );
    bool unGet = !msgBase->isMessage();
    KMMessage *msg = folder->getMsg( idx );
    // do the actual filtering stuff
    if ( !msg || !beginFiltering( msg ) ) {
      if ( unGet) {
        folder->unGetMsg( idx );
        folder->close( "filtermgr" );
      }
      return 1;
    }
    if ( filter->execActions( msg, stopIt ) == KMFilter::CriticalError ) {
      if ( unGet ) {
        folder->unGetMsg( idx );
        folder->close( "filtermgr" );
      }
      return 2;
    }

    KMFolder *targetFolder = MessageProperty::filterFolder( msg );

    endFiltering( msg );
    if ( targetFolder ) {
      tempOpenFolder( targetFolder );
      msg->setTransferInProgress( false );
      result = targetFolder->moveMsg( msg );
      msg->setTransferInProgress( true );
    }
    if ( unGet) {
      folder->unGetMsg( idx );
      folder->close( "filtermgr" );
    }
  } else {
    result = 1;
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return result;
}

int KMFilterMgr::process( KMime::Message * msg, FilterSet set,
                          bool account, uint accountId ) {
  if ( bPopFilter )
    return processPop( msg );

  if ( set == NoSet ) {
    kDebug() << "KMFilterMgr: process() called with not filter set selected";
    return 1;
  }

  bool stopIt = false;
  bool atLeastOneRuleMatched = false;

  if (!beginFiltering( msg ))
    return 1;
#if 0 //TODO port to akonadi
  for ( QList<KMFilter*>::const_iterator it = mFilters.constBegin();
        !stopIt && it != mFilters.constEnd() ; ++it ) {

    if ( ( ( (set&Inbound) && (*it)->applyOnInbound() ) &&
         ( !account ||
             ( account && (*it)->applyOnAccount( accountId ) ) ) ) ||
         ( (set&Outbound)  && (*it)->applyOnOutbound() ) ||
         ( (set&BeforeOutbound)  && (*it)->applyBeforeOutbound() ) ||
         ( (set&Explicit) && (*it)->applyOnExplicit() ) ) {
        // filter is applicable

      if ( isMatching( msg, (*it) ) ) {
        // filter matches
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
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return 1;
}

bool KMFilterMgr::isMatching( KMime::Message * msg, const KMFilter * filter )
{
  bool result = false;
  if ( FilterLog::instance()->isLogging() ) {
    QString logText( i18n( "<b>Evaluating filter rules:</b> " ) );
    logText.append( filter->pattern()->asString() );
    FilterLog::instance()->add( logText, FilterLog::patternDesc );
  }
#if 0 //TODO port to akonadi
  if ( filter->pattern()->matches( msg ) ) {
    if ( FilterLog::instance()->isLogging() ) {
      FilterLog::instance()->add( i18n( "<b>Filter rules have matched.</b>" ),
                                  FilterLog::patternResult );
    }
    result = true;
  }
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return result;
}

bool KMFilterMgr::isMatching( quint32 serNum, const KMFilter * filter )
{
  bool result = false;
  if ( FilterLog::instance()->isLogging() ) {
    QString logText( i18n( "<b>Evaluating filter rules:</b> " ) );
    logText.append( filter->pattern()->asString() );
    FilterLog::instance()->add( logText, FilterLog::patternDesc );
  }
  if ( filter->pattern()->matches( serNum ) ) {
    if ( FilterLog::instance()->isLogging() ) {
      FilterLog::instance()->add( i18n( "<b>Filter rules have matched.</b>" ),
                                  FilterLog::patternResult );
    }
    result = true;
  }
  return result;
}

bool KMFilterMgr::atLeastOneFilterAppliesTo( unsigned int accountID ) const
{
  QList<KMFilter*>::const_iterator it = mFilters.constBegin();
  for ( ; it != mFilters.constEnd() ; ++it ) {
    if ( (*it)->applyOnAccount( accountID ) ) {
      return true;
    }
  }
  return false;
}

bool KMFilterMgr::atLeastOneIncomingFilterAppliesTo( unsigned int accountID ) const
{
  QList<KMFilter*>::const_iterator it = mFilters.constBegin();
  for ( ; it != mFilters.constEnd() ; ++it ) {
    if ( (*it)->applyOnInbound() && (*it)->applyOnAccount( accountID ) ) {
      return true;
    }
  }
  return false;
}

bool KMFilterMgr::atLeastOneOnlineImapFolderTarget()
{
  if (!mDirtyBufferedFolderTarget)
    return mBufferedFolderTarget;

  mDirtyBufferedFolderTarget = false;

  QList<KMFilter*>::const_iterator it = mFilters.constBegin();
  for ( ; it != mFilters.constEnd() ; ++it ) {
    KMFilter *filter = *it;
    QList<KMFilterAction*>::const_iterator jt = filter->actions()->constBegin();
    const QList<KMFilterAction*>::const_iterator jtend = filter->actions()->constEnd();
    for ( ; jt != jtend ; ++jt ) {
      KMFilterActionWithFolder *f = dynamic_cast<KMFilterActionWithFolder*>(*jt);
      if (!f)
        continue;
      QString name = f->argsAsString();
      KMFolder *folder = kmkernel->imapFolderMgr()->findIdString( name );
      if (folder) {
        mBufferedFolderTarget = true;
        return true;
      }
    }
  }
  mBufferedFolderTarget = false;
  return false;
}

//-----------------------------------------------------------------------------
void KMFilterMgr::ref( void )
{
  mRefCount++;
}

//-----------------------------------------------------------------------------
void KMFilterMgr::deref( bool force )
{
  if ( !force ) {
    mRefCount--;
  }
  if ( mRefCount < 0 ) {
    mRefCount = 0;
  }
  if ( mRefCount && !force ) {
    return;
  }
  QVector< KMFolder *>::const_iterator it;
  for ( it = mOpenFolders.constBegin(); it != mOpenFolders.constEnd(); ++it ) {
    (*it)->close( "filtermgr" );
  }
  mOpenFolders.clear();
}


//-----------------------------------------------------------------------------
int KMFilterMgr::tempOpenFolder( KMFolder *aFolder )
{
  assert( aFolder );

  int rc = aFolder->open( "filtermgr" );
  if ( rc ) {
    return rc;
  }

  mOpenFolders.append( aFolder );
  return 0;
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
  mDirtyBufferedFolderTarget = true;
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

void KMFilterMgr::slotFolderRemoved( KMFolder * aFolder )
{
  folderRemoved( aFolder, 0 );
}

//-----------------------------------------------------------------------------
bool KMFilterMgr::folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder)
{
  mDirtyBufferedFolderTarget = true;
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
  emit filterListUpdated();
}

#include "kmfiltermgr.moc"
