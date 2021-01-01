/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#ifndef POTENTIALPHISHINGEMAILWARNING_H
#define POTENTIALPHISHINGEMAILWARNING_H

#include <KMessageWidget>
#include "kmail_private_export.h"
class KMAILTESTS_TESTS_EXPORT PotentialPhishingEmailWarning : public KMessageWidget
{
    Q_OBJECT
public:
    explicit PotentialPhishingEmailWarning(QWidget *parent = nullptr);
    ~PotentialPhishingEmailWarning();

    void setPotentialPhisingEmail(const QStringList &lst);

Q_SIGNALS:
    void sendNow();

private:
    void slotShowDetails(const QString &link);
    QStringList mPotentialPhishingEmails;
};

#endif // POTENTIALPHISHINGEMAILWARNING_H
