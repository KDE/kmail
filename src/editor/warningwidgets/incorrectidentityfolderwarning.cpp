/*
   Copyright (C) 2017-2018 Montel Laurent <montel@kde.org>

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

#include "incorrectidentityfolderwarning.h"
#include <KLocalizedString>

IncorrectIdentityFolderWarning::IncorrectIdentityFolderWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setVisible(false);
    setCloseButtonVisible(true);
    setMessageType(Warning);
    setWordWrap(true);
    connect(this, &IncorrectIdentityFolderWarning::hideAnimationFinished, this, &IncorrectIdentityFolderWarning::slotHideAnnimationFinished);
}

IncorrectIdentityFolderWarning::~IncorrectIdentityFolderWarning()
{
}

void IncorrectIdentityFolderWarning::mailTransportIsInvalid()
{
    mMailTransportIsInvalid = true;
    updateText();
}

void IncorrectIdentityFolderWarning::fccIsInvalid()
{
    mFccIsInvalid = true;
    updateText();
}

void IncorrectIdentityFolderWarning::identityInvalid()
{
    mIdentityIsInvalid = true;
    updateText();
}

void IncorrectIdentityFolderWarning::dictionaryInvalid()
{
    mDictionaryIsInvalid = true;
    updateText();
}

void IncorrectIdentityFolderWarning::updateText()
{
    QString text;
    if (mMailTransportIsInvalid) {
        text = i18n("Transport was not found. Please verify that you will use a correct mail transport.");
    }
    if (mFccIsInvalid) {
        if (!text.isEmpty()) {
            text += QLatin1Char('\n');
        }
        text += i18n("Sent Folder is not defined. Please verify it before to send it.");
    }
    if (mIdentityIsInvalid) {
        if (!text.isEmpty()) {
            text += QLatin1Char('\n');
        }
        text += i18n("Identity was not found. Please verify that you will use a correct identity.");
    }
    if (mDictionaryIsInvalid) {
        if (!text.isEmpty()) {
            text += QLatin1Char('\n');
        }
        text += i18n("Dictionary was not found. Please verify that you will use a correct dictionary.");
    }
    setText(text);
    animatedShow();
}

void IncorrectIdentityFolderWarning::slotHideAnnimationFinished()
{
    mMailTransportIsInvalid = false;
    mFccIsInvalid = false;
    mIdentityIsInvalid = false;
    mDictionaryIsInvalid = false;
}
