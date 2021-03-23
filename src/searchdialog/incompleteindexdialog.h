/*
 * SPDX-FileCopyrightText: 2016 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDialog>
#include <QList>
#include <memory>
class QProgressDialog;
class QDBusInterface;

class Ui_IncompleteIndexDialog;
class IncompleteIndexDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IncompleteIndexDialog(const QVector<qint64> &unindexedCollections, QWidget *parent = nullptr);
    ~IncompleteIndexDialog() override;

private Q_SLOTS:

    void slotCurrentlyIndexingCollectionChanged(qlonglong colId);

private:
    void selectAll();
    void unselectAll();
    void slotStopIndexing();
    void readConfig();
    void writeConfig();
    Q_REQUIRED_RESULT QList<qlonglong> collectionsToReindex() const;
    void waitForIndexer();
    void updateAllSelection(bool select);

    std::unique_ptr<Ui_IncompleteIndexDialog> mUi;
    QProgressDialog *mProgressDialog = nullptr;
    QDBusInterface *mIndexer = nullptr;
    QList<qlonglong> mIndexingQueue;
};

