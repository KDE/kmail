/*
  SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDialog>
class QPushButton;
namespace Akonadi
{
class EmailAddressSelectionWidget;
class Item;
}
class AddEmailToExistingContactDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddEmailToExistingContactDialog(QWidget *parent);
    ~AddEmailToExistingContactDialog() override;

    [[nodiscard]] Akonadi::Item selectedContact() const;

private:
    void slotSelectionChanged();

    void slotDoubleClicked();
    void readConfig();
    void writeConfig();
    Akonadi::EmailAddressSelectionWidget *mEmailSelectionWidget = nullptr;
    QPushButton *mOkButton = nullptr;
};
