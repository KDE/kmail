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
#include "ui_identitypage.h"
namespace KIdentityManagementCore
{
class IdentityManager;
}

namespace KMail
{
class IdentityDialog;
class IdentityListView;
class IdentityListViewItem;

class KMAIL_EXPORT IdentityPage : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit IdentityPage(QWidget *parent = nullptr);
    ~IdentityPage() override;

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
    /** connected to @p mIdentityList's renamed() signal. Validates the
      new name and sets it in the KIdentityManagementCore::IdentityManager */
    KMAIL_NO_EXPORT void slotRenameIdentityFromItem(KMail::IdentityListViewItem *, const QString &);
    KMAIL_NO_EXPORT void slotContextMenu(KMail::IdentityListViewItem *, const QPoint &);
    KMAIL_NO_EXPORT void slotSetAsDefault();
    KMAIL_NO_EXPORT void slotIdentitySelectionChanged();

    KMAIL_NO_EXPORT void refreshList();
    KMAIL_NO_EXPORT void updateButtons();

private: // data members
    Ui_IdentityPage mIPage;
    KMail::IdentityDialog *mIdentityDialog = nullptr;
    int mOldNumberOfIdentities = 0;
    KIdentityManagementCore::IdentityManager *mIdentityManager = nullptr;
};
}
