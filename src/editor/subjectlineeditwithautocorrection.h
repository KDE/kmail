/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2022-2026 Laurent Montel <montel@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "kmail_private_export.h"
#include <PimCommon/LineEditWithAutoCorrection>
#include <textautocorrectionwidgets_version.h>

class KMAILTESTS_TESTS_EXPORT SubjectLineEditWithAutoCorrection
#if TEXTAUTOCORRECTIONWIDGETS_VERSION >= QT_VERSION_CHECK(2, 1, 43)
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
