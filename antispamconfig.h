/*  -*- mode: C++; c-file-style: "gnu" -*-
    antispamconfig.h

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

#ifndef __KMAIL_ANTISPAMCONFIG_H__
#define __KMAIL_ANTISPAMCONFIG_H__

#include <qvaluelist.h>
#include <qregexp.h>

class QString;
class QCString;

namespace KMail {

  /// Valid types of @see SpamAgent
  typedef enum {
    SpamAgentNone,          //!< Invalid SpamAgent, skip this agent
    SpamAgentBool,          //!< Simple Yes or No (Razor)
    SpamAgentFloat,         //!< For straight percentages between 0.0 and 1.0 (BogoFilter)
    SpamAgentFloatLarge,    //!< For straight percentages between 0.0 and 100.0
    SpamAgentAdjustedFloat  //!< Use this when we need to compare against a threshold (SpamAssasssin)
  } SpamAgentTypes;

  class SpamAgent {
  public:
    SpamAgent() : mType( SpamAgentNone ) {}
    SpamAgent( const QString & name, SpamAgentTypes type, const QCString & field,
	       const QRegExp & score, const QRegExp & threshold )
      : mName( name ), mType( type ), mField( field ),
	mScore( score ), mThreshold( threshold ) {}

    QString name() const { return mName; }
    SpamAgentTypes scoreType() const { return mType; }
    QCString header() const { return mField; }
    QRegExp scorePattern() const { return mScore; }
    QRegExp thresholdPattern() const { return mThreshold; }

  private:
    QString mName;
    SpamAgentTypes mType;
    QCString mField;
    QRegExp mScore;
    QRegExp mThreshold;
  };
  typedef QValueList<SpamAgent> SpamAgents;
  typedef QValueListIterator<SpamAgent> SpamAgentsIterator;

  /**
     @short Singleton to manage loading the kmail.antispamrc file.
     @author Patrick Audley <paudley@blackcat.ca>

     Use of this config-manager class is straight forward.  Since it
     is a singleton object, all you have to do is obtain an instance
     by calling @p SpamConfig::instance() and use any of the
     public member functions.
   */
  class AntiSpamConfig {
  private:
    static AntiSpamConfig * sSelf;

    AntiSpamConfig() {}

  public:
    ~AntiSpamConfig() {}

    static AntiSpamConfig * instance();

    SpamAgents agents() const { return mAgents; }

  private:
    SpamAgents mAgents;

    void readConfig();
  };

} // namespace KMail

#endif // __KMAIL_ANTISPAMCONFIG_H__
