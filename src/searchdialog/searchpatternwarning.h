/*
   SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SEARCHPATTERNWARNING_H
#define SEARCHPATTERNWARNING_H

#include <KMessageWidget>

namespace KMail {
class SearchPatternWarning : public KMessageWidget
{
    Q_OBJECT
public:
    explicit SearchPatternWarning(QWidget *parent = nullptr);
    ~SearchPatternWarning();

    void showWarningPattern(const QStringList &lstError);
    void hideWarningPattern();

private:
    void setError(const QStringList &lstError);
};
}

#endif // SEARCHPATTERNWARNING_H
