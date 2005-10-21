/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
 *              Copyright (C) 2001-2003 Marc Mutz, mutz@kde.org
 *   Contains code segments and ideas from earlier kmail dialog code.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

// This must be first
#include <config.h>
#include "configuredialog.h"
#include "configuredialog_p.h"
#include <kcmodule.h>

//----------------------------
// KCM stuff
//----------------------------
extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_misc( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() )
  {
    MiscPage *page = new MiscPage( instance, parent, args );
    page->setObjectName( "kcmkmail_config_misc" );
    return page;
  }
}

extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_appearance( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() )
  {
    AppearancePage *page =
       new AppearancePage( instance, parent, args );
    page->setObjectName( "kcmkmail_config_appearance" );
    return page;
  }
}

extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_composer( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() )
  {
    ComposerPage *page = new ComposerPage( instance, parent, args );
    page->setObjectName( "kcmkmail_config_composer" );
    return page;
  }
}

extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_identity( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() )
  {
    IdentityPage *page = new IdentityPage( instance, parent, args );
    page->setObjectName( "kcmkmail_config_identity" );
    return page;
  }
}

extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_accounts( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() )
  {
    AccountsPage *page = new AccountsPage( instance, parent, args );
    page->setObjectName( "kcmkmail_config_accounts" );
    return page;
  }
}

extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_security( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() )
  {
    SecurityPage *page = new SecurityPage( instance, parent, args );
    page->setObjectName( "kcmkmail_config_security" );
    return page;
  }
}
