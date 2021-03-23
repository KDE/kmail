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

namespace KIdentityManagement
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
    IdentityListViewItem(IdentityListView *parent, const KIdentityManagement::Identity &ident);
    IdentityListViewItem(IdentityListView *parent, QTreeWidgetItem *after, const KIdentityManagement::Identity &ident);

    Q_REQUIRED_RESULT uint uoid() const;

    KIdentityManagement::Identity &identity() const;
    virtual void setIdentity(const KIdentityManagement::Identity &ident);
    void redisplay();

private:
    void init(const KIdentityManagement::Identity &ident);
    uint mUOID = 0;
};

/** @short A QTreeWidget for KIdentityManagement::Identity
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
    KIdentityManagement::IdentityManager *identityManager() const;
    void setIdentityManager(KIdentityManagement::IdentityManager *im);

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
    KIdentityManagement::IdentityManager *mIdentityManager = nullptr;
};
} // namespace KMail

