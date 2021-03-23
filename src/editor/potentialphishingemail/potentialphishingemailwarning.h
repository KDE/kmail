/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include "kmail_private_export.h"
#include <KMessageWidget>
class KMAILTESTS_TESTS_EXPORT PotentialPhishingEmailWarning : public KMessageWidget
{
    Q_OBJECT
public:
    explicit PotentialPhishingEmailWarning(QWidget *parent = nullptr);
    ~PotentialPhishingEmailWarning() override;

    void setPotentialPhisingEmail(const QStringList &lst);

Q_SIGNALS:
    void sendNow();

private:
    void slotShowDetails(const QString &link);
    QStringList mPotentialPhishingEmails;
};

