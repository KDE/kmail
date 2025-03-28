/*
   SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "searchpatternwarning.h"

#include <KLocalizedString>

using namespace KMail;
using namespace Qt::Literals::StringLiterals;

SearchPatternWarning::SearchPatternWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setPosition(KMessageWidget::Header);
    setVisible(false);
    setCloseButtonVisible(true);
    setMessageType(Information);
    setWordWrap(true);
}

SearchPatternWarning::~SearchPatternWarning() = default;

void SearchPatternWarning::setError(const QStringList &lstError)
{
    setText(i18n("Search failed some errors were found: <ul><li>%1</li></ul>", lstError.join("</li><li>"_L1)));
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

#include "moc_searchpatternwarning.cpp"
