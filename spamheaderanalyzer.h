/*  -*- mode: C++; c-file-style: "gnu" -*-
    spamheaderanalyzer.h

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

#ifndef __KMAIL_SPAMHEADERANALYZER_H__
#define __KMAIL_SPAMHEADERANALYZER_H__


class QString;
class KMMessage;
#include <QString>
#include <QList>
namespace KMail {

  /**
     @short A simple tupel of agent, score, confidence and header.

     The score returned is positive if no error has occurred.
     Negative values indicate the following errors:
       -1.0     Unintialized struct used
       -2.0     Error extracing agent string
       -3.0     Couldn't convert score to float
       -4.0     Couldn't convert threshold to float or threshold is negative
       -5.0     Couldn't find the score field
       -6.0     Couldn't find the threshold field
  */
  class SpamScore {
  public:
    SpamScore() : mScore( -2.0 ), mConfidence( -2.0 ) {}
    SpamScore( const QString & agent, float score, float confidence, 
        const QString & header, const QString & cheader )
      : mAgent( agent ), mScore( score ), mConfidence( confidence ), 
          mHeader( header ), mConfidenceHeader( cheader ) {}
    QString agent() const { return mAgent; }
    float score() const { return mScore; }
    float confidence() const { return mConfidence; }
    QString spamHeader() const { return mHeader; }
    QString confidenceHeader() const { return mConfidenceHeader; }

  private:
    QString mAgent;
    float mScore;
    float mConfidence;
    QString mHeader;
    QString mConfidenceHeader;
  };
  typedef QList<SpamScore> SpamScores;
  typedef QList<SpamScore>::Iterator SpamScoresIterator;


  /**
     @short Flyweight for analysing spam headers.
     @author Patrick Audley <paudley@blackcat.ca>
   */
  class SpamHeaderAnalyzer {
  public:
    /**
       @short Extract scores from known anti-spam headers
       @param message A KMMessage to examine
       @return A list of detected scores. See SpamScore
    */
    static SpamScores getSpamScores( const KMMessage *message );
  };

} // namespace KMail

#endif // __KMAIL_SPAMHEADERANALYZER_H__
