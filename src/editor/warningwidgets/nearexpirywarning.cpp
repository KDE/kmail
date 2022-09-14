/*
   SPDX-FileCopyrightText: 2022 Sandro Knauß <knauss@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "nearexpirywarning.h"

#include <QDebug>

NearExpiryWarning::NearExpiryWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setVisible(false);
    setCloseButtonVisible(true);
    setWordWrap(true);
    clearInfo();
}

NearExpiryWarning::~NearExpiryWarning() = default;

void NearExpiryWarning::addInfo(QString msg)
{
    setText(text() + QStringLiteral("\n<p>") + msg + QStringLiteral("</p>"));
}

void NearExpiryWarning::setWarning(bool warning)
{
    if(warning) {
        setMessageType(Warning);
    } else {
        setMessageType(Information);
    }
}

void NearExpiryWarning::clearInfo()
{
    setWarning(false);
    setText(QStringLiteral());
}
