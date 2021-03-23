/*
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
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

    Q_REQUIRED_RESULT Akonadi::Item selectedContact() const;

private:
    void slotSelectionChanged();

    void slotDoubleClicked();
    void readConfig();
    void writeConfig();
    Akonadi::EmailAddressSelectionWidget *mEmailSelectionWidget = nullptr;
    QPushButton *mOkButton = nullptr;
};

