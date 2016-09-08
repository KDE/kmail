/*
  Copyright (c) 2016 Montel Laurent <montel@kde.org>

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


#include "configurepluginslistwidgettest.h"
#include "../configurepluginslistwidget.h"
#include <QHBoxLayout>
#include <QTest>
#include <QTreeWidget>

ConfigurePluginsListWidgetTest::ConfigurePluginsListWidgetTest(QObject *parent)
    : QObject(parent)
{

}

ConfigurePluginsListWidgetTest::~ConfigurePluginsListWidgetTest()
{

}

void ConfigurePluginsListWidgetTest::shouldHaveDefaultValue()
{
    ConfigurePluginsListWidget w;

    QVBoxLayout *mainLayout = w.findChild<QVBoxLayout *>(QStringLiteral("mainlayout"));
    QVERIFY(mainLayout);
    QCOMPARE(mainLayout->margin(), 0);

    QTreeWidget *mListWidget = w.findChild<QTreeWidget *>(QStringLiteral("listwidget"));
    QVERIFY(mListWidget);
}

void ConfigurePluginsListWidgetTest::shouldTestIsEnabled_data()
{
    QTest::addColumn<QStringList>("enabledPluginsList");
    QTest::addColumn<QStringList>("disabledPluginsList");
    QTest::addColumn<bool>("isEnabledByDefault");
    QTest::addColumn<QString>("pluginId");
    QTest::addColumn<bool>("result");

    //Use default value
    QTest::newRow("emptysettings") << QStringList() << QStringList() << true << QStringLiteral("foo") << true;
    QTest::newRow("emptysettings-2") << QStringList() << QStringList() << false << QStringLiteral("foo") << false;

    //Return false each time that pluginid is empty
    QTest::newRow("pluginidempty") << QStringList() << QStringList() << true << QString() << false;
    QTest::newRow("pluginidempty-2") << QStringList() << QStringList() << false << QString() << false;

    //Use setting from user settings
    QStringList disabled{ QStringLiteral("foo") };
    QTest::newRow("usersettingdisabled") << QStringList() << disabled << true << QStringLiteral("foo") << false;
    QTest::newRow("usersettingdisabled-2") << QStringList() << disabled << false << QStringLiteral("foo") << false;


    QStringList enabled{ QStringLiteral("foo") };
    QTest::newRow("usersettingenabled") << enabled << QStringList() << true << QStringLiteral("foo") << true;
    QTest::newRow("usersettingenabled-2") << enabled << QStringList() << false << QStringLiteral("foo") << true;
}

void ConfigurePluginsListWidgetTest::shouldTestIsEnabled()
{
    QFETCH(QStringList, enabledPluginsList);
    QFETCH(QStringList, disabledPluginsList);
    QFETCH(bool, isEnabledByDefault);
    QFETCH(QString, pluginId);
    QFETCH(bool, result);

    QCOMPARE(ConfigurePluginsListWidget::isActivate(enabledPluginsList, disabledPluginsList, isEnabledByDefault, pluginId), result);
}

QTEST_MAIN(ConfigurePluginsListWidgetTest)
