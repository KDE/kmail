/*
  SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <QObject>

class identityaddvcarddialogtest : public QObject
{
    Q_OBJECT
public:
    explicit identityaddvcarddialogtest(QObject *parent = nullptr);

private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldEnabledUrlRequesterWhenSelectFromExistingVCard();
    void shouldEnabledComboboxWhenSelectDuplicateVCard();
    void shouldEnabledComboboxWhenSelectFromExistingVCardAndAfterDuplicateVCard();
};

