/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2022-2026 Laurent Montel <montel@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "kmail_private_export.h"
#include <qglobal.h>
#include <textautocorrectionwidgets_version.h>

#if TEXTAUTOCORRECTIONWIDGETS_VERSION >= QT_VERSION_CHECK(2, 1, 47)
#include <PimCommon/SpellCheckLineEdit>
#else
#include <PimCommon/LineEditWithAutoCorrection>
#endif

class KMAILTESTS_TESTS_EXPORT SubjectLineEditWithAutoCorrection
#if TEXTAUTOCORRECTIONWIDGETS_VERSION >= QT_VERSION_CHECK(2, 1, 47)
    : public PimCommon::SpellCheckLineEdit
#else
    : public PimCommon::LineEditWithAutoCorrection
#endif
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
