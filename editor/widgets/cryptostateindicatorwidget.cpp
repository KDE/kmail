/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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
#include "messagecore/messagecoreutil.h"

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
    // Get the colors for the label
    KColorScheme scheme(QPalette::Active, KColorScheme::View);

    QHBoxLayout *hbox = new QHBoxLayout;
    setLayout(hbox);
    hbox->setMargin(0);
    mSignatureStateIndicator = new QLabel(this);
    mSignatureStateIndicator->setAlignment(Qt::AlignHCenter);
    hbox->addWidget(mSignatureStateIndicator);
    mSignatureStateIndicator->setObjectName(QStringLiteral("signatureindicator"));
    QPalette p(mSignatureStateIndicator->palette());
    p.setColor(QPalette::Window, MessageCore::Util::pgpSignedTrustedMessageColor());
    p.setColor(QPalette::Text, MessageCore::Util::pgpSignedTrustedTextColor());
    mSignatureStateIndicator->setPalette(p);
    mSignatureStateIndicator->setAutoFillBackground(true);

    mEncryptionStateIndicator = new QLabel(this);
    mEncryptionStateIndicator->setAlignment(Qt::AlignHCenter);
    hbox->addWidget(mEncryptionStateIndicator);
    p = mEncryptionStateIndicator->palette();
    p.setColor(QPalette::Window, MessageCore::Util::pgpEncryptedMessageColor());
    p.setColor(QPalette::Text, MessageCore::Util::pgpEncryptedTextColor());
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
