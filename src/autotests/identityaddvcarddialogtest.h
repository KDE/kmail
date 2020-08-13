/*
  SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef IDENTITYADDVCARDDIALOGTEST_H
#define IDENTITYADDVCARDDIALOGTEST_H

#include <QObject>

class identityaddvcarddialogtest : public QObject
{
    Q_OBJECT
public:
    identityaddvcarddialogtest();

private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldEnabledUrlRequesterWhenSelectFromExistingVCard();
    void shouldEnabledComboboxWhenSelectDuplicateVCard();
    void shouldEnabledComboboxWhenSelectFromExistingVCardAndAfterDuplicateVCard();
};

#endif // IDENTITYADDVCARDDIALOGTEST_H
