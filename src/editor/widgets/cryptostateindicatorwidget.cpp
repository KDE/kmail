/*
   Copyright (C) 2014-2019 Montel Laurent <montel@kde.org>

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

#include "cryptostateindicatorwidget.h"
#include "MessageCore/MessageCoreUtil"

#include <KColorScheme>
#include <KLocalizedString>

#include <QHBoxLayout>
#include <QLabel>

CryptoStateIndicatorWidget::CryptoStateIndicatorWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    mSignatureStateIndicator = new QLabel(this);
    mSignatureStateIndicator->setAlignment(Qt::AlignHCenter);
    mSignatureStateIndicator->setTextFormat(Qt::PlainText);
    hbox->addWidget(mSignatureStateIndicator);
    mSignatureStateIndicator->setObjectName(QStringLiteral("signatureindicator"));
    QPalette p(mSignatureStateIndicator->palette());
    p.setColor(QPalette::Window, MessageCore::ColorUtil::self()->pgpSignedTrustedMessageColor());
    p.setColor(QPalette::Text, MessageCore::ColorUtil::self()->pgpSignedTrustedTextColor());
    mSignatureStateIndicator->setPalette(p);
    mSignatureStateIndicator->setAutoFillBackground(true);

    mEncryptionStateIndicator = new QLabel(this);
    mEncryptionStateIndicator->setAlignment(Qt::AlignHCenter);
    mEncryptionStateIndicator->setTextFormat(Qt::PlainText);
    hbox->addWidget(mEncryptionStateIndicator);
    p = mEncryptionStateIndicator->palette();
    p.setColor(QPalette::Window, MessageCore::ColorUtil::self()->pgpEncryptedMessageColor());
    p.setColor(QPalette::Text, MessageCore::ColorUtil::self()->pgpEncryptedTextColor());
    mEncryptionStateIndicator->setPalette(p);
    mEncryptionStateIndicator->setAutoFillBackground(true);
    mEncryptionStateIndicator->setObjectName(QStringLiteral("encryptionindicator"));
    hide();
}

CryptoStateIndicatorWidget::~CryptoStateIndicatorWidget()
{
}

void CryptoStateIndicatorWidget::setShowAlwaysIndicator(bool status)
{
    if (mShowAlwaysIndicator != status) {
        mShowAlwaysIndicator = status;
        updateShowAlwaysIndicator();
    }
}

void CryptoStateIndicatorWidget::updateShowAlwaysIndicator()
{
    if (mShowAlwaysIndicator) {
        mSignatureStateIndicator->setVisible(mIsSign);
        mEncryptionStateIndicator->setVisible(mIsEncrypted);
        if (mIsSign || mIsEncrypted) {
            show();
        } else {
            hide();
        }
    } else {
        mSignatureStateIndicator->setVisible(false);
        mEncryptionStateIndicator->setVisible(false);
        hide();
    }
}

void CryptoStateIndicatorWidget::updateSignatureAndEncrypionStateIndicators(bool isSign, bool isEncrypted)
{
    mIsEncrypted = isEncrypted;
    mIsSign = isSign;

    mSignatureStateIndicator->setText(isSign
                                      ? i18n("Message will be signed")
                                      : i18n("Message will not be signed"));
    mEncryptionStateIndicator->setText(isEncrypted
                                       ? i18n("Message will be encrypted")
                                       : i18n("Message will not be encrypted"));
    updateShowAlwaysIndicator();
}
