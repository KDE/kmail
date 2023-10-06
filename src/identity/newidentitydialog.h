/*
 *   kmail: KDE mail client
 *   SPDX-FileCopyrightText: 2000 Espen Sand <espen@kde.org>
 *   SPDX-FileCopyrightText: 2001-2003 Marc Mutz <mutz@kde.org>
 *   Contains code segments and ideas from earlier kmail dialog code.
 *   SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#pragma once

#include <QDialog>

class QComboBox;
class QLineEdit;
class QButtonGroup;

namespace KIdentityManagementCore
{
class IdentityManager;
}

namespace KMail
{
class NewIdentityDialog : public QDialog
{
    Q_OBJECT

public:
    enum DuplicateMode {
        Empty = 0,
        ControlCenter,
        ExistingEntry,
    };
    explicit NewIdentityDialog(KIdentityManagementCore::IdentityManager *manager, QWidget *parent = nullptr);

    [[nodiscard]] QString identityName() const;
    [[nodiscard]] QString duplicateIdentity() const;
    [[nodiscard]] DuplicateMode duplicateMode() const;

private:
    void slotHelp();
    void slotEnableOK(const QString &);
    QLineEdit *mLineEdit = nullptr;
    QComboBox *mComboBox = nullptr;
    QButtonGroup *mButtonGroup = nullptr;
    KIdentityManagementCore::IdentityManager *const mIdentityManager;
    QPushButton *mOkButton = nullptr;
};
}
