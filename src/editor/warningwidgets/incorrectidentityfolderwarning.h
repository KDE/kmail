/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <KMessageWidget>

class KMAILTESTS_TESTS_EXPORT IncorrectIdentityFolderWarning : public KMessageWidget
{
    Q_OBJECT
public:
    explicit IncorrectIdentityFolderWarning(QWidget *parent = nullptr);
    ~IncorrectIdentityFolderWarning() override;

    void mailTransportIsInvalid();
    void fccIsInvalid();
    void identityInvalid();
    void dictionaryInvalid();
    void clearFccInvalid();

private:
    void addNewLine(QString &str);
    void updateText();
    void slotHideAnnimationFinished();
    bool mMailTransportIsInvalid = false;
    bool mFccIsInvalid = false;
    bool mIdentityIsInvalid = false;
    bool mDictionaryIsInvalid = false;
};

