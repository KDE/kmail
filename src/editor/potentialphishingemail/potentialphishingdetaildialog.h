/*
  SPDX-FileCopyrightText: 2015-2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include "kmail_private_export.h"
#include <QDialog>
class PotentialPhishingDetailWidget;
class KMAILTESTS_TESTS_EXPORT PotentialPhishingDetailDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PotentialPhishingDetailDialog(QWidget *parent = nullptr);
    ~PotentialPhishingDetailDialog() override;

    void fillList(const QStringList &lst);

private:
    KMAIL_NO_EXPORT void slotSave();
    KMAIL_NO_EXPORT void readConfig();
    KMAIL_NO_EXPORT void writeConfig();
    PotentialPhishingDetailWidget *const mPotentialPhishingDetailWidget;
};
