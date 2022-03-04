/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2022 Laurent Montel <montel@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "kmail_private_export.h"
#include <PimCommon/LineEditWithAutoCorrection>
class KMAILTESTS_TESTS_EXPORT SubjectLineEditWithAutoCorrection : public PimCommon::LineEditWithAutoCorrection
{
    Q_OBJECT
public:
    explicit SubjectLineEditWithAutoCorrection(QWidget *parent, const QString &configFile);
    ~SubjectLineEditWithAutoCorrection() override;
};
