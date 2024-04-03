/*
   SPDX-FileCopyrightText: 2014-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "cryptostateindicatorwidget.h"
using namespace Qt::Literals::StringLiterals;

#include <MessageCore/ColorUtil>

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QLabel>

CryptoStateIndicatorWidget::CryptoStateIndicatorWidget(QWidget *parent)
    : QWidget(parent)
    , mSignatureStateIndicator(new QLabel(this))
    , mEncryptionStateIndicator(new QLabel(this))
{
    auto hbox = new QHBoxLayout(this);
    hbox->setContentsMargins({});
    mSignatureStateIndicator->setAlignment(Qt::AlignHCenter);
    mSignatureStateIndicator->setTextFormat(Qt::PlainText);
    mSignatureStateIndicator->setContentsMargins(4, 4, 4, 4);
    hbox->addWidget(mSignatureStateIndicator);
    mSignatureStateIndicator->setObjectName("signatureindicator"_L1);
    QPalette p(mSignatureStateIndicator->palette());
    p.setColor(QPalette::Window, MessageCore::ColorUtil::self()->pgpSignedTrustedMessageColor());
    p.setColor(QPalette::Text, MessageCore::ColorUtil::self()->pgpSignedTrustedTextColor());
    mSignatureStateIndicator->setPalette(p);
    mSignatureStateIndicator->setAutoFillBackground(true);

    mEncryptionStateIndicator->setAlignment(Qt::AlignHCenter);
    mEncryptionStateIndicator->setTextFormat(Qt::PlainText);
    mEncryptionStateIndicator->setContentsMargins(4, 4, 4, 4);
    hbox->addWidget(mEncryptionStateIndicator);
    p = mEncryptionStateIndicator->palette();
    p.setColor(QPalette::Window, MessageCore::ColorUtil::self()->pgpEncryptedMessageColor());
    p.setColor(QPalette::Text, MessageCore::ColorUtil::self()->pgpEncryptedTextColor());
    mEncryptionStateIndicator->setPalette(p);
    mEncryptionStateIndicator->setAutoFillBackground(true);
    mEncryptionStateIndicator->setObjectName("encryptionindicator"_L1);
    hide();
}

CryptoStateIndicatorWidget::~CryptoStateIndicatorWidget() = default;

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

    mSignatureStateIndicator->setText(isSign ? i18n("Message will be signed") : i18n("Message will not be signed"));
    mEncryptionStateIndicator->setText(isEncrypted ? i18n("Message will be encrypted") : i18n("Message will not be encrypted"));
    updateShowAlwaysIndicator();
}

#include "moc_cryptostateindicatorwidget.cpp"
