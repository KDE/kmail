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


#include <assert.h>


KMFilter::KMFilter( KConfig* aConfig )
{
  mActions.setAutoDelete( TRUE );

  if ( aConfig )
    readConfig( aConfig );
  else {
    bApplyOnInbound = TRUE;
    bApplyOnOutbound = FALSE;
    bStopProcessingHere = TRUE;
  }
}


KMFilter::KMFilter( KMFilter * aFilter )
{
  mActions.setAutoDelete( TRUE );

  if ( aFilter ) {
    mPattern = *aFilter->pattern();
    
    bApplyOnInbound = aFilter->applyOnInbound();
    bApplyOnOutbound = aFilter->applyOnOutbound();
    bStopProcessingHere = aFilter->stopProcessingHere();
    
    QPtrListIterator<KMFilterAction> it( *aFilter->actions() );
    for ( it.toFirst() ; it.current() ; ++it ) {
      KMFilterActionDesc *desc = (*kernel->filterActionDict())[ (*it)->name() ];
      if ( desc ) {
	KMFilterAction *f = desc->create();
	if ( f ) {
	  f->argsFromString( (*it)->argsAsString() );
	  mActions.append( f );
	}
      }
    }
  } else {
    bApplyOnInbound = TRUE;
    bApplyOnOutbound = FALSE;
    bStopProcessingHere = TRUE;
  }
}


KMFilter::ReturnCode KMFilter::execActions( KMMessage* msg, bool& stopIt ) const
{
  ReturnCode status = NoResult;

  QPtrListIterator<KMFilterAction> it( mActions );
  for ( it.toFirst() ; !stopIt && it.current() ; ++it ) {

    kdDebug(5006) << "####### KMFilter::process: going to apply action "
	      << (*it)->label() << " \"" << (*it)->argsAsString()
	      << "\"" << endl;

    KMFilterAction::ReturnCode result = (*it)->process( msg );

    switch ( result ) {
    case KMFilterAction::CriticalError:
      return CriticalError;
    case KMFilterAction::ErrorButGoOn:
      // Small problem, keep a copy
      status = GoOn;
      break;
    case KMFilterAction::Finished:
      // Message saved in a folder
      kdDebug(5006) << "got result Finished" << endl;
      status = MsgExpropriated;
    default:
      break;
    }
  }

  if ( status == NoResult ) // No filters matched, keep copy of message
    status = GoOn;

  stopIt = stopProcessingHere();

  return status;
}


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

  { // limit lifetime of "sets"
    QStringList sets = config->readListEntry("apply-on");
    if ( sets.isEmpty() ) {
      bApplyOnOutbound = FALSE;
      bApplyOnInbound = TRUE;
    } else {
      bApplyOnInbound = bool(sets.contains("check-mail"));
      bApplyOnOutbound = bool(sets.contains("send-mail"));
    }
  }

  bStopProcessingHere = config->readBoolEntry("StopProcessingHere", TRUE);

  int i, numActions;
  QString actName, argsName;

  mActions.clear();

  numActions = config->readNumEntry("actions",0);
  if (numActions > FILTER_MAX_ACTIONS) {
    numActions = FILTER_MAX_ACTIONS ;
    KMessageBox::information( 0, i18n("Too many filter actions in filter rule `%1'").arg( mPattern.name() ) );
  }

  for ( i=0 ; i < numActions ; i++ ) {
    actName.sprintf("action-name-%d", i);
    argsName.sprintf("action-args-%d", i);
    // get the action description...
    KMFilterActionDesc *desc = (*kernel->filterActionDict())[ config->readEntry( actName ) ];
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
				i18n("Unknown filter action `%1'\n in filter rule `%2'."
				     "\nIgnoring it.").arg( config->readEntry( actName ) ).arg( mPattern.name() ) );
  }
}


void KMFilter::writeConfig(KConfig* config) const
{
  mPattern.writeConfig(config);

  QStringList sets;
  if ( bApplyOnInbound )
    sets.append( "check-mail" );
  if ( bApplyOnOutbound )
    sets.append( "send-mail" );
  sets.append( "manual-filtering" );
  config->writeEntry( "apply-on", sets );

  config->writeEntry( "StopProcessingHere", bStopProcessingHere );

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

void KMFilter::purify()
{
  mPattern.purify();

  QPtrListIterator<KMFilterAction> it( mActions );
  it.toLast();
  while ( it.current() )
    if ( (*it)->isEmpty() )
      mActions.remove ( (*it) );
    else
      --it;

}

const QString KMFilter::asString() const
{
  QString result;

  result += mPattern.asString();

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
  result += "\n";
  if ( bStopProcessingHere )
    result += "If it matches, processing stops at this filter.\n";

  return result;
}

