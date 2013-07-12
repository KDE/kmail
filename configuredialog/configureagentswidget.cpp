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
#include <QListWidget>
#include <QDBusInterface>
#include <QDBusReply>


ConfigureAgentsWidget::ConfigureAgentsWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout;
    mListWidget = new QListWidget;
    lay->addWidget(mListWidget);
    setLayout(lay);
    initialize();
    connect(mListWidget, SIGNAL(itemChanged(QListWidgetItem*)), SIGNAL(changed()));
}

ConfigureAgentsWidget::~ConfigureAgentsWidget()
{

}

void ConfigureAgentsWidget::initialize()
{
    createItem(QLatin1String("akonadi_sendlater_agent"), QLatin1String("/SendLaterAgent"), i18n("Send Later Agent"));
    createItem(QLatin1String("akonadi_archivemail_agent"), QLatin1String("/ArchiveMailAgent"), i18n("Achive Mail Agent"));
    createItem(QLatin1String("akonadi_newmailnotifier_agent"), QLatin1String("/NewMailNotifierAgent"), i18n("New Mail Notifier Agent"));
    //Add more
}

void ConfigureAgentsWidget::createItem(const QString &interfaceName, const QString &path, const QString &name)
{
    QListWidgetItem *item = new QListWidgetItem(name, mListWidget);
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    item->setData(InterfaceName, interfaceName);
    item->setData(PathName, path);
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
    const int numberOfElement(mListWidget->count());
    for (int i=0; i <numberOfElement; ++i) {
        QListWidgetItem *item = mListWidget->item(i);
        if (item->flags() & Qt::ItemIsEnabled)
            changeAgentActiveState((item->checkState() == Qt::Checked), item->data(InterfaceName).toString(), item->data(PathName).toString());
    }
}

QString ConfigureAgentsWidget::helpAnchor() const
{
    return QString();
}

void ConfigureAgentsWidget::doLoadFromGlobalSettings()
{
    const int numberOfElement(mListWidget->count());
    for (int i=0; i <numberOfElement; ++i) {
        QListWidgetItem *item = mListWidget->item(i);
        bool failed;
        const bool enabled = agentActivateState(item->data(InterfaceName).toString(), item->data(PathName).toString(), failed);
        item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
        if (failed) {
            item->setFlags(Qt::NoItemFlags);
            item->setBackgroundColor(Qt::red);
        }
    }
}

void ConfigureAgentsWidget::doResetToDefaultsOther()
{
    const int numberOfElement(mListWidget->count());
    for (int i=0; i <numberOfElement; ++i) {
        QListWidgetItem *item = mListWidget->item(i);
        if (item->flags() & Qt::ItemIsEnabled)
            item->setCheckState(Qt::Checked);
    }
}


#include "configureagentswidget.moc"
