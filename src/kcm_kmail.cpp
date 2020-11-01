/*
 *   kmail: KDE mail client
 *   SPDX-FileCopyrightText: 2000 Espen Sand <espen@kde.org>
 *   SPDX-FileCopyrightText: 2001-2003 Marc Mutz <mutz@kde.org>
 *   Contains code segments and ideas from earlier kmail dialog code.
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
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
#include "configuredialog/configurepluginpage.h"
#include "identity/identitypage.h"
#include <KCModule>

//----------------------------
// KCM stuff
//----------------------------
extern "C"
{
Q_DECL_EXPORT KCModule *create_kmail_config_misc(QWidget *parent, const char *)
{
    auto *page = new MiscPage(parent);
    page->setObjectName(QStringLiteral("kcmkmail_config_misc"));
    return page;
}
}

extern "C"
{
Q_DECL_EXPORT KCModule *create_kmail_config_appearance(QWidget *parent, const char *)
{
    auto *page
        = new AppearancePage(parent);
    page->setObjectName(QStringLiteral("kcmkmail_config_appearance"));
    return page;
}
}

extern "C"
{
Q_DECL_EXPORT KCModule *create_kmail_config_composer(QWidget *parent, const char *)
{
    auto *page = new ComposerPage(parent);
    page->setObjectName(QStringLiteral("kcmkmail_config_composer"));
    return page;
}
}

extern "C"
{
Q_DECL_EXPORT KCModule *create_kmail_config_accounts(QWidget *parent, const char *)
{
    auto *page = new AccountsPage(parent);
    page->setObjectName(QStringLiteral("kcmkmail_config_accounts"));
    return page;
}
}

extern "C"
{
Q_DECL_EXPORT KCModule *create_kmail_config_security(QWidget *parent, const char *)
{
    auto *page = new SecurityPage(parent);
    page->setObjectName(QStringLiteral("kcmkmail_config_security"));
    return page;
}
}

extern "C"
{
Q_DECL_EXPORT KCModule *create_kmail_config_plugins(QWidget *parent, const char *)
{
    auto *page = new ConfigurePluginPage(parent);
    page->setObjectName(QStringLiteral("kcmkmail_config_plugins"));
    return page;
}
}
