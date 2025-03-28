/*
   SPDX-FileCopyrightText: 2021-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmailplugincheckbeforedeletingmanagerinterface.h"
#include "kmail_debug.h"
#include <MessageViewer/MessageViewerCheckBeforeDeletingInterface>
#include <MessageViewer/MessageViewerCheckBeforeDeletingPlugin>
#include <MessageViewer/MessageViewerCheckBeforeDeletingPluginManager>

KMailPluginCheckBeforeDeletingManagerInterface::KMailPluginCheckBeforeDeletingManagerInterface(QObject *parent)
    : QObject(parent)
{
}

KMailPluginCheckBeforeDeletingManagerInterface::~KMailPluginCheckBeforeDeletingManagerInterface() = default;

void KMailPluginCheckBeforeDeletingManagerInterface::initializePlugins()
{
    if (mWasInitialized) {
        qCDebug(KMAIL_LOG) << "KMailPluginCheckBeforeDeletingManagerInterface : Plugin was already initialized. This is a bug";
        return;
    }
    if (!mParentWidget) {
        qCDebug(KMAIL_LOG) << "KMailPluginCheckBeforeDeletingManagerInterface : Parent is null. This is a bug";
    }
    const QList<MessageViewer::MessageViewerCheckBeforeDeletingPlugin *> lstPlugin =
        MessageViewer::MessageViewerCheckBeforeDeletingPluginManager::self()->pluginsList();
    for (MessageViewer::MessageViewerCheckBeforeDeletingPlugin *plugin : lstPlugin) {
        if (plugin->isEnabled()) {
            auto interface = static_cast<MessageViewer::MessageViewerCheckBeforeDeletingInterface *>(plugin->createInterface(this));
            interface->setParentWidget(mParentWidget);
            interface->createActions(mActionCollection);
            mActions.append(interface->actions());
            mListPluginInterface.append(interface);
        }
    }
    mWasInitialized = true;
}

QWidget *KMailPluginCheckBeforeDeletingManagerInterface::parentWidget() const
{
    return mParentWidget;
}

void KMailPluginCheckBeforeDeletingManagerInterface::setActionCollection(KActionCollection *ac)
{
    mActionCollection = ac;
}

const QList<QAction *> KMailPluginCheckBeforeDeletingManagerInterface::actions() const
{
    return mActions;
}

void KMailPluginCheckBeforeDeletingManagerInterface::setParentWidget(QWidget *newParentWidget)
{
    mParentWidget = newParentWidget;
}

Akonadi::Item::List KMailPluginCheckBeforeDeletingManagerInterface::confirmBeforeDeleting(const Akonadi::Item::List &list)
{
    Akonadi::Item::List currentList = list;
    for (MessageViewer::MessageViewerCheckBeforeDeletingInterface *interface : std::as_const(mListPluginInterface)) {
        currentList = interface->exec(currentList);
    }
    return currentList;
}

#include "moc_kmailplugincheckbeforedeletingmanagerinterface.cpp"
