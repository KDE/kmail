/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include "cryptostateindicatorwidget.h"
#include "messagecore/settings/globalsettings.h"

#include <KColorScheme>
#include <KLocalizedString>

#include <QHBoxLayout>
#include <QLabel>

CryptoStateIndicatorWidget::CryptoStateIndicatorWidget(QWidget *parent)
    : QWidget(parent),
      mShowAlwaysIndicator(true),
      mIsSign(false),
      mIsEncrypted(false)
{
    QHBoxLayout *hbox = new QHBoxLayout;
    setLayout(hbox);
    hbox->setMargin(0);
    mSignatureStateIndicator = new QLabel(this);
    mSignatureStateIndicator->setAlignment(Qt::AlignHCenter);
    hbox->addWidget(mSignatureStateIndicator);
    mSignatureStateIndicator->setObjectName(QStringLiteral("signatureindicator"));

    // Get the colors for the label
    QPalette p(mSignatureStateIndicator->palette());
    KColorScheme scheme(QPalette::Active, KColorScheme::View);
    const QColor defaultSignedColor =  // pgp signed
        scheme.background(KColorScheme::PositiveBackground).color();
    const QColor defaultEncryptedColor(0x00, 0x80, 0xFF);   // light blue // pgp encrypted

    QColor signedColor = defaultSignedColor;
    QColor encryptedColor = defaultEncryptedColor;
    if (!MessageCore::GlobalSettings::self()->useDefaultColors()) {
        signedColor = MessageCore::GlobalSettings::self()->pgpSignedMessageColor();
        encryptedColor = MessageCore::GlobalSettings::self()->pgpEncryptedMessageColor();
    }

    p.setColor(QPalette::Window, signedColor);
    mSignatureStateIndicator->setPalette(p);
    mSignatureStateIndicator->setAutoFillBackground(true);

    mEncryptionStateIndicator = new QLabel(this);
    mEncryptionStateIndicator->setAlignment(Qt::AlignHCenter);
    hbox->addWidget(mEncryptionStateIndicator);
    p.setColor(QPalette::Window, encryptedColor);
    mEncryptionStateIndicator->setPalette(p);
    mEncryptionStateIndicator->setAutoFillBackground(true);
    mEncryptionStateIndicator->setObjectName(QStringLiteral("encryptionindicator"));
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
    } else {
        mSignatureStateIndicator->setVisible(false);
        mEncryptionStateIndicator->setVisible(false);
    }
}

void CryptoStateIndicatorWidget::updateSignatureAndEncrypionStateIndicators(bool isSign, bool isEncrypted)
{
    mIsEncrypted = isEncrypted;
    mIsSign = isSign;

    mSignatureStateIndicator->setText(isSign ?
                                      i18n("Message will be signed") :
                                      i18n("Message will not be signed"));
    mEncryptionStateIndicator->setText(isEncrypted ?
                                       i18n("Message will be encrypted") :
                                       i18n("Message will not be encrypted"));
    updateShowAlwaysIndicator();

}
