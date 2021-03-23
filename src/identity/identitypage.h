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
namespace KIdentityManagement
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

    Q_REQUIRED_RESULT QString helpAnchor() const;

    void load();
    void save() override;

private:
    void slotNewIdentity();
    void slotModifyIdentity();
    void slotRemoveIdentity();
    /** Connected to @p mRenameButton's clicked() signal. Just does a
      QTreeWidget::editItem on the selected item */
    void slotRenameIdentity();
    /** connected to @p mIdentityList's renamed() signal. Validates the
      new name and sets it in the KIdentityManagement::IdentityManager */
    void slotRenameIdentityFromItem(KMail::IdentityListViewItem *, const QString &);
    void slotContextMenu(KMail::IdentityListViewItem *, const QPoint &);
    void slotSetAsDefault();
    void slotIdentitySelectionChanged();

    void refreshList();
    void updateButtons();

private: // data members
    Ui_IdentityPage mIPage;
    KMail::IdentityDialog *mIdentityDialog = nullptr;
    int mOldNumberOfIdentities = 0;
    KIdentityManagement::IdentityManager *mIdentityManager = nullptr;
};
}

