/*
   Copyright (C) 2013-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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

SearchPatternWarning::~SearchPatternWarning()
{
}

void SearchPatternWarning::setError(const QStringList &lstError)
{
    setText(i18n("Search failed some errors were found: <ul><li>%1</li></ul>", lstError.join(QStringLiteral("</li><li>"))));
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

