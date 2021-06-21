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

namespace KIdentityManagement
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
    explicit NewIdentityDialog(KIdentityManagement::IdentityManager *manager, QWidget *parent = nullptr);

    Q_REQUIRED_RESULT QString identityName() const;
    Q_REQUIRED_RESULT QString duplicateIdentity() const;
    Q_REQUIRED_RESULT DuplicateMode duplicateMode() const;

private:
    void slotHelp();
    void slotEnableOK(const QString &);
    QLineEdit *mLineEdit = nullptr;
    QComboBox *mComboBox = nullptr;
    QButtonGroup *mButtonGroup = nullptr;
    KIdentityManagement::IdentityManager *const mIdentityManager;
    QPushButton *mOkButton = nullptr;
};
}

