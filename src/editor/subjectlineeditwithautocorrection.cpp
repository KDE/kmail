/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2022-2024 Laurent Montel <montel@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "subjectlineeditwithautocorrection.h"
#include <KLocalizedString>
#include <QDropEvent>
#include <QMimeData>

SubjectLineEditWithAutoCorrection::SubjectLineEditWithAutoCorrection(QWidget *parent, const QString &configFile)
    : PimCommon::LineEditWithAutoCorrection(parent, configFile)
{
    setActivateLanguageMenu(false);
    setToolTip(i18nc("@info:tooltip", "Set a subject for this message"));
}

SubjectLineEditWithAutoCorrection::~SubjectLineEditWithAutoCorrection() = default;

void SubjectLineEditWithAutoCorrection::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        Q_EMIT handleMimeData(mimeData);
        event->accept();
        return;
    }
    PimCommon::LineEditWithAutoCorrection::dropEvent(event);
}

#include "moc_subjectlineeditwithautocorrection.cpp"
