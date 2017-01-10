/*
   Copyright (C) 2014-2016 Montel Laurent <montel@kde.org>

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

#ifndef CRYPTOSTATEINDICATORWIDGET_H
#define CRYPTOSTATEINDICATORWIDGET_H

#include <QWidget>
class QLabel;

class CryptoStateIndicatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CryptoStateIndicatorWidget(QWidget *parent = nullptr);
    ~CryptoStateIndicatorWidget();

    void updateSignatureAndEncrypionStateIndicators(bool isSign, bool isEncrypted);

    void setShowAlwaysIndicator(bool status);

private:
    void updateShowAlwaysIndicator();
    QLabel *mSignatureStateIndicator;
    QLabel *mEncryptionStateIndicator;
    bool mShowAlwaysIndicator;
    bool mIsSign;
    bool mIsEncrypted;
};

#endif // CRYPTOSTATEINDICATORWIDGET_H
