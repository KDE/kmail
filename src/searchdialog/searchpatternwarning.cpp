/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "searchpatternwarning.h"

#include <KLocalizedString>

using namespace KMail;
SearchPatternWarning::SearchPatternWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setVisible(false);
    setCloseButtonVisible(true);
    setMessageType(Information);
    setWordWrap(true);
}

SearchPatternWarning::~SearchPatternWarning() = default;

void SearchPatternWarning::setError(const QStringList &lstError)
{
    setText(i18n("Search failed some errors were found: <ul><li>%1</li></ul>", lstError.join(QLatin1String("</li><li>"))));
}

void SearchPatternWarning::showWarningPattern(const QStringList &lstError)
{
    setError(lstError);
    animatedShow();
}

void SearchPatternWarning::hideWarningPattern()
{
    setText(QString());
    animatedHide();
}
