/*

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

#include <kparts/statusbarextension.h>
#include <kparts/mainwindow.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kiconloader.h>
#include "kmail_debug.h"
#include <ksettings/dispatcher.h>
#include <kmailpartadaptor.h>
#include <AkonadiCore/collection.h>
#include <AkonadiCore/entitydisplayattribute.h>
#include <AkonadiCore/changerecorder.h>
#include "foldertreeview.h"
#include "tag/tagactionmanager.h"
#include "foldershortcutactionmanager.h"

#include <KSharedConfig>

K_PLUGIN_FACTORY(KMailFactory, registerPlugin<KMailPart>();)

using namespace KMail;

KMailPart::KMailPart(QWidget *parentWidget, QObject *parent, const QVariantList &) :
    KParts::ReadOnlyPart(parent),
    mParentWidget(parentWidget)
{
    qCDebug(KMAIL_LOG) << "InstanceName:" << KComponentData::mainComponent().componentName();
#pragma message("port QT5")

    //QT5 setComponentData(KMailFactory::componentData());
    qCDebug(KMAIL_LOG) << "InstanceName:" << KComponentData::mainComponent().componentName();

    // Migrate to xdg path
    KMail::migrateConfig();

    // import i18n data and icons from libraries:
    KMail::insertLibraryCataloguesAndIcons();

    //local, do the init
    KMKernel *mKMailKernel = new KMKernel();
    mKMailKernel->init();
    mKMailKernel->setXmlGuiInstanceName(QLatin1String("kmail"));

    // and session management
    mKMailKernel->doSessionManagement();

    // any dead letters?
    mKMailKernel->recoverDeadLetters();

    kmkernel->setupDBus(); // Ok. We are ready for D-Bus requests.
    (void) new KmailpartAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QLatin1String("/KMailPart"), this);

    // create a canvas to insert our widget
    QWidget *canvas = new QWidget(parentWidget);
    canvas->setFocusPolicy(Qt::ClickFocus);
    canvas->setObjectName(QLatin1String("canvas"));
    setWidget(canvas);
    KIconLoader::global()->addAppDir(QLatin1String("libkdepim"));
    mainWidget = new KMMainWidget(canvas, this, actionCollection(),
                                  KSharedConfig::openConfig());
    mainWidget->setObjectName(QLatin1String("partmainwidget"));
    QVBoxLayout *topLayout = new QVBoxLayout(canvas);
    topLayout->addWidget(mainWidget);
    topLayout->setMargin(0);
    mainWidget->setFocusPolicy(Qt::ClickFocus);
    KParts::StatusBarExtension *statusBar  = new KParts::StatusBarExtension(this);
    statusBar->addStatusBarItem(mainWidget->vacationScriptIndicator(), 2, false);

    // Get to know when the user clicked on a folder in the KMail part and update the headerWidget of Kontact
    connect(mainWidget->folderTreeView(), SIGNAL(currentChanged(Akonadi::Collection)),
            this, SLOT(slotFolderChanged(Akonadi::Collection)));
    connect(kmkernel->folderCollectionMonitor(), SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)),
            this, SLOT(slotCollectionChanged(Akonadi::Collection,QSet<QByteArray>)));
    setXMLFile(QLatin1String("kmail_part.rc"), true);
#pragma message("port QT5")

    //QT5 KSettings::Dispatcher::registerComponent( KMailFactory::componentData(), mKMailKernel, "slotConfigChanged" );
}

KMailPart::~KMailPart()
{
    qCDebug(KMAIL_LOG) << "Closing last KMMainWin: stopping mail check";
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
    qCDebug(KMAIL_LOG);

    mainWidget->show();
    return true;
}

void KMailPart::slotFolderChanged(const Akonadi::Collection &col)
{
    //Don't know which code use it
    if (col.isValid()) {
        emit textChanged(col.name());
        if (col.hasAttribute<Akonadi::EntityDisplayAttribute>() &&
                !col.attribute<Akonadi::EntityDisplayAttribute>()->iconName().isEmpty()) {
            emit iconChanged(col.attribute<Akonadi::EntityDisplayAttribute>()->icon().pixmap(22, 22));
        }
    }
}
void KMailPart::slotCollectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &attributeNames)
{
    if (attributeNames.contains("ENTITYDISPLAY") || attributeNames.contains("NAME")) {
        slotFolderChanged(collection);
    }
}

//-----------------------------------------------------------------------------

void KMailPart::guiActivateEvent(KParts::GUIActivateEvent *e)
{
    qCDebug(KMAIL_LOG);
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

QWidget *KMailPart::parentWidget() const
{
    return mParentWidget;
}
#include "kmail_part.moc"
