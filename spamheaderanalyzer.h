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

#ifndef __KMAIL_SPAMHEADERANALYZER_H__
#define __KMAIL_SPAMHEADERANALYZER_H__

#include <qvaluelist.h>

class QString;
class KMMessage;

namespace KMail {

  /**
     @short A simple tupel of agent, score and header.

     The score returned is positive if no error has occured.
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
    SpamScore() : mScore( -2.0 ) {}
    SpamScore( const QString & agent, float score, const QString & header )
      : mAgent( agent ), mScore( score ), mHeader( header ) {}
    QString agent() const { return mAgent; }
    float score() const { return mScore; }
    QString spamHeader() const { return mHeader; }

  private:
    QString mAgent;
    float mScore;
    QString mHeader;
  };
  typedef QValueList<SpamScore> SpamScores;
  typedef QValueListIterator<SpamScore> SpamScoresIterator;


  /**
     @short Flyweight for analysing spam headers.
     @author Patrick Audley <paudley@blackcat.ca>
   */
  class SpamHeaderAnalyzer {
  public:
    /**
       @short Extract scores from known anti-spam headers
       @param message A KMMessage to examine
       @return A list of detected scores. @see SpamScore
    */
    static SpamScores getSpamScores( const KMMessage *message );
  };

} // namespace KMail

#endif // __KMAIL_SPAMHEADERANALYZER_H__
