/*  -*- mode: C++; c-file-style: "gnu" -*-
    spamheaderanalyzer.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Patrick Audley <paudley@blackcat.ca>
    Copyright (c) 2004 Ingo Kloecker <kloecker@kde.org>

    KMail is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "spamheaderanalyzer.h"

#include "antispamconfig.h"
#include "kmmessage.h"

#include <kdebug.h>

using namespace KMail;

// static
SpamScores SpamHeaderAnalyzer::getSpamScores( const KMMessage* message ) {
  kdDebug(5006) << k_funcinfo << endl;
  SpamScores scores;
  SpamAgents agents = AntiSpamConfig::instance()->agents();

  for ( SpamAgentsIterator it = agents.begin(); it != agents.end(); ++it ) {
    float score = -2.0;

    // Skip bogus agents
    if ( (*it).scoreType() == SpamAgentNone )
      continue;

    // Do we have the needed field for this agent?
    QString mField = message->headerField( (*it).header() );
    if ( mField.isEmpty() )
      continue;

    QString scoreString;
    bool scoreValid = false;

    if ( (*it).scoreType() != SpamAgentBool ) {
      // Can we extract the score?
      QRegExp scorePattern = (*it).scorePattern();
      if ( scorePattern.search( mField ) != -1 ) {
	scoreString = scorePattern.cap( 1 );
	scoreValid = true;
      }
    } else
      scoreValid = true;

    if ( !scoreValid ) {
      score = -5.0;
      kdDebug(5006) << "Score could not be extracted from header '"
                    << mField << "'" << endl;
    } else {
      bool floatValid = false;
      switch ( (*it).scoreType() ) {
	case SpamAgentNone:
	  score = -2.0;
	  break;

	case SpamAgentBool:
	  if( (*it).scorePattern().search( mField ) == -1 )
	    score = 0.0;
	  else
	    score = 100.0;
	  break;

	case SpamAgentFloat:
	  score = scoreString.toFloat( &floatValid );
	  if ( !floatValid ) {
	    score = -3.0;
            kdDebug(5006) << "Score (" << scoreString << ") is no number"
                          << endl;
          }
	  else
	    score *= 100.0;
	  break;

	case SpamAgentFloatLarge:
	  score = scoreString.toFloat( &floatValid );
	  if ( !floatValid ) {
            score = -3.0;
            kdDebug(5006) << "Score (" << scoreString << ") is no number"
                          << endl;
          }
	  break;

	case SpamAgentAdjustedFloat:
	  score = scoreString.toFloat( &floatValid );
	  if ( !floatValid ) {
            score = -3.0;
            kdDebug(5006) << "Score (" << scoreString << ") is no number"
                          << endl;
            break;
          }

	  // Find the threshold value.
	  QString thresholdString;
          QRegExp thresholdPattern = (*it).thresholdPattern();
	  if ( thresholdPattern.search( mField ) != -1 ) {
	    thresholdString = thresholdPattern.cap( 1 );
          }
	  else {
	    score = -6.0;
            kdDebug(5006) << "Threshold could not be extracted from header '"
                          << mField << "'" << endl;
	    break;
	  }
	  float threshold = thresholdString.toFloat( &floatValid );
	  if ( !floatValid || ( threshold <= 0.0 ) ) {
            score = -4.0;
            kdDebug(5006) << "Threshold (" << thresholdString << ") is no "
                          << "number or is negative" << endl;
            break;
          }

          // Normalize the score. Anything below 0 means 0%, anything above
          // threshold mean 100%. Values between 0 and threshold are mapped
          // linearily to 0% - 100%.
	  if ( score < 0.0 )
            score = 0.0;
          else if ( score > threshold )
            score = 100.0;
          else
            score = score / threshold * 100.0;
	  break;
	}
    }
    scores.append( SpamScore( (*it).name(), score, mField ) );
  }

  return scores;
}
