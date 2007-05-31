/*
    This file is part of KMail.

    Copyright (c) 2005 David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2,
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "globalsettings.h"
#include <kstaticdeleter.h>
#include <qtimer.h>

GlobalSettings *GlobalSettings::mSelf = 0;
static KStaticDeleter<GlobalSettings> staticGlobalSettingsDeleter;

GlobalSettings *GlobalSettings::self()
{
  if ( !mSelf ) {
    staticGlobalSettingsDeleter.setObject( mSelf, new GlobalSettings() );
    mSelf->readConfig();
  }

  return mSelf;
}

GlobalSettings::GlobalSettings()
{
  mConfigSyncTimer = new QTimer( this, "mConfigSyncTimer" );
  connect( mConfigSyncTimer, SIGNAL( timeout() ), this, SLOT( slotSyncNow() ) );
}

void GlobalSettings::requestSync()
{
 if ( !mConfigSyncTimer->isActive() )
   mConfigSyncTimer->start( 0, true /*single shot*/ );
}

void GlobalSettings::slotSyncNow()
{
  config()->sync();
}

GlobalSettings::~GlobalSettings()
{
}

#include "globalsettings.moc"
