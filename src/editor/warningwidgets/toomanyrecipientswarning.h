/*
   SPDX-FileCopyrightText: 2021-2022 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once
#include "kmail_private_export.h"
#include <KMessageWidget>

class KMAILTESTS_TESTS_EXPORT TooManyRecipientsWarning : public KMessageWidget
{
    Q_OBJECT
public:
    explicit TooManyRecipientsWarning(QWidget *parent = nullptr);
    ~TooManyRecipientsWarning() override;
};
