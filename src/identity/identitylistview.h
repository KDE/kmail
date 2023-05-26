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
class IdentityListView;

/** @short A QWidgetTreeItem for use in IdentityListView
 *  @author Marc Mutz <mutz@kde.org>
 **/
class IdentityListViewItem : public QTreeWidgetItem
{
public:
    IdentityListViewItem(IdentityListView *parent, const KIdentityManagementCore::Identity &ident);
    IdentityListViewItem(IdentityListView *parent, QTreeWidgetItem *after, const KIdentityManagementCore::Identity &ident);

    Q_REQUIRED_RESULT uint uoid() const;

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
class IdentityListView : public QTreeWidget
{
    Q_OBJECT
public:
    explicit IdentityListView(QWidget *parent = nullptr);
    ~IdentityListView() override = default;

public:
    void editItem(QTreeWidgetItem *item, int column = 0);
    KIdentityManagementCore::IdentityManager *identityManager() const;
    void setIdentityManager(KIdentityManagementCore::IdentityManager *im);

protected Q_SLOTS:
    void commitData(QWidget *editor) override;

Q_SIGNALS:
    void contextMenu(KMail::IdentityListViewItem *, const QPoint &);
    void rename(KMail::IdentityListViewItem *, const QString &);

protected:
#ifndef QT_NO_DRAGANDDROP
    void startDrag(Qt::DropActions supportedActions) override;
#endif

private:
    void slotCustomContextMenuRequested(const QPoint &);
    KIdentityManagementCore::IdentityManager *mIdentityManager = nullptr;
};
} // namespace KMail
