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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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
  SpamScores scores;
  SpamAgents agents = AntiSpamConfig::instance()->uniqueAgents();

  for ( SpamAgentsIterator it = agents.begin(); it != agents.end(); ++it ) {
    float score = -2.0;

    // Skip bogus agents
    if ( (*it).scoreType() == SpamAgentNone )
      continue;

    // Do we have the needed score field for this agent?
    QString mField = message->headerField( (*it).header() );
    if ( mField.isEmpty() )
      continue;

    QString scoreString;
    bool scoreValid = false;

    if ( (*it).scoreType() != SpamAgentBool ) {
      // Can we extract the score?
      QRegExp scorePattern = (*it).scorePattern();
      if ( scorePattern.indexIn( mField ) != -1 ) {
        scoreString = scorePattern.cap( 1 );
        scoreValid = true;
      }
    } else
      scoreValid = true;

    if ( !scoreValid ) {
      score = -5.0;
      kDebug() << "Score could not be extracted from header '"
                    << mField << "'";
    } else {
      bool floatValid = false;
      switch ( (*it).scoreType() ) {
        case SpamAgentNone:
          score = -2.0;
          break;

        case SpamAgentBool:
          if( (*it).scorePattern().indexIn( mField ) == -1 )
            score = 0.0;
          else
            score = 100.0;
          break;

        case SpamAgentFloat:
          score = scoreString.toFloat( &floatValid );
          if ( !floatValid ) {
            score = -3.0;
            kDebug() << "Score (" << scoreString <<") is no number";
          }
          else
            score *= 100.0;
          break;

        case SpamAgentFloatLarge:
          score = scoreString.toFloat( &floatValid );
          if ( !floatValid ) {
            score = -3.0;
            kDebug() << "Score (" << scoreString <<") is no number";
          }
          break;

        case SpamAgentAdjustedFloat:
          score = scoreString.toFloat( &floatValid );
          if ( !floatValid ) {
            score = -3.0;
            kDebug() << "Score (" << scoreString <<") is no number";
            break;
          }

          // Find the threshold value.
          QString thresholdString;
          QRegExp thresholdPattern = (*it).thresholdPattern();
          if ( thresholdPattern.indexIn( mField ) != -1 ) {
            thresholdString = thresholdPattern.cap( 1 );
          }
          else {
            score = -6.0;
            kDebug() << "Threshold could not be extracted from header '"
                          << mField << "'";
            break;
          }
          float threshold = thresholdString.toFloat( &floatValid );
          if ( !floatValid || ( threshold <= 0.0 ) ) {
            score = -4.0;
            kDebug() << "Threshold (" << thresholdString <<") is no"
                          << "number or is negative";
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
    //Find the confidence
    float confidence = -2.0;
    QString confidenceString = "-2.0";
    bool confidenceValid = false;
    // Do we have the needed confidence field for this agent?
    QString mCField = message->headerField( (*it).confidenceHeader() );
    if ( ! mCField.isEmpty() ) {
      // Can we extract the confidence?
      QRegExp cScorePattern = (*it).confidencePattern();
      if ( cScorePattern.indexIn( mCField ) != -1 ) {
        confidenceString = cScorePattern.cap( 1 );
      }
      confidence = confidenceString.toFloat( &confidenceValid );
      if( !confidenceValid) {
        kDebug() << "Unable to convert confidence to float:" << confidenceString;
        confidence = -3.0;
      }
    }
    scores.append( SpamScore( (*it).name(), score, ( confidence < 0.0 ) ? confidence : confidence*100, mField, mCField ) );
  }

  return scores;
}
