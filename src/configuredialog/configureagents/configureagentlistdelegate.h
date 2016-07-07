/*
   Copyright (C) 2015-2016 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef CONFIGUREAGENTLISTDELEGATE_H
#define CONFIGUREAGENTLISTDELEGATE_H

#include <KWidgetItemDelegate>
#include <QAbstractItemView>

class ConfigureAgentListDelegate : public KWidgetItemDelegate
{
    Q_OBJECT
public:
    explicit ConfigureAgentListDelegate(QAbstractItemView *itemView, QObject *parent = Q_NULLPTR);
    virtual ~ConfigureAgentListDelegate();

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const Q_DECL_OVERRIDE;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const Q_DECL_OVERRIDE;

    QList<QWidget *> createItemWidgets(const QModelIndex &) const Q_DECL_OVERRIDE;
    void updateItemWidgets(const QList<QWidget *> widgets,
                           const QStyleOptionViewItem &option,
                           const QPersistentModelIndex &index) const Q_DECL_OVERRIDE;
Q_SIGNALS:
    void requestConfiguration(const QModelIndex &index);
private:
    void slotCheckboxClicked(bool checked);

    void slotConfigure();
};

#endif // CONFIGUREAGENTLISTDELEGATE_H
