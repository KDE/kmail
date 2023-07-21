/*
   SPDX-FileCopyrightText: 2020-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include "kmail_private_export.h"
#ifdef USE_KUSERFEEDBACK_QT6
#include <KUserFeedbackQt6/AbstractDataSource>
#else
#include <KUserFeedback/AbstractDataSource>
#endif

class KMAILTESTS_TESTS_EXPORT AccountInfoSource : public KUserFeedback::AbstractDataSource
{
public:
    AccountInfoSource();

    Q_REQUIRED_RESULT QString name() const override;
    Q_REQUIRED_RESULT QString description() const override;

    Q_REQUIRED_RESULT QVariant data() override;

private:
    void updateAccountInfo(const QString &resourceName, int numberOfResource, QVariantList &l);
};
