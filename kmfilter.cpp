// kmfilter.cpp
// Author: Stefan Taferner <taferner@kde.org>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfilter.h"
#include "kmfilteraction.h"
#include "kmglobal.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kconfig.h>

#include <assert.h>


KMFilter::KMFilter( KConfig* aConfig, bool popFilter )
  : bPopFilter(popFilter)
{
 if (!bPopFilter)
    mActions.setAutoDelete( TRUE );

  if ( aConfig )
    readConfig( aConfig );
  else if ( bPopFilter )
    mAction = Down;
  else {
    bApplyOnInbound = TRUE;
    bApplyOnOutbound = FALSE;
    bApplyOnExplicit = TRUE;
    bStopProcessingHere = TRUE;
    bConfigureShortcut = FALSE;
  }
}


KMFilter::KMFilter( const KMFilter & aFilter )
{
  bPopFilter = aFilter.isPopFilter();

  if ( !bPopFilter )
    mActions.setAutoDelete( true );

  mPattern = aFilter.mPattern;

  if ( bPopFilter ){
    mAction = aFilter.mAction;
  } else {
    bApplyOnInbound = aFilter.applyOnInbound();
    bApplyOnOutbound = aFilter.applyOnOutbound();
    bApplyOnExplicit = aFilter.applyOnExplicit();
    bStopProcessingHere = aFilter.stopProcessingHere();
    bConfigureShortcut = aFilter.configureShortcut();

    QPtrListIterator<KMFilterAction> it( aFilter.mActions );
    for ( it.toFirst() ; it.current() ; ++it ) {
      KMFilterActionDesc *desc = (*kmkernel->filterActionDict())[ (*it)->name() ];
      if ( desc ) {
	KMFilterAction *f = desc->create();
	if ( f ) {
	  f->argsFromString( (*it)->argsAsString() );
	  mActions.append( f );
	}
      }
    }
  }
}

// only for !bPopFilter
KMFilter::ReturnCode KMFilter::execActions( KMMessage* msg, bool& stopIt ) const
{
  ReturnCode status = NoResult;

  QPtrListIterator<KMFilterAction> it( mActions );
  for ( it.toFirst() ; it.current() ; ++it ) {

    kdDebug(5006) << "####### KMFilter::process: going to apply action "
	      << (*it)->label() << " \"" << (*it)->argsAsString()
	      << "\"" << endl;

    KMFilterAction::ReturnCode result = (*it)->process( msg );

    switch ( result ) {
    case KMFilterAction::CriticalError:
      // in case it's a critical error: return immediately!
      return CriticalError;
    case KMFilterAction::Moved:
      // Message saved in a folder
      kdDebug(5006) << "got result Moved" << endl;
      status = MsgExpropriated;
    case KMFilterAction::ErrorButGoOn:
    default:
      break;
    }
  }

  if ( status == NoResult ) // No filters matched, keep copy of message
    status = GoOn;

  stopIt = stopProcessingHere();

  return status;
}

/** No descriptions */
// only for bPopFilter
void KMFilter::setAction(const KMPopFilterAction aAction)
{
  mAction = aAction;
}

// only for bPopFilter
KMPopFilterAction KMFilter::action()
{
  return mAction;
}

// only for !bPopFilter
bool KMFilter::folderRemoved( KMFolder* aFolder, KMFolder* aNewFolder )
{
  bool rem = FALSE;

  QPtrListIterator<KMFilterAction> it( mActions );
  for ( it.toFirst() ; it.current() ; ++it )
    if ( (*it)->folderRemoved( aFolder, aNewFolder ) )
      rem = TRUE;

  return rem;
}

