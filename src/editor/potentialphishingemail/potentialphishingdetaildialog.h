/*
  SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#ifndef POTENTIALPHISHINGDETAILDIALOG_H
#define POTENTIALPHISHINGDETAILDIALOG_H

#include <QDialog>
#include "kmail_private_export.h"
class PotentialPhishingDetailWidget;
class KMAILTESTS_TESTS_EXPORT PotentialPhishingDetailDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PotentialPhishingDetailDialog(QWidget *parent = nullptr);
    ~PotentialPhishingDetailDialog();

    void fillList(const QStringList &lst);

private:
    void slotSave();
    void readConfig();
    void writeConfig();
    PotentialPhishingDetailWidget *mPotentialPhishingDetailWidget = nullptr;
};

#endif // POTENTIALPHISHINGDETAILDIALOG_H
