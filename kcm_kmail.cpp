/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright (C) 2001-2003 Marc Mutz, mutz@kde.org
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
#include "configuredialog/configuredialog.h"
#include "configuredialog/configuredialog_p.h"
#include "configuredialog/configuremiscpage.h"
#include "configuredialog/configuresecuritypage.h"
#include "configuredialog/configurecomposerpage.h"
#include "configuredialog/configureappearancepage.h"
#include "configuredialog/configureaccountpage.h"
#include "identity/identitypage.h"
#include <kcmodule.h>
#include <kcomponentdata.h>

//----------------------------
// KCM stuff
//----------------------------
extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_misc( QWidget *parent, const char* )
  {
    KComponentData instance( "kcmkmail_config_misc" );
    MiscPage *page = new MiscPage( instance, parent );
    page->setObjectName( QLatin1String("kcmkmail_config_misc") );
    return page;
  }
}

extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_appearance( QWidget *parent, const char* )
  {
    KComponentData instance( "kcmkmail_config_appearance" );
    AppearancePage *page =
       new AppearancePage( instance, parent );
    page->setObjectName( QLatin1String("kcmkmail_config_appearance") );
    return page;
  }
}

extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_composer( QWidget *parent, const char* )
  {
    KComponentData instance( "kcmkmail_config_composer" );
    ComposerPage *page = new ComposerPage( instance, parent );
    page->setObjectName( QLatin1String("kcmkmail_config_composer") );
    return page;
  }
}

extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_identity( QWidget *parent, const char* )
  {
    KComponentData instance( "kcmkmail_config_identity" );
    KMail::IdentityPage *page = new KMail::IdentityPage( instance, parent );
    page->setObjectName( QLatin1String("kcmkmail_config_identity") );
    return page;
  }
}

extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_accounts( QWidget *parent, const char* )
  {
    KComponentData instance( "kcmkmail_config_accounts" );
    AccountsPage *page = new AccountsPage( instance, parent );
    page->setObjectName( QLatin1String("kcmkmail_config_accounts") );
    return page;
  }
}

extern "C"
{
  KDE_EXPORT KCModule *create_kmail_config_security( QWidget *parent, const char* )
  {
    KComponentData instance( "kcmkmail_config_security" );
    SecurityPage *page = new SecurityPage( instance, parent );
    page->setObjectName( QLatin1String("kcmkmail_config_security") );
    return page;
  }
}
