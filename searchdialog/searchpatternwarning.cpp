/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "searchpatternwarning.h"

#include <KLocale>

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
    setText( i18n( "Nepomuk can not make search. Errors found: <ul><li>%1</li></ul>", lstError.join( QLatin1String( "</li><li>" ) ) ) );
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

#include "searchpatternwarning.moc"
