/*

    This file is part of KMail.
    SPDX-FileCopyrightText: 2002-2003 Don Sanders <sanders@kde.org>,
    SPDX-FileCopyrightText: 2003 Zack Rusin <zack@kde.org>,
    Based on the work of Cornelius Schumacher <schumacher@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "kmail_part.h"

#include "kmmainwidget.h"

#include <QVBoxLayout>

#include "foldershortcutactionmanager.h"
#include "kmail_debug.h"
#include "tag/tagactionmanager.h"
#include <Akonadi/ChangeRecorder>
#include <Akonadi/Collection>
#include <Akonadi/EntityDisplayAttribute>
#include <KLocalizedString>
#include <KParts/GUIActivateEvent>
#include <KPluginFactory>
#include <KSharedConfig>
#include <MailCommon/FolderTreeView>
#include <kmailpartadaptor.h>
#include <kparts/mainwindow.h>
#include <kparts/statusbarextension.h>

K_PLUGIN_FACTORY(KMailFactory, registerPlugin<KMailPart>();)

using namespace KMail;

KMailPart::KMailPart(QWidget *parentWidget, QObject *parent, const KPluginMetaData &data, const QVariantList &)
    : KParts::ReadOnlyPart(parent, data)
    , mParentWidget(parentWidget)
{
    setComponentName(QStringLiteral("kmail2"), i18n("KMail2"));
    // local, do the init
    auto mKMailKernel = new KMKernel();
    mKMailKernel->init();
    mKMailKernel->setXmlGuiInstanceName(QStringLiteral("kmail2"));

    // and session management
    mKMailKernel->doSessionManagement();

    // any dead letters?
    mKMailKernel->recoverDeadLetters();

    kmkernel->setupDBus(); // Ok. We are ready for D-Bus requests.
    (void)new KmailpartAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/KMailPart"), this);

    // create a canvas to insert our widget
    auto canvas = new QWidget(parentWidget);
    canvas->setFocusPolicy(Qt::ClickFocus);
    canvas->setObjectName(QStringLiteral("canvas"));
    setWidget(canvas);
    mainWidget = new KMMainWidget(canvas, this, actionCollection(), KSharedConfig::openConfig());
    mainWidget->setObjectName(QStringLiteral("partmainwidget"));
    auto topLayout = new QVBoxLayout(canvas);
    topLayout->addWidget(mainWidget);
    topLayout->setContentsMargins({});
    mainWidget->setFocusPolicy(Qt::ClickFocus);
    auto statusBar = new KParts::StatusBarExtension(this);
    statusBar->addStatusBarItem(mainWidget->vacationScriptIndicator(), 2, false);
    statusBar->addStatusBarItem(mainWidget->zoomLabelIndicator(), 3, false);
    statusBar->addStatusBarItem(mainWidget->dkimWidgetInfo(), 4, false);

    setXMLFile(QStringLiteral("kmail_part.rc"), true);
    connect(mainWidget, &KMMainWidget::captionChangeRequest, this, &KMailPart::setWindowCaption);
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
    return true;
}

//-----------------------------------------------------------------------------

void KMailPart::guiActivateEvent(KParts::GUIActivateEvent *e)
{
    KParts::ReadOnlyPart::guiActivateEvent(e);
    if (e->activated()) {
        mainWidget->initializeFilterActions(true);
        mainWidget->tagActionManager()->createActions();
        mainWidget->folderShortcutActionManager()->createActions();
        mainWidget->populateMessageListStatusFilterCombo();
        mainWidget->initializePluginActions();

        const QString title = mainWidget->fullCollectionPath();
        if (!title.isEmpty()) {
            Q_EMIT setWindowCaption(title);
        }
    }
}

void KMailPart::exit()
{
    delete this;
}

void KMailPart::save()
{
    /*TODO*/
}

#include "kmail_part.moc"

#include "moc_kmail_part.cpp"
