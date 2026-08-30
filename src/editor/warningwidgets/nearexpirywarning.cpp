/*
   SPDX-FileCopyrightText: 2022 Sandro Knauß <knauss@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "nearexpirywarning.h"

#include <QDebug>
using namespace Qt::Literals::StringLiterals;

NearExpiryWarning::NearExpiryWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setPosition(KMessageWidget::Header);
    setVisible(false);
    setCloseButtonVisible(true);
    setWordWrap(true);
    clearInfo();
}

NearExpiryWarning::~NearExpiryWarning() = default;

void NearExpiryWarning::addInfo(const QString &msg)
{
    setText(text() + (text().isEmpty() ? QString() : u"\n"_s) + u"<p>"_s + msg + u"</p>"_s);
}

void NearExpiryWarning::setWarning(bool warning)
{
    if (warning) {
        setMessageType(Warning);
    } else {
        setMessageType(Information);
    }
}

void NearExpiryWarning::clearInfo()
{
    setWarning(false);
    setText(QString());
}

#include "moc_nearexpirywarning.cpp"
