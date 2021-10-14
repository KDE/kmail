/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "toomanyrecipientswarning.h"

TooManyRecipientsWarning::TooManyRecipientsWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setVisible(false);
    setCloseButtonVisible(true);
    setMessageType(Warning);
    setWordWrap(true);
}

TooManyRecipientsWarning::~TooManyRecipientsWarning()
{
}
