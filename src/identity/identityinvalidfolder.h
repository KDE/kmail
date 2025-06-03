/*
  SPDX-FileCopyrightText: 2016-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KMessageWidget>
namespace KMail
{
class IdentityInvalidFolder : public KMessageWidget
{
    Q_OBJECT
public:
    explicit IdentityInvalidFolder(QWidget *parent = nullptr);
    ~IdentityInvalidFolder() override;

    void setErrorMessage(const QString &msg);
};
}
