/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <KMessageWidget>
class KMAILTESTS_TESTS_EXPORT ExternalEditorWarning : public KMessageWidget
{
    Q_OBJECT
public:
    explicit ExternalEditorWarning(QWidget *parent = nullptr);
    ~ExternalEditorWarning() override;
};

