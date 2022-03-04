/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2022 Laurent Montel <montel@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "subjectlineeditwithautocorrection.h"
#include <KLocalizedString>

SubjectLineEditWithAutoCorrection::SubjectLineEditWithAutoCorrection(QWidget *parent, const QString &configFile)
    : PimCommon::LineEditWithAutoCorrection(parent, configFile)
{
    setActivateLanguageMenu(false);
    setToolTip(i18n("Set a subject for this message"));
}

SubjectLineEditWithAutoCorrection::~SubjectLineEditWithAutoCorrection()
{
}
