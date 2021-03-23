/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#ifndef POTENTIALPHISHINGDETAILWIDGETTEST_H
#define POTENTIALPHISHINGDETAILWIDGETTEST_H

#include <QObject>

class PotentialPhishingDetailWidgetTest : public QObject
{
    Q_OBJECT
public:
    explicit PotentialPhishingDetailWidgetTest(QObject *parent = nullptr);
    ~PotentialPhishingDetailWidgetTest() override;
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldFillList();
    void shouldClearListBeforeToAddNew();
    void shouldNotAddDuplicateEntries();
};

#endif // POTENTIALPHISHINGDETAILWIDGETTEST_H
