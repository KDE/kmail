/*
 * Copyright (c) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef INCOMPLETEINDEXDIALOG_H
#define INCOMPLETEINDEXDIALOG_H

#include <QDialog>
#include <QVector>

class QProgressDialog;
class QDBusInterface;

class Ui_IncompleteIndexDialog;
class IncompleteIndexDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IncompleteIndexDialog(const QVector<qint64> &unindexedCollections, QWidget *parent = Q_NULLPTR);
    ~IncompleteIndexDialog();

private Q_SLOTS:
    void selectAll();
    void unselectAll();

    void slotCurrentlyIndexingCollectionChanged(qlonglong colId);
    void slotStopIndexing();

private:
    QList<qlonglong> collectionsToReindex() const;
    void waitForIndexer();
    void updateAllSelection(bool select);

    QScopedPointer<Ui_IncompleteIndexDialog> mUi;
    QProgressDialog *mProgressDialog;
    QDBusInterface *mIndexer;
    QList<qlonglong> mIndexingQueue;
};

#endif
