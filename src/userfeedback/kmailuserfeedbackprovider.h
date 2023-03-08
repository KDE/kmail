/*
   SPDX-FileCopyrightText: 2020-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_export.h"
#ifdef USE_KUSERFEEDBACK_QT6
#include <KUserFeedbackQt6/Provider>
#else
#include <KUserFeedback/Provider>
#endif

class KMAIL_EXPORT KMailUserFeedbackProvider : public KUserFeedback::Provider
{
    Q_OBJECT
public:
    explicit KMailUserFeedbackProvider(QObject *parent = nullptr);
    ~KMailUserFeedbackProvider() override;
};
