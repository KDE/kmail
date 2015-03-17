/*  -*- mode: C++; c-file-style: "gnu" -*-

    This file is part of KMail.
    Copyright (c) 2002-2003 Don Sanders <sanders@kde.org>,
    Copyright (c) 2003      Zack Rusin  <zack@kde.org>,
    Based on the work of Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

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

#include "kmail_part.h"

#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kmstartup.h"
#include "aboutdata.h"

#include <QVBoxLayout>
#include <QLabel>

#include <kparts/statusbarextension.h>
#include <kparts/mainwindow.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <ksettings/dispatcher.h>
#include <kmailpartadaptor.h>
#include <kicon.h>
#include <akonadi/collection.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/changerecorder.h>
#include "foldertreeview.h"
#include "tag/tagactionmanager.h"
#include "foldershortcutactionmanager.h"

#include <kglobal.h>

K_PLUGIN_FACTORY( KMailFactory, registerPlugin<KMailPart>(); )
K_EXPORT_PLUGIN( KMailFactory( KMail::AboutData() ) )

using namespace KMail;

KMailPart::KMailPart(QWidget *parentWidget, QObject *parent, const QVariantList &) :
    KParts::ReadOnlyPart( parent ),
    mParentWidget( parentWidget )
{
    kDebug() << "InstanceName:" << KGlobal::mainComponent().componentName();
    setComponentData(KMailFactory::componentData());
    kDebug() << "InstanceName:" << KGlobal::mainComponent().componentName();

    // import i18n data and icons from libraries:
    KMail::insertLibraryCataloguesAndIcons();


    //local, do the init
    KMKernel *mKMailKernel = new KMKernel();
    mKMailKernel->init();
    mKMailKernel->setXmlGuiInstance( KMailFactory::componentData() );

    // and session management
    mKMailKernel->doSessionManagement();

    // any dead letters?
    mKMailKernel->recoverDeadLetters();

    kmkernel->setupDBus(); // Ok. We are ready for D-Bus requests.
    (void) new KmailpartAdaptor( this );
    QDBusConnection::sessionBus().registerObject( QLatin1String("/KMailPart"), this );

    // create a canvas to insert our widget
    QWidget *canvas = new QWidget( parentWidget );
    canvas->setFocusPolicy(Qt::ClickFocus);
    canvas->setObjectName( QLatin1String("canvas") );
    setWidget(canvas);
    KIconLoader::global()->addAppDir( QLatin1String("libkdepim") );
    mainWidget = new KMMainWidget( canvas, this, actionCollection(),
                                   KGlobal::config() );
    mainWidget->setObjectName( QLatin1String("partmainwidget") );
    QVBoxLayout *topLayout = new QVBoxLayout(canvas);
    topLayout->addWidget(mainWidget);
    topLayout->setMargin(0);
    mainWidget->setFocusPolicy(Qt::ClickFocus);
    KParts::StatusBarExtension *statusBar  = new KParts::StatusBarExtension(this);
    statusBar->addStatusBarItem( mainWidget->vacationScriptIndicator(), 2, false );

    setXMLFile( QLatin1String("kmail_part.rc"), true );

    KSettings::Dispatcher::registerComponent( KMailFactory::componentData(), mKMailKernel, "slotConfigChanged" );
}

KMailPart::~KMailPart()
{
    kDebug() << "Closing last KMMainWin: stopping mail check";
    // Running KIO jobs prevent kapp from exiting, so we need to kill them
    // if they are only about checking mail (not important stuff like moving messages)
    mainWidget->destruct();
    kmkernel->cleanup();
    delete kmkernel;
}

void KMailPart::updateQuickSearchText()
{
    mainWidget->updateQuickSearchLineText();
}

bool KMailPart::openFile()
{
    kDebug();

    mainWidget->show();
    return true;
}

//-----------------------------------------------------------------------------

void KMailPart::guiActivateEvent(KParts::GUIActivateEvent *e)
{
    kDebug();
    KParts::ReadOnlyPart::guiActivateEvent(e);
    mainWidget->initializeFilterActions();
    mainWidget->tagActionManager()->createActions();
    mainWidget->folderShortcutActionManager()->createActions();
    mainWidget->updateVacationScriptStatus();
    mainWidget->populateMessageListStatusFilterCombo();
}

void KMailPart::exit()
{
    delete this;
}

QWidget* KMailPart::parentWidget() const
{
    return mParentWidget;
}

