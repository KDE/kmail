/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef CONFIGUREAGENTLISTDELEGATE_H
#define CONFIGUREAGENTLISTDELEGATE_H

#include <KWidgetItemDelegate>
#include <QAbstractItemView>

class ConfigureAgentListDelegate : public KWidgetItemDelegate
{
    Q_OBJECT
public:
    explicit ConfigureAgentListDelegate(QAbstractItemView* itemView, QObject* parent = 0);
    virtual ~ConfigureAgentListDelegate();

    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;

    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option,
                       const QModelIndex& index) const;

    virtual QList<QWidget*> createItemWidgets() const;

    virtual void updateItemWidgets(const QList<QWidget*> widgets,
                                   const QStyleOptionViewItem& option,
                                   const QPersistentModelIndex& index) const;
private Q_SLOTS:
    void slotCheckboxClicked(bool checked);

    void slotConfigure();
Q_SIGNALS:
    void requestConfiguration(const QModelIndex& index);
    void requestChangeAgentState(const QModelIndex& index, bool checked);
};

#endif // CONFIGUREAGENTLISTDELEGATE_H
