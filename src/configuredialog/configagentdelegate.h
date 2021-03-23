/*
    SPDX-FileCopyrightText: 2010 Casey Link <unnamedrambler@gmail.com>
    SPDX-FileCopyrightText: 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

Q_SIGNALS:
    void optionsClicked(const QString &, const QPoint &);

private:
    void drawFocus(QPainter *, const QStyleOptionViewItem &, const QRect &) const;
    QTextDocument *document(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QStyleOptionButton buttonOption(const QStyleOptionViewItem &option) const;
};

