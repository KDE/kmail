/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
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

IncorrectIdentityFolderWarning::~IncorrectIdentityFolderWarning() = default;

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

void IncorrectIdentityFolderWarning::clearFccInvalid()
{
    if (mFccIsInvalid) {
        mFccIsInvalid = false;
        updateText();
    }
}

void IncorrectIdentityFolderWarning::addNewLine(QString &str)
{
    if (!str.isEmpty()) {
        str += QLatin1Char('\n');
    }
}

void IncorrectIdentityFolderWarning::updateText()
{
    QString text;
    if (mMailTransportIsInvalid) {
        text = i18n("Transport was not found. Please verify that you will use a correct mail transport.");
    }
    if (mFccIsInvalid) {
        addNewLine(text);
        text += i18n("Sent Folder is not defined. Please set it before sending the mail.");
    }
    if (mIdentityIsInvalid) {
        addNewLine(text);
        text += i18n("Identity was not found. Please verify that you will use a correct identity.");
    }
    if (mDictionaryIsInvalid) {
        addNewLine(text);
        text += i18n("Dictionary was not found. Please verify that you will use a correct dictionary.");
    }
    if (text.isEmpty()) {
        animatedHide();
        setText(QString());
    } else {
        setText(text);
        animatedShow();
    }
}

void IncorrectIdentityFolderWarning::slotHideAnnimationFinished()
{
    mMailTransportIsInvalid = false;
    mFccIsInvalid = false;
    mIdentityIsInvalid = false;
    mDictionaryIsInvalid = false;
}
