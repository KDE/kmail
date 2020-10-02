/*
  SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#ifndef POTENTIALPHISHINGDETAILWIDGET_H
#define POTENTIALPHISHINGDETAILWIDGET_H

#include <QWidget>
#include "kmail_private_export.h"
class QListWidget;
class KMAILTESTS_TESTS_EXPORT PotentialPhishingDetailWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PotentialPhishingDetailWidget(QWidget *parent = nullptr);
    ~PotentialPhishingDetailWidget();

    void save();

    void fillList(const QStringList &lst);
private:
    QListWidget *const mListWidget = nullptr;
};

#endif // POTENTIALPHISHINGDETAILWIDGET_H
