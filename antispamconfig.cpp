/*  -*- mode: C++; c-file-style: "gnu" -*-
    antispamconfig.cpp

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
#include "config.h"

#include "antispamconfig.h"

#include <kascii.h>

#include <kstaticdeleter.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <QStringList>

using namespace KMail;

AntiSpamConfig * AntiSpamConfig::sSelf = 0;
static KStaticDeleter<AntiSpamConfig> antispamconfig_sd;

AntiSpamConfig * AntiSpamConfig::instance() {
  if ( !sSelf ) {
    antispamconfig_sd.setObject( sSelf, new AntiSpamConfig() );
    sSelf->readConfig();
  }
  return sSelf;
}

void AntiSpamConfig::readConfig()
{
  mAgents.clear();
  KConfig config( "kmail.antispamrc", true );
  config.setReadDefaults( true );
  KConfigGroup general( &config, "General" );
  unsigned int totalTools = general.readEntry( "tools", QVariant( (uint) 0 ) ).toUInt();
  for ( unsigned int i = 1; i <= totalTools; ++i ) {
    KConfigGroup tool( &config, QString("Spamtool #%1").arg( i ) );
    if ( tool.hasKey( "ScoreHeader" ) ) {
      QString name        = tool.readEntry( "ScoreName" );
      QByteArray header   = tool.readEntry( "ScoreHeader" ).toLatin1();
      QByteArray cheader  = tool.readEntry( "ConfidenceHeader" ).toLatin1();
      QByteArray type     = tool.readEntry( "ScoreType" ).toLatin1();
      QString score       = tool.readEntryUntranslated( "ScoreValueRegexp" );
      QString threshold   = tool.readEntryUntranslated( "ScoreThresholdRegexp" );
      QString confidence  = tool.readEntryUntranslated( "ScoreConfidenceRegexp" );
      SpamAgentTypes typeE = SpamAgentNone;
      if ( kasciistricmp( type.data(), "bool" ) == 0 )
	typeE = SpamAgentBool;
      else if ( kasciistricmp( type.data(), "decimal" ) == 0 )
	typeE = SpamAgentFloat;
      else if ( kasciistricmp( type.data(), "percentage" ) == 0 )
	typeE = SpamAgentFloatLarge;
      else if ( kasciistricmp( type.data(), "adjusted" ) == 0 )
	typeE = SpamAgentAdjustedFloat;
      mAgents.append( SpamAgent( name, typeE, header, cheader, QRegExp( score ),
                                 QRegExp( threshold ), QRegExp( confidence ) ) );
    }
  }
}

const SpamAgents AntiSpamConfig::uniqueAgents() const
{
    QStringList seenAgents;
    SpamAgents agents;
    SpamAgents::ConstIterator it( mAgents.begin() );
    SpamAgents::ConstIterator end( mAgents.end() );
    for ( ; it != end ; ++it ) {
        const QString agent( ( *it ).name() );
        if ( !seenAgents.contains( agent )  ) {
            agents.append( *it );
            seenAgents.append( agent );
        }
    }
    return agents;
}
