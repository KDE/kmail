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
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <config-enterprise.h>
#include "settings/globalsettings.h"
#include "kmkernel.h"
// my headers:
#include "configuredialog.h"
#include "configuredialog_p.h"

#include <KWindowSystem>

// other headers:
#include <assert.h>
#include <stdlib.h>

ConfigureDialog::ConfigureDialog( QWidget *parent, bool modal )
    : KCMultiDialog( parent )
{
    setFaceType( List );
    setButtons( Help | Default | Cancel | Apply | Ok | Reset );
    setModal( modal );
    KWindowSystem::setIcons( winId(), qApp->windowIcon().pixmap( IconSize( KIconLoader::Desktop ), IconSize( KIconLoader::Desktop ) ), qApp->windowIcon().pixmap(IconSize( KIconLoader::Small ), IconSize( KIconLoader::Small ) ) );
    addModule( QLatin1String("kmail_config_identity") );
    addModule( QLatin1String("kmail_config_accounts") );
    addModule( QLatin1String("kmail_config_appearance") );
    addModule( QLatin1String("kmail_config_composer") );
    addModule( QLatin1String("kmail_config_security") );
    addModule( QLatin1String("kmail_config_misc") );

    connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );
    connect( this, SIGNAL(applyClicked()), SLOT(slotApply()) );

    // We store the size of the dialog on hide, because otherwise
    // the KCMultiDialog starts with the size of the first kcm, not
    // the largest one. This way at least after the first showing of
    // the largest kcm the size is kept.
    const int width = GlobalSettings::self()->configureDialogWidth();
    const int height = GlobalSettings::self()->configureDialogHeight();
    if ( width != 0 && height != 0 ) {
        resize( width, height );
    }

}

void ConfigureDialog::hideEvent( QHideEvent *ev )
{
    GlobalSettings::self()->setConfigureDialogWidth( width() );
    GlobalSettings::self()->setConfigureDialogHeight( height() );
    KDialog::hideEvent( ev );
}

ConfigureDialog::~ConfigureDialog()
{
}

void ConfigureDialog::slotApply()
{
    slotApplyClicked();
    KMKernel::self()->slotRequestConfigSync();
    emit configChanged();
}

void ConfigureDialog::slotOk()
{
    slotOkClicked();
    KMKernel::self()->slotRequestConfigSync();
    emit configChanged();
}

