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

#include "incompleteindexdialog.h"
#include "ui_incompleteindexdialog.h"
#include "kmkernel.h"

#include <QProgressDialog>
#include <KDescendantsProxyModel>
#include <KLocalizedString>

#include <AkonadiCore/EntityTreeModel>
#include <AkonadiCore/EntityMimeTypeFilterModel>

#include <PimCommon/PimUtil>
#include <PimCommonAkonadi/MailUtil>

#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusMetaType>
#include <QTimer>
#include <QHBoxLayout>
#include <QDialogButtonBox>

Q_DECLARE_METATYPE(Qt::CheckState)
Q_DECLARE_METATYPE(QVector<qint64>)

class SearchCollectionProxyModel : public QSortFilterProxyModel
{
public:
    explicit SearchCollectionProxyModel(const QVector<qint64> &unindexedCollections, QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        mFilterCollections.reserve(unindexedCollections.size());
        for (qint64 col : unindexedCollections) {
            mFilterCollections.insert(col, true);
        }
    }

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE
    {
        if (role == Qt::CheckStateRole) {
            if (index.isValid() && index.column() == 0) {
                const qint64 colId = collectionIdForIndex(index);
                return mFilterCollections.value(colId) ? Qt::Checked : Qt::Unchecked;
            }
        }

        return QSortFilterProxyModel::data(index, role);
    }

    bool setData(const QModelIndex &index, const QVariant &data, int role) Q_DECL_OVERRIDE {
        if (role == Qt::CheckStateRole)
        {
            if (index.isValid() && index.column() == 0) {
                const qint64 colId = collectionIdForIndex(index);
                mFilterCollections[colId] = data.value<Qt::CheckState>();
                return true;
            }
        }

        return QSortFilterProxyModel::setData(index, data, role);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE
    {
        if (index.isValid() && index.column() == 0) {
            return QSortFilterProxyModel::flags(index) | Qt::ItemIsUserCheckable;
        } else {
            return QSortFilterProxyModel::flags(index);
        }
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const Q_DECL_OVERRIDE
    {
        const QModelIndex source_idx = sourceModel()->index(source_row, 0, source_parent);
        const qint64 colId = sourceModel()->data(source_idx, Akonadi::EntityTreeModel::CollectionIdRole).toLongLong();
        return mFilterCollections.contains(colId);
    }

private:
    qint64 collectionIdForIndex(const QModelIndex &index) const
    {
        return data(index, Akonadi::EntityTreeModel::CollectionIdRole).toLongLong();
    }

private:
    QHash<qint64, bool> mFilterCollections;
};

IncompleteIndexDialog::IncompleteIndexDialog(const QVector<qint64> &unindexedCollections, QWidget *parent)
    : QDialog(parent)
    , mUi(new Ui::IncompleteIndexDialog)
    , mProgressDialog(nullptr)
    , mIndexer(nullptr)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setMargin(0);
    QWidget *w = new QWidget(this);
    mainLayout->addWidget(w);
    qDBusRegisterMetaType<QVector<qint64> >();

    mUi->setupUi(w);

    Akonadi::EntityTreeModel *etm = KMKernel::self()->entityTreeModel();
    Akonadi::EntityMimeTypeFilterModel *mimeProxy = new Akonadi::EntityMimeTypeFilterModel(this);
    mimeProxy->addMimeTypeInclusionFilter(Akonadi::Collection::mimeType());
    mimeProxy->setSourceModel(etm);

    KDescendantsProxyModel *flatProxy = new KDescendantsProxyModel(this);
    flatProxy->setDisplayAncestorData(true);
    flatProxy->setAncestorSeparator(QStringLiteral(" / "));
    flatProxy->setSourceModel(mimeProxy);

    SearchCollectionProxyModel *proxy = new SearchCollectionProxyModel(unindexedCollections, this);
    proxy->setSourceModel(flatProxy);

    mUi->collectionView->setModel(proxy);

    connect(mUi->selectAllBtn, &QPushButton::clicked, this, &IncompleteIndexDialog::selectAll);
    connect(mUi->unselectAllBtn, &QPushButton::clicked, this, &IncompleteIndexDialog::unselectAll);
    mUi->buttonBox->button(QDialogButtonBox::Ok)->setText(i18n("Reindex"));
    mUi->buttonBox->button(QDialogButtonBox::Cancel)->setText(i18n("Search Anyway"));
    connect(mUi->buttonBox, &QDialogButtonBox::accepted, this, &IncompleteIndexDialog::waitForIndexer);
    connect(mUi->buttonBox, &QDialogButtonBox::rejected, this, &IncompleteIndexDialog::reject);
}

IncompleteIndexDialog::~IncompleteIndexDialog()
{
}

void IncompleteIndexDialog::selectAll()
{
    updateAllSelection(true);
}

void IncompleteIndexDialog::unselectAll()
{
    updateAllSelection(false);
}

void IncompleteIndexDialog::updateAllSelection(bool select)
{
    QAbstractItemModel *model = mUi->collectionView->model();
    for (int i = 0, cnt = model->rowCount(); i < cnt; ++i) {
        const QModelIndex idx = model->index(i, 0, QModelIndex());
        model->setData(idx, select ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
    }
}

QList<qlonglong> IncompleteIndexDialog::collectionsToReindex() const
{
    QList<qlonglong> res;

    QAbstractItemModel *model = mUi->collectionView->model();
    for (int i = 0, cnt = model->rowCount(); i < cnt; ++i) {
        const QModelIndex idx = model->index(i, 0, QModelIndex());
        if (model->data(idx, Qt::CheckStateRole).toInt() == Qt::Checked) {
            res.push_back(model->data(idx, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>().id());
        }
    }

    return res;
}

void IncompleteIndexDialog::waitForIndexer()
{
    mIndexer = new QDBusInterface(PimCommon::MailUtil::indexerServiceName(), QLatin1String("/"),
                                  QStringLiteral("org.freedesktop.Akonadi.Indexer"),
                                  QDBusConnection::sessionBus(), this);

    if (!mIndexer->isValid()) {
        accept();
        return;
    }
    mIndexingQueue = collectionsToReindex();
    if (mIndexingQueue.isEmpty()) {
        accept();
        return;
    }

    mProgressDialog = new QProgressDialog(this);
    mProgressDialog->setMaximum(mIndexingQueue.size());
    mProgressDialog->setValue(0);
    mProgressDialog->setLabelText(i18n("Indexing Collections..."));
    connect(mProgressDialog, &QDialog::rejected, this, &IncompleteIndexDialog::slotStopIndexing);

    connect(mIndexer, SIGNAL(currentCollectionChanged(qlonglong)),
            this, SLOT(slotCurrentlyIndexingCollectionChanged(qlonglong)));

    mIndexer->asyncCall(QStringLiteral("reindexCollections"), QVariant::fromValue(mIndexingQueue));
    mProgressDialog->show();
}

void IncompleteIndexDialog::slotStopIndexing()
{
    mProgressDialog->close();
    reject();
}

void IncompleteIndexDialog::slotCurrentlyIndexingCollectionChanged(qlonglong colId)
{
    const int idx = mIndexingQueue.indexOf(colId);
    if (idx > -1) {
        mIndexingQueue.removeAll(idx);
        mProgressDialog->setValue(mProgressDialog->maximum() - mIndexingQueue.size());

        if (mIndexingQueue.isEmpty()) {
            QTimer::singleShot(1000, this, &IncompleteIndexDialog::accept);
        }
    }
}
