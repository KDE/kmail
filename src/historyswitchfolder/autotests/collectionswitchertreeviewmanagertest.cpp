/*
This file is part of KMail, the KDE mail client.
SPDX-FileCopyrightText: 2021-2022 Laurent Montel <montel@kde.org>

SPDX-License-Identifier: GPL-2.0-only
*/

#include "collectionswitchertreeviewmanagertest.h"
#include "historyswitchfolder/collectionswitchertreeview.h"
#include "historyswitchfolder/collectionswitchertreeviewmanager.h"
#include <QTest>
QTEST_MAIN(CollectionSwitcherTreeViewManagerTest)
CollectionSwitcherTreeViewManagerTest::CollectionSwitcherTreeViewManagerTest(QObject *parent)
    : QObject{parent}
{
}

void CollectionSwitcherTreeViewManagerTest::shouldHaveDefaultValues()
{
    CollectionSwitcherTreeViewManager m;
    QVERIFY(!m.parentWidget());

    QVERIFY(m.collectionSwitcherTreeView());
    QVERIFY(m.collectionSwitcherTreeView()->model());
}
