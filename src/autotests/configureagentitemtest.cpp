/*
  Copyright (c) 2015-2016 Montel Laurent <montel@kde.org>

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

#include "configureagentitemtest.h"
#include "../configuredialog/configureagents/configureagentitem.h"
#include <qtest.h>

ConfigureAgentItemTest::ConfigureAgentItemTest(QObject *parent)
    : QObject(parent)
{

}

ConfigureAgentItemTest::~ConfigureAgentItemTest()
{

}

void ConfigureAgentItemTest::shouldHaveDefaultValue()
{
    ConfigureAgentItem item;
    QVERIFY(item.agentName().isEmpty());
    QVERIFY(!item.checked());
    QVERIFY(!item.failed());
    QVERIFY(item.description().isEmpty());
    QVERIFY(item.path().isEmpty());
    QVERIFY(item.interfaceName().isEmpty());
}

void ConfigureAgentItemTest::shouldAssignValue()
{
    ConfigureAgentItem item;
    QString agentName = QStringLiteral("foo");
    QString description = QStringLiteral("bla");
    QString interface = QStringLiteral("interface");
    QString path = QStringLiteral("path");
    bool failed = true;
    bool checked = true;
    item.setAgentName(agentName);
    item.setDescription(description);
    item.setInterfaceName(interface);
    item.setFailed(failed);
    item.setPath(path);
    item.setChecked(checked);
    QCOMPARE(item.agentName(), agentName);
    QCOMPARE(item.description(), description);
    QCOMPARE(item.path(), path);
    QCOMPARE(item.interfaceName(), interface);
    QCOMPARE(item.failed(), failed);
    QCOMPARE(item.checked(), checked);
}

void ConfigureAgentItemTest::shouldBeEqual()
{
    ConfigureAgentItem item;
    QString agentName = QStringLiteral("foo");
    QString description = QStringLiteral("bla");
    QString interface = QStringLiteral("interface");
    QString path = QStringLiteral("path");
    bool failed = true;
    bool checked = true;
    item.setAgentName(agentName);
    item.setDescription(description);
    item.setInterfaceName(interface);
    item.setFailed(failed);
    item.setPath(path);
    item.setChecked(checked);
    ConfigureAgentItem copyItem;
    copyItem = item;
    QVERIFY(copyItem == item);
}

QTEST_MAIN(ConfigureAgentItemTest)
