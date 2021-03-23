/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KMessageWidget>

namespace KMail
{
class SearchPatternWarning : public KMessageWidget
{
    Q_OBJECT
public:
    explicit SearchPatternWarning(QWidget *parent = nullptr);
    ~SearchPatternWarning() override;

    void showWarningPattern(const QStringList &lstError);
    void hideWarningPattern();

private:
    void setError(const QStringList &lstError);
};
}

