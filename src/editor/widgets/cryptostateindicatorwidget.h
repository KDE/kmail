/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CRYPTOSTATEINDICATORWIDGET_H
#define CRYPTOSTATEINDICATORWIDGET_H

#include <QWidget>
#include "kmail_private_export.h"
class QLabel;

class KMAILTESTS_TESTS_EXPORT CryptoStateIndicatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CryptoStateIndicatorWidget(QWidget *parent = nullptr);
    ~CryptoStateIndicatorWidget();

    void updateSignatureAndEncrypionStateIndicators(bool isSign, bool isEncrypted);

    void setShowAlwaysIndicator(bool status);

private:
    void updateShowAlwaysIndicator();
    QLabel *mSignatureStateIndicator = nullptr;
    QLabel *mEncryptionStateIndicator = nullptr;
    bool mShowAlwaysIndicator = true;
    bool mIsSign = false;
    bool mIsEncrypted = false;
};

#endif // CRYPTOSTATEINDICATORWIDGET_H
