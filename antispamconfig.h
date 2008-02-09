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

#ifndef __KMAIL_ANTISPAMCONFIG_H__
#define __KMAIL_ANTISPAMCONFIG_H__

#include <QRegExp>
#include <QList>
class QString;

namespace KMail {

  /// Valid types of SpamAgent
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
    SpamAgent( const QString & name, SpamAgentTypes type, const QByteArray & field, const QByteArray & cfield,
               const QRegExp & score, const QRegExp & threshold, const QRegExp & confidence )
      : mName( name ), mType( type ), mField( field ), mConfidenceField( cfield ),
        mScore( score ), mThreshold( threshold ), mConfidence( confidence ) {}

    QString name() const { return mName; }
    SpamAgentTypes scoreType() const { return mType; }
    QByteArray header() const { return mField; }
    QByteArray confidenceHeader() const { return mConfidenceField; }
    QRegExp scorePattern() const { return mScore; }
    QRegExp thresholdPattern() const { return mThreshold; }
    QRegExp confidencePattern() const { return mConfidence; }

  private:
    QString mName;
    SpamAgentTypes mType;
    QByteArray mField;
    QByteArray mConfidenceField;
    QRegExp mScore;
    QRegExp mThreshold;
    QRegExp mConfidence;
  };
  typedef QList<SpamAgent> SpamAgents;
  typedef QList<SpamAgent>::Iterator SpamAgentsIterator;

  class AntiSpamConfigSingletonProvider;

  /**
     @short Singleton to manage loading the kmail.antispamrc file.
     @author Patrick Audley <paudley@blackcat.ca>

     Use of this config-manager class is straight forward.  Since it
     is a singleton object, all you have to do is obtain an instance
     by calling @p SpamConfig::instance() and use any of the
     public member functions.
   */
  class AntiSpamConfig {
  friend class AntiSpamConfigSingletonProvider;
  private:
    AntiSpamConfig() {}

  public:
    ~AntiSpamConfig() {}

    static AntiSpamConfig * instance();

    /** 
     * Returns a list of all agents found on the system. This
     * might list SA twice, if both the C and the Perl version are present.
     */
    const SpamAgents agents() const { return mAgents; }
    SpamAgents agents() { return mAgents; }

    /** 
     * Returns a list of unique agents, found on the system. SpamAssassin will
     * only be listed once, even if both the C and the Perl version are
     * installed.
     */
    const SpamAgents uniqueAgents() const;

  private:
    SpamAgents mAgents;

    void readConfig();
  };

} // namespace KMail

#endif // __KMAIL_ANTISPAMCONFIG_H__