//-----------------------------------------------------------------------------
void KMFilter::readConfig(KConfig* config)
{
  // MKSearchPattern::readConfig ensures
  // that the pattern is purified.
  mPattern.readConfig(config);

  if (bPopFilter)
    // get the action description...
    mAction = (KMPopFilterAction) config->readNumEntry( "action" );
  else {
    QStringList sets = config->readListEntry("apply-on");
    if ( sets.isEmpty() && !config->hasKey("apply-on") ) {
      bApplyOnOutbound = false;
      bApplyOnInbound = true;
      bApplyOnExplicit = true;
    } else {
      bApplyOnInbound = bool(sets.contains("check-mail"));
      bApplyOnOutbound = bool(sets.contains("send-mail"));
      bApplyOnExplicit = bool(sets.contains("manual-filtering"));
    }

    bStopProcessingHere = config->readBoolEntry("StopProcessingHere", TRUE);
    bConfigureShortcut = config->readBoolEntry("ConfigureShortcut", FALSE);

    int i, numActions;
    QString actName, argsName;

    mActions.clear();

    numActions = config->readNumEntry("actions",0);
    if (numActions > FILTER_MAX_ACTIONS) {
      numActions = FILTER_MAX_ACTIONS ;
      KMessageBox::information( 0, i18n("<qt>Too many filter actions in filter rule <b>%1</b>.</qt>").arg( mPattern.name() ) );
    }

    for ( i=0 ; i < numActions ; i++ ) {
      actName.sprintf("action-name-%d", i);
      argsName.sprintf("action-args-%d", i);
      // get the action description...
      KMFilterActionDesc *desc = (*kmkernel->filterActionDict())[ config->readEntry( actName ) ];
      if ( desc ) {
        //...create an instance...
        KMFilterAction *fa = desc->create();
        if ( fa ) {
  	  //...load it with it's parameter...
          fa->argsFromString( config->readEntry( argsName ) );
	  //...check if it's emoty and...
	  if ( !fa->isEmpty() )
	    //...append it if it's not and...
	    mActions.append( fa );
	  else
	    //...delete is else.
	    delete fa;
        }
      } else
        KMessageBox::information( 0 /* app-global modal dialog box */,
				  i18n("<qt>Unknown filter action <b>%1</b><br>in filter rule <b>%2</b>.<br>Ignoring it.</qt>")
				       .arg( config->readEntry( actName ) ).arg( mPattern.name() ) );
    }
  }
}


void KMFilter::writeConfig(KConfig* config) const
{
  mPattern.writeConfig(config);

  if (bPopFilter) {
    config->writeEntry( "action", mAction );
  } else {
    QStringList sets;
    if ( bApplyOnInbound )
      sets.append( "check-mail" );
    if ( bApplyOnOutbound )
      sets.append( "send-mail" );
    if ( bApplyOnExplicit )
      sets.append( "manual-filtering" );
    config->writeEntry( "apply-on", sets );

    config->writeEntry( "StopProcessingHere", bStopProcessingHere );
    config->writeEntry( "ConfigureShortcut", bConfigureShortcut );

    QString key;
    int i;

    QPtrListIterator<KMFilterAction> it( mActions );
    for ( i=0, it.toFirst() ; it.current() ; ++it, ++i ) {
      config->writeEntry( key.sprintf("action-name-%d", i),
    			  (*it)->name() );
      config->writeEntry( key.sprintf("action-args-%d", i),
			  (*it)->argsAsString() );
    }
    config->writeEntry("actions", i );
  }
}

void KMFilter::purify()
{
  mPattern.purify();

  if (!bPopFilter) {
    QPtrListIterator<KMFilterAction> it( mActions );
    it.toLast();
    while ( it.current() )
      if ( (*it)->isEmpty() )
        mActions.remove ( (*it) );
      else
        --it;
  }
}

bool KMFilter::isEmpty() const
{
  if (bPopFilter)
    return mPattern.isEmpty();
  else
    return mPattern.isEmpty() && mActions.isEmpty();
}

#ifndef NDEBUG
const QString KMFilter::asString() const
{
  QString result;

  result += mPattern.asString();

  if (bPopFilter){
    result += "    action: ";
    result += mAction;
    result += "\n";
  }
  else {
    QPtrListIterator<KMFilterAction> it( mActions );
    for ( it.toFirst() ; it.current() ; ++it ) {
      result += "    action: ";
      result += (*it)->label();
      result += " ";
      result += (*it)->argsAsString();
      result += "\n";
    }
    result += "This filter belongs to the following sets:";
    if ( bApplyOnInbound )
      result += " Inbound";
    if ( bApplyOnOutbound )
      result += " Outbound";
    if ( bApplyOnExplicit )
      result += " Explicit";
    result += "\n";
    if ( bStopProcessingHere )
      result += "If it matches, processing stops at this filter.\n";
  }
  return result;
}
#endif
