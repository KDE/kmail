/*
  This file is part of KMail, the KDE mail client.
  SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/
#include "collectionswitchertreeviewtest.h"
#include "historyswitchfolder/collectionswitchertreeview.h"
#include <QTest>

QTEST_MAIN(CollectionSwitcherTreeViewTest)

CollectionSwitcherTreeViewTest::CollectionSwitcherTreeViewTest(QObject *parent)
    : QObject{parent}
{
}

void CollectionSwitcherTreeViewTest::shouldHaveDefaultValues()
{
    CollectionSwitcherTreeView w(nullptr);
    QVERIFY(!w.rootIsDecorated());
    QCOMPARE(w.selectionBehavior(), QAbstractItemView::SelectRows);
    QCOMPARE(w.selectionMode(), QAbstractItemView::SingleSelection);
    QCOMPARE(w.textElideMode(), Qt::ElideMiddle);
    QCOMPARE(w.horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    QVERIFY(w.isHeaderHidden());
    QCOMPARE(w.windowFlags(), Qt::Popup | Qt::FramelessWindowHint);
}
