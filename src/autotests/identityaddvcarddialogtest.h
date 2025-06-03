/*
  SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
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
