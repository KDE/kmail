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

#include "configurepluginswidgettest.h"
#include "../configurepluginswidget.h"
#include "../configurepluginslistwidget.h"

#include <KTextEdit>
#include <KMessageWidget>
#include <QSplitter>
#include <QTest>

ConfigurePluginsWidgetTest::ConfigurePluginsWidgetTest(QObject *parent)
    : QObject(parent)
{

}

ConfigurePluginsWidgetTest::~ConfigurePluginsWidgetTest()
{

}

void ConfigurePluginsWidgetTest::shouldHaveDefaultValue()
{
    ConfigurePluginsWidget w;

    KMessageWidget *messageWidget = w.findChild<KMessageWidget *>(QStringLiteral("messagewidget"));
    QVERIFY(messageWidget);
    QVERIFY(!messageWidget->isCloseButtonVisible());
    QVERIFY(!messageWidget->text().isEmpty());

    QSplitter *mSplitter = w.findChild<QSplitter *>(QStringLiteral("splitter"));
    QVERIFY(mSplitter);
    QCOMPARE(mSplitter->count(), 2);

    ConfigurePluginsListWidget *mConfigureListWidget = mSplitter->findChild<ConfigurePluginsListWidget *>(QStringLiteral("configureListWidget"));
    QVERIFY(mConfigureListWidget);

    KTextEdit *mDescription = mSplitter->findChild<KTextEdit *>(QStringLiteral("description"));
    QVERIFY(mDescription);
    QVERIFY(mDescription->isReadOnly());
    QVERIFY(mDescription->document()->isEmpty());
}

QTEST_MAIN(ConfigurePluginsWidgetTest)
