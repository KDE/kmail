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

#include "antispamconfig.h"

#include <kasciistricmp.h>

#include <kstaticdeleter.h>
#include <kconfig.h>

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
  unsigned int totalTools = general.readUnsignedNumEntry( "tools", 0 );
  for ( unsigned int i = 1; i <= totalTools; ++i ) {
    KConfigGroup tool( &config, QString("Spamtool #%1").arg( i ) );
    if ( tool.hasKey( "ScoreHeader" ) ) {
      QString name      = tool.readEntry( "ScoreName" );
      QCString header   = tool.readEntry( "ScoreHeader" ).latin1();
      QCString type     = tool.readEntry( "ScoreType" ).latin1();
      QString score     = tool.readEntryUntranslated( "ScoreValueRegexp" );
      QString threshold = tool.readEntryUntranslated( "ScoreThresholdRegexp" );
      SpamAgentTypes typeE = SpamAgentNone;
      if ( kasciistricmp( type.data(), "bool" ) == 0 )
	typeE = SpamAgentBool;
      else if ( kasciistricmp( type.data(), "decimal" ) == 0 )
	typeE = SpamAgentFloat;
      else if ( kasciistricmp( type.data(), "percentage" ) == 0 )
	typeE = SpamAgentFloatLarge;
      else if ( kasciistricmp( type.data(), "adjusted" ) == 0 )
	typeE = SpamAgentAdjustedFloat;
      mAgents.append( SpamAgent( name, typeE, header, QRegExp( score ),
                                 QRegExp( threshold ) ) );
    }
  }
}
