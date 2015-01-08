/*
 *   kmail: KDE mail client
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright (C) 2001-2003 Marc Mutz, mutz@kde.org
 *   Contains code segments and ideas from earlier kmail dialog code.
 *   Copyright (C) 2010 Volker Krause <vkrause@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef NEWIDENTITYDIALOG_H
#define NEWIDENTITYDIALOG_H

#include <QDialog>

class KComboBox;
class KLineEdit;
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
    enum DuplicateMode { Empty, ControlCenter, ExistingEntry };

    explicit NewIdentityDialog(KIdentityManagement::IdentityManager *manager, QWidget *parent = Q_NULLPTR);

    QString identityName() const;
    QString duplicateIdentity() const;
    DuplicateMode duplicateMode() const;

protected Q_SLOTS:
    void slotEnableOK(const QString &);

private Q_SLOTS:
    void slotHelp();
private:
    KLineEdit  *mLineEdit;
    KComboBox  *mComboBox;
    QButtonGroup *mButtonGroup;
    KIdentityManagement::IdentityManager *mIdentityManager;
    QPushButton *mOkButton;
};

}

#endif
