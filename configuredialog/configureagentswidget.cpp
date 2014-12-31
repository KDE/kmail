/*
  Copyright (c) 2013-2015 Montel Laurent <montel@kde.org>

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

#include "agents/sendlateragent/sendlaterutil.h"
#include <akonadi/private/xdgbasedirs_p.h>

#include <KLocalizedString>
#include <KDesktopFile>
#include <KDebug>
#include <KTextEdit>
#include <KGlobal>
#include <KConfigGroup>

#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDBusInterface>
#include <QDBusReply>
#include <QSplitter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFile>
#include <QDir>


ConfigureAgentsWidget::ConfigureAgentsWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *lay = new QHBoxLayout;
    mSplitter = new QSplitter;
    mSplitter->setChildrenCollapsible(false);
    lay->addWidget(mSplitter);

    mTreeWidget = new QTreeWidget;
    QStringList headers;
    headers<<i18n("Activate")<<i18n("Name");
    mTreeWidget->setHeaderLabels(headers);
    mTreeWidget->setSortingEnabled(true);
    mTreeWidget->setRootIsDecorated(false);

    mSplitter->addWidget(mTreeWidget);
    QWidget *w = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout;
    mDescription = new KTextEdit;
    mDescription->setReadOnly(true);
    mDescription->enableFindReplace(false);
    vbox->addWidget(mDescription);
    mConfigure = new QPushButton(i18n("Configure..."));
    mConfigure->setEnabled(false);
    vbox->addWidget(mConfigure);
    w->setLayout(vbox);
    mSplitter->addWidget(w);

    setLayout(lay);
    connect(mTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(slotItemClicked(QTreeWidgetItem*)));
    connect(mTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), SLOT(slotItemClicked(QTreeWidgetItem*)));
    connect(mTreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), SIGNAL(changed()));
    connect(mConfigure, SIGNAL(clicked()), this, SLOT(slotConfigureAgent()));
    connect(mTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(slotConfigureAgent()));
    mAgentPathList = Akonadi::XdgBaseDirs::findAllResourceDirs( "data", QLatin1String( "akonadi/agents" ) );
    initialize();
    readConfig();
}

ConfigureAgentsWidget::~ConfigureAgentsWidget()
{
    writeConfig();
}

void ConfigureAgentsWidget::readConfig()
{
    KConfigGroup group( KGlobal::config(), "ConfigureAgentsWidget" );
    QList<int> size;
    size << 400 << 100;
    mSplitter->setSizes(group.readEntry( "splitter", size));
}

void ConfigureAgentsWidget::writeConfig()
{
    KConfigGroup group( KGlobal::config(), "ConfigureAgentsWidget" );
    group.writeEntry( "splitter", mSplitter->sizes());
}

void ConfigureAgentsWidget::slotItemClicked(QTreeWidgetItem *item)
{
    if (item) {
        if (item->flags() & Qt::ItemIsEnabled) {
            mDescription->setText(item->data(AgentName, Description).toString());
        }
    }
    mConfigure->setEnabled(item);
}

void ConfigureAgentsWidget::addInfos(QTreeWidgetItem *item, const QString &desktopFile)
{
    KDesktopFile config(desktopFile);
    item->setText(AgentName, config.readName());
    const QString descriptionStr = QLatin1String("<b>") + i18n("Description:") + QLatin1String("</b><br>") + config.readComment();
    item->setData(AgentName, Description, descriptionStr);
}

void ConfigureAgentsWidget::initialize()
{
    createItem(QLatin1String("akonadi_sendlater_agent"), QLatin1String("/SendLaterAgent"), QLatin1String("sendlateragent.desktop"));
    createItem(QLatin1String("akonadi_archivemail_agent"), QLatin1String("/ArchiveMailAgent"), QLatin1String("archivemailagent.desktop"));
    createItem(QLatin1String("akonadi_newmailnotifier_agent"), QLatin1String("/NewMailNotifierAgent"), QLatin1String("newmailnotifieragent.desktop"));
    createItem(QLatin1String("akonadi_followupreminder_agent"), QLatin1String("/FollowUpReminder"), QLatin1String("followupreminder.desktop"));
    //Add more
}

void ConfigureAgentsWidget::createItem(const QString &interfaceName, const QString &path, const QString &desktopFileName)
{
    Q_FOREACH(const QString &agentPath, mAgentPathList) {
        QFile file (agentPath + QDir::separator() + desktopFileName);
        if (file.exists()) {
            QTreeWidgetItem *item = new QTreeWidgetItem(mTreeWidget);
            item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
            item->setData(AgentName, InterfaceName, interfaceName);
            item->setData(AgentName, PathName, path);
            addInfos(item, file.fileName());
        }
    }
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

void ConfigureAgentsWidget::slotConfigureAgent()
{
    QTreeWidgetItem *item = mTreeWidget->currentItem();
    if (item) {
        QDBusInterface interface( QLatin1String("org.freedesktop.Akonadi.Agent.") + item->data(AgentName, InterfaceName).toString(), item->data(AgentName, PathName).toString() );
        if (interface.isValid()) {
            interface.call(QLatin1String("showConfigureDialog"), (qlonglong)winId());
        } else {
            kDebug()<<" interface does not exist ";
        }
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
    SendLater::SendLaterUtil::forceReparseConfiguration();
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


