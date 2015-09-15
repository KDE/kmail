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
#include "configureagents/configureagentlistview.h"

#include "libsendlater/sendlaterutil.h"
#include <akonadi/private/xdgbasedirs_p.h>

#include <KLocalizedString>
#include <KDesktopFile>
#include "kmail_debug.h"
#include <KTextEdit>
#include <KConfigGroup>

#include <QDBusInterface>
#include <QDBusReply>
#include <QSplitter>
#include <QFile>
#include <QDir>
#include <KSharedConfig>
#include <QHBoxLayout>

ConfigureAgentsWidget::ConfigureAgentsWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *lay = new QHBoxLayout;
    mSplitter = new QSplitter;
    mSplitter->setChildrenCollapsible(false);
    lay->addWidget(mSplitter);

    mConfigureAgentListView = new ConfigureAgentListView;

    mSplitter->addWidget(mConfigureAgentListView);
    mDescription = new KTextEdit;
    mDescription->setReadOnly(true);
    mDescription->enableFindReplace(false);
    mSplitter->addWidget(mDescription);

    connect(mConfigureAgentListView, &ConfigureAgentListView::descriptionChanged, mDescription, &QTextEdit::setText);
    connect(mConfigureAgentListView, &ConfigureAgentListView::agentChanged, this, &ConfigureAgentsWidget::changed);

    setLayout(lay);
    mAgentPathList = Akonadi::XdgBaseDirs::findAllResourceDirs("data", QStringLiteral("akonadi/agents"));
    initialize();
    readConfig();
}

ConfigureAgentsWidget::~ConfigureAgentsWidget()
{
    writeConfig();
}

void ConfigureAgentsWidget::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "ConfigureAgentsWidget");
    QList<int> size;
    size << 400 << 100;
    mSplitter->setSizes(group.readEntry("splitter", size));
}

void ConfigureAgentsWidget::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "ConfigureAgentsWidget");
    group.writeEntry("splitter", mSplitter->sizes());
}

void ConfigureAgentsWidget::addInfos(const QString &desktopFile, ConfigureAgentItem &item)
{
    KDesktopFile config(desktopFile);
    item.setAgentName(config.readName());
    const QString descriptionStr = QLatin1String("<b>") + i18n("Description:") + QLatin1String("</b><br>") + config.readComment();
    item.setDescription(descriptionStr);
}

void ConfigureAgentsWidget::initialize()
{
    QVector<ConfigureAgentItem> lst;
    createItem(QStringLiteral("akonadi_sendlater_agent"), QStringLiteral("/SendLaterAgent"), QStringLiteral("sendlateragent.desktop"), lst);
    createItem(QStringLiteral("akonadi_archivemail_agent"), QStringLiteral("/ArchiveMailAgent"), QStringLiteral("archivemailagent.desktop"), lst);
    createItem(QStringLiteral("akonadi_newmailnotifier_agent"), QStringLiteral("/NewMailNotifierAgent"), QStringLiteral("newmailnotifieragent.desktop"), lst);
    createItem(QStringLiteral("akonadi_followupreminder_agent"), QStringLiteral("/FollowUpReminder"), QStringLiteral("followupreminder.desktop"), lst);
    //Add more
    mConfigureAgentListView->setAgentItems(lst);
}

void ConfigureAgentsWidget::createItem(const QString &interfaceName, const QString &path, const QString &desktopFileName, QVector<ConfigureAgentItem> &listItem)
{
    Q_FOREACH (const QString &agentPath, mAgentPathList) {
        QFile file(agentPath + QDir::separator() + desktopFileName);
        if (file.exists()) {
            ConfigureAgentItem item;
            item.setInterfaceName(interfaceName);
            item.setPath(path);
            bool failed = false;
            const bool enabled = agentActivateState(interfaceName, path, failed);
            item.setChecked(enabled);
            item.setFailed(failed);
            addInfos(file.fileName(), item);
            listItem.append(item);
        }
    }
}

bool ConfigureAgentsWidget::agentActivateState(const QString &interfaceName, const QString &pathName, bool &failed)
{
    failed = false;
    QDBusInterface interface(QLatin1String("org.freedesktop.Akonadi.Agent.") + interfaceName, pathName);
    if (interface.isValid()) {
        QDBusReply<bool> enabled = interface.call(QStringLiteral("enabledAgent"));
        if (enabled.isValid()) {
            return enabled;
        } else {
            qCDebug(KMAIL_LOG) << interfaceName << "doesn't have enabledAgent function";
            failed = true;
            return false;
        }
    } else {
        failed = true;
        qCDebug(KMAIL_LOG) << interfaceName << "does not exist ";
    }
    return false;
}

void ConfigureAgentsWidget::save()
{
    mConfigureAgentListView->save();
    SendLater::SendLaterUtil::forceReparseConfiguration();
}

QString ConfigureAgentsWidget::helpAnchor() const
{
    return QString();
}

void ConfigureAgentsWidget::doLoadFromGlobalSettings()
{
    //initialize();
}

void ConfigureAgentsWidget::doResetToDefaultsOther()
{
    mConfigureAgentListView->resetToDefault();
}

