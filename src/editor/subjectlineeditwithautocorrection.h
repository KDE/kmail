/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2022 Laurent Montel <montel@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "kmail_private_export.h"
#include <PimCommonAutoCorrection/LineEditWithAutoCorrection>
class KMAILTESTS_TESTS_EXPORT SubjectLineEditWithAutoCorrection : public PimCommonAutoCorrection::LineEditWithAutoCorrection
{
    Q_OBJECT
public:
    explicit SubjectLineEditWithAutoCorrection(QWidget *parent, const QString &configFile);
    ~SubjectLineEditWithAutoCorrection() override;

protected:
    void dropEvent(QDropEvent *event) override;

Q_SIGNALS:
    void handleMimeData(const QMimeData *mimeData);
};
