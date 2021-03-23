/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <QWidget>
class QLabel;

class KMAILTESTS_TESTS_EXPORT CryptoStateIndicatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CryptoStateIndicatorWidget(QWidget *parent = nullptr);
    ~CryptoStateIndicatorWidget() override;

    void updateSignatureAndEncrypionStateIndicators(bool isSign, bool isEncrypted);

    void setShowAlwaysIndicator(bool status);

private:
    void updateShowAlwaysIndicator();
    QLabel *const mSignatureStateIndicator;
    QLabel *const mEncryptionStateIndicator;
    bool mShowAlwaysIndicator = true;
    bool mIsSign = false;
    bool mIsEncrypted = false;
};

