/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "configureagentswidget.h"

#include <KLocale>
#include <KDebug>

#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDBusInterface>
#include <QDBusReply>


ConfigureAgentsWidget::ConfigureAgentsWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout;
    mTreeWidget = new QTreeWidget;
    QStringList headers;
    headers<<i18n("Activate")<<i18n("Agent Name");
    mTreeWidget->setHeaderLabels(headers);
    mTreeWidget->setSortingEnabled(true);
    mTreeWidget->setRootIsDecorated(false);

    lay->addWidget(mTreeWidget);
    setLayout(lay);
    initialize();
    connect(mTreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), SIGNAL(changed()));
}

ConfigureAgentsWidget::~ConfigureAgentsWidget()
{

}

void ConfigureAgentsWidget::initialize()
{
    createItem(QLatin1String("akonadi_sendlater_agent"), QLatin1String("/SendLaterAgent"), i18n("Send Later Agent"));
    createItem(QLatin1String("akonadi_archivemail_agent"), QLatin1String("/ArchiveMailAgent"), i18n("Archive Mail Agent"));
    createItem(QLatin1String("akonadi_newmailnotifier_agent"), QLatin1String("/NewMailNotifierAgent"), i18n("New Mail Notifier Agent"));
    createItem(QLatin1String("akonadi_folderarchive_agent"), QLatin1String("/FolderArchiveAgent"), i18n("Archive Folder Agent"));
    //Add more
}

void ConfigureAgentsWidget::createItem(const QString &interfaceName, const QString &path, const QString &name)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(mTreeWidget);
    item->setText(AgentName, name);
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    item->setData(AgentName, InterfaceName, interfaceName);
    item->setData(AgentName, PathName, path);
}

bool ConfigureAgentsWidget::agentActivateState(const QString &interfaceName, const QString &pathName, bool &failed)
{
    failed = false;
    QDBusInterface interface( QLatin1String("org.freedesktop.Akonadi.Agent.") + interfaceName, pathName );
    if (interface.isValid()) {
        QDBusReply<bool> enabled = interface.call(QLatin1String("enabledAgent"));
        if (enabled.isValid()) {
            return enabled;
        } else {
            kDebug()<<interfaceName << "doesn't have enabledAgent function";
            failed = true;
            return false;
        }
    } else {
        failed = true;
        kDebug()<<interfaceName << "does not exist ";
    }
    return false;
}

void ConfigureAgentsWidget::changeAgentActiveState(bool enable, const QString &interfaceName, const QString &pathName)
{
    QDBusInterface interface( QLatin1String("org.freedesktop.Akonadi.Agent.") + interfaceName, pathName );
    if (interface.isValid()) {
        interface.call(QLatin1String("setEnableAgent"), enable);
    } else {
        kDebug()<<interfaceName << "does not exist ";
    }
}

void ConfigureAgentsWidget::save()
{
    const int numberOfElement(mTreeWidget->topLevelItemCount());
    for (int i=0; i <numberOfElement; ++i) {
        QTreeWidgetItem *item = mTreeWidget->topLevelItem(i);
        if (item->flags() & Qt::ItemIsEnabled)
            changeAgentActiveState((item->checkState(AgentState) == Qt::Checked), item->data(AgentName, InterfaceName).toString(), item->data(AgentName, PathName).toString());
    }
}

QString ConfigureAgentsWidget::helpAnchor() const
{
    return QString();
}

void ConfigureAgentsWidget::doLoadFromGlobalSettings()
{
    const int numberOfElement(mTreeWidget->topLevelItemCount());
    for (int i=0; i <numberOfElement; ++i) {
        QTreeWidgetItem *item = mTreeWidget->topLevelItem(i);
        bool failed;
        const bool enabled = agentActivateState(item->data(AgentName, InterfaceName).toString(), item->data(AgentName, PathName).toString(), failed);
        item->setCheckState(AgentState, enabled ? Qt::Checked : Qt::Unchecked);
        if (failed) {
            item->setFlags(Qt::NoItemFlags);
            item->setBackgroundColor(AgentState, Qt::red);
        }
    }
}

void ConfigureAgentsWidget::doResetToDefaultsOther()
{
    const int numberOfElement(mTreeWidget->topLevelItemCount());
    for (int i=0; i <numberOfElement; ++i) {
        QTreeWidgetItem *item = mTreeWidget->topLevelItem(i);
        if (item->flags() & Qt::ItemIsEnabled)
            item->setCheckState(AgentState, Qt::Checked);
    }
}


#include "configureagentswidget.moc"
