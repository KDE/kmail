/*
    Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
    Copyright (c) 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    Copyright (c) 2006-2008 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef CONFIGAGENTDELEGATE_H
#define CONFIGAGENTDELEGATE_H

#include <QStyledItemDelegate>

class QTextDocument;

/**
 * @short A delegate for listing the accounts in the account list with kmail specific options.
 * Based off Akonadi::Internal::AgentInstanceWidgetDelegate
 * by Tobias Koenig.
 *
 */
class ConfigAgentDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ConfigAgentDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    QWidget  *createEditor(QWidget *parent, const QStyleOptionViewItem   &option, const QModelIndex &index) const override;

Q_SIGNALS:
    void optionsClicked(const QString &, const QPoint &);

private:
    void drawFocus(QPainter *, const QStyleOptionViewItem &, const QRect &) const;
    QTextDocument *document(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QStyleOptionButton buttonOption(const QStyleOptionViewItem &option) const;
};

#endif // CONFIGAGENTDELEGATE_H
