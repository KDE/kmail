/*  -*- c++ -*-
    identitylistview.h

    This file is part of KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2002 Marc Mutz <mutz@kde.org>
    SPDX-FileCopyrightText: 2007 Mathias Soeken <msoeken@tzi.de>

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace KIdentityManagementCore
{
class Identity;
class IdentityManager;
}

namespace KMail
{
class IdentityTreeWidget;

/** @short A QWidgetTreeItem for use in IdentityListView
 *  @author Marc Mutz <mutz@kde.org>
 **/
class IdentityTreeWidgetItem : public QTreeWidgetItem
{
public:
    IdentityTreeWidgetItem(IdentityTreeWidget *parent, const KIdentityManagementCore::Identity &ident);
    IdentityTreeWidgetItem(IdentityTreeWidget *parent, QTreeWidgetItem *after, const KIdentityManagementCore::Identity &ident);

    [[nodiscard]] uint uoid() const;

    KIdentityManagementCore::Identity &identity() const;
    virtual void setIdentity(const KIdentityManagementCore::Identity &ident);
    void redisplay();

private:
    void init(const KIdentityManagementCore::Identity &ident);
    uint mUOID = 0;
};

/** @short A QTreeWidget for KIdentityManagementCore::Identity
 * @author Marc Mutz <mutz@kde.org>
 **/
class IdentityTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit IdentityTreeWidget(QWidget *parent = nullptr);
    ~IdentityTreeWidget() override = default;

public:
    void editItem(QTreeWidgetItem *item, int column = 0);
    KIdentityManagementCore::IdentityManager *identityManager() const;
    void setIdentityManager(KIdentityManagementCore::IdentityManager *im);

protected Q_SLOTS:
    void commitData(QWidget *editor) override;

Q_SIGNALS:
    void contextMenu(KMail::IdentityTreeWidgetItem *, const QPoint &);
    void rename(KMail::IdentityTreeWidgetItem *, const QString &);

protected:
#ifndef QT_NO_DRAGANDDROP
    void startDrag(Qt::DropActions supportedActions) override;
#endif

private:
    void slotCustomContextMenuRequested(const QPoint &);
    KIdentityManagementCore::IdentityManager *mIdentityManager = nullptr;
};
} // namespace KMail
