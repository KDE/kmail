/*
 *   kmail: KDE mail client
 *   SPDX-FileCopyrightText: 2016-2025 Laurent Montel <montel@kde.org>
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QTreeWidget>

class ListView : public QTreeWidget
{
    Q_OBJECT
public:
    explicit ListView(QWidget *parent = nullptr);

Q_SIGNALS:
    void addHeader();
    void removeHeader();

protected:
    void resizeEvent(QResizeEvent *e) override;
    void showEvent(QShowEvent *e) override;

private:
    void slotContextMenu(QPoint pos);
    void resizeColums();
};
