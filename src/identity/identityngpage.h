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

#include "kmail_export.h"

#include "configuredialog/configuredialog_p.h"
#include "ui_identityngpage.h"
namespace KIdentityManagementCore
{
class IdentityManager;
}

namespace KMail
{
class IdentityDialog;
class IdentityTreeWidget;
class IdentityTreeWidgetItem;

class KMAIL_EXPORT IdentityNgPage : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit IdentityNgPage(QWidget *parent = nullptr);
    ~IdentityNgPage() override;

    [[nodiscard]] QString helpAnchor() const;

    void load();
    void save() override;

    void setEnablePlasmaActivities(bool enable);

private:
    KMAIL_NO_EXPORT void slotNewIdentity();
    KMAIL_NO_EXPORT void slotModifyIdentity();
    KMAIL_NO_EXPORT void slotRemoveIdentity();
    /** Connected to @p mRenameButton's clicked() signal. Just does a
      QTreeWidget::editItem on the selected item */
    KMAIL_NO_EXPORT void slotRenameIdentity();
    KMAIL_NO_EXPORT void slotContextMenu(const QPoint &);
    KMAIL_NO_EXPORT void slotSetAsDefault();
    KMAIL_NO_EXPORT void slotIdentitySelectionChanged();

    KMAIL_NO_EXPORT void refreshList();
    KMAIL_NO_EXPORT void updateButtons();

private: // data members
    Ui_IdentityNgPage mIPage;
    KMail::IdentityDialog *mIdentityDialog = nullptr;
    int mOldNumberOfIdentities = 0;
    KIdentityManagementCore::IdentityManager *mIdentityManager = nullptr;
};
}
