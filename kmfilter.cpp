/* -*- mode: C++; c-file-style: "gnu" -*-
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfilter.h"
#include "kmfilteraction.h"
#include "kmglobal.h"
#include "filterlog.h"
using KMail::FilterLog;

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kconfig.h>

#include <assert.h>


KMFilter::KMFilter( KConfig* aConfig, bool popFilter )
  : bPopFilter(popFilter)
{
  if (!bPopFilter)
    mActions.setAutoDelete( true );

  if ( aConfig )
    readConfig( aConfig );
  else if ( bPopFilter )
    mAction = Down;
  else {
    bApplyOnInbound = true;
    bApplyOnOutbound = false;
    bApplyOnExplicit = true;
    bStopProcessingHere = true;
    bConfigureShortcut = false;
    bConfigureToolbar = false;
    bAutoNaming = true;
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
    bConfigureToolbar = aFilter.configureToolbar();
    mIcon = aFilter.icon();
    mShortcut = aFilter.shortcut();

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

    if ( FilterLog::instance()->isLogging() ) {
      // FIXME use the following after the string freeze:
      // QString logText( i18n( "<b>Applying filter action:</b> %1" )
      //                  .arg( (*it)->displayString() ) );
      QString logText( i18n( "<b>Applying filter action:</b> " ) );
      logText.append( (*it)->displayString() );
      FilterLog::instance()->add( logText, FilterLog::appliedAction );
    }

    KMFilterAction::ReturnCode result = (*it)->process( msg );

    switch ( result ) {
    case KMFilterAction::CriticalError:
      if ( FilterLog::instance()->isLogging() ) {
        QString logText = QString( "<font color=#FF0000>%1</font>" )
          .arg( i18n( "A critical error occurred. Processing stops here." ) );
        FilterLog::instance()->add( logText, FilterLog::appliedAction );
      }
      // in case it's a critical error: return immediately!
      return CriticalError;
    case KMFilterAction::ErrorButGoOn:
      if ( FilterLog::instance()->isLogging() ) {
        QString logText = QString( "<font color=#FF0000>%1</font>" )
          .arg( i18n( "A problem was found while applying this action." ) );
        FilterLog::instance()->add( logText, FilterLog::appliedAction );
      }
    default:
      break;
    }
  }

  if ( status == NoResult ) // No filters matched, keep copy of message
    status = GoOn;

  stopIt = stopProcessingHere();

  return status;
}

bool KMFilter::requiresBody( KMMsgBase* msg )
{
  if (pattern() && pattern()->requiresBody())
    return true; // no pattern means always matches?
  QPtrListIterator<KMFilterAction> it( *actions() );
  for ( it.toFirst() ; it.current() ; ++it )
    if ((*it)->requiresBody( msg ))
      return true;
  return false;
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
  bool rem = false;

  QPtrListIterator<KMFilterAction> it( mActions );
  for ( it.toFirst() ; it.current() ; ++it )
    if ( (*it)->folderRemoved( aFolder, aNewFolder ) )
      rem = true;

  return rem;
}

//-----------------------------------------------------------------------------
void KMFilter::readConfig(KConfig* config)
{
  // MKSearchPattern::readConfig ensures
  // that the pattern is purified.
  mPattern.readConfig(config);

  if (bPopFilter) {
    // get the action description...
    QString action = config->readEntry( "action" );
    if ( action == "down" )
      mAction = Down;
    else if ( action == "later" )
      mAction = Later;
    else if ( action == "delete" )
      mAction = Delete;
    else
      mAction = NoAction;
  }
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

    bStopProcessingHere = config->readBoolEntry("StopProcessingHere", true);
    bConfigureShortcut = config->readBoolEntry("ConfigureShortcut", false);
    QString shortcut( config->readEntry( "Shortcut" ) );
    if ( !shortcut.isEmpty() ) {
      KShortcut sc( shortcut );
      setShortcut( sc );
    }
    bConfigureToolbar = config->readBoolEntry("ConfigureToolbar", false);
    bConfigureToolbar = bConfigureToolbar && bConfigureShortcut;
    mIcon = config->readEntry( "Icon", "gear" );
    bAutoNaming = config->readBoolEntry("AutomaticName", false);

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
    switch ( mAction ) {
    case Down:
      config->writeEntry( "action", "down" );
      break;
    case Later:
      config->writeEntry( "action", "later" );
      break;
    case Delete:
      config->writeEntry( "action", "delete" );
      break;
    default:
      config->writeEntry( "action", "" );
    }
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
    if ( !mShortcut.isNull() )
      config->writeEntry( "Shortcut", mShortcut.toString() );
    config->writeEntry( "ConfigureToolbar", bConfigureToolbar );
    config->writeEntry( "Icon", mIcon );
    config->writeEntry( "AutomaticName", bAutoNaming );

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
