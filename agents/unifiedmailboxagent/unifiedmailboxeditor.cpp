/*
   Copyright (C) 2018 Daniel Vrátil <dvratil@kde.org>

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

#include "unifiedmailboxeditor.h"
#include "mailkernel.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSortFilterProxyModel>
#include <QLabel>

#include <KIconDialog>
#include <KLocalizedString>
#include <KCheckableProxyModel>

#include <MailCommon/FolderTreeView>
#include <MailCommon/FolderTreeWidget>

namespace {

class SelfFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SelfFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {}

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::CheckStateRole) {
            // Make top-level collections uncheckable
            const Akonadi::Collection col = data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
            if (col.parentCollection() == Akonadi::Collection::root()) {
                return {};
            }
        }

        return QSortFilterProxyModel::data(index, role);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        // Make top-level collections uncheckable
        const Akonadi::Collection col = data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        if (col.parentCollection() == Akonadi::Collection::root()) {
            return QSortFilterProxyModel::flags(index) & ~Qt::ItemIsUserCheckable;
        } else {
            return QSortFilterProxyModel::flags(index);
        }
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        // Hide ourselves
        const auto sourceIndex = sourceModel()->index(source_row, 0, source_parent);
        const Akonadi::Collection col = sourceModel()->data(sourceIndex, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        return col.resource() != QLatin1String("akonadi_unifiedmailbox_agent");
    }
};

}

UnifiedMailboxEditor::UnifiedMailboxEditor(QWidget* parent)
    : UnifiedMailboxEditor({}, parent)
{
}

UnifiedMailboxEditor::UnifiedMailboxEditor(UnifiedMailbox mailbox, QWidget *parent)
    : QDialog(parent)
    , mBox(std::move(mailbox))
{
    resize(500, 900);

    auto l = new QVBoxLayout;
    setLayout(l);

    auto f = new QFormLayout;
    l->addLayout(f);
    auto nameEdit = new QLineEdit(mBox.name());
    f->addRow(i18n("Name:"), nameEdit);
    connect(nameEdit, &QLineEdit::textChanged,
            this, [this](const QString &name) {
                mBox.setName(name);
            });

    auto iconButton = new QPushButton(QIcon::fromTheme(mBox.icon(), QIcon::fromTheme(QStringLiteral("folder-mail"))),
                                                           i18n("Pick icon..."));
    f->addRow(i18n("Icon:"), iconButton);
    connect(iconButton, &QPushButton::clicked,
            this, [iconButton, this]() {
                const auto iconName = KIconDialog::getIcon();
                if (!iconName.isEmpty()) {
                    mBox.setIcon(iconName);
                    iconButton->setIcon(QIcon::fromTheme(iconName));
                }
            });
    mBox.setIcon(iconButton->icon().name());

    l->addSpacing(10);

    auto ftw = new MailCommon::FolderTreeWidget(nullptr, nullptr,
            MailCommon::FolderTreeWidget::TreeViewOptions(MailCommon::FolderTreeWidget::UseDistinctSelectionModel |
                                                          MailCommon::FolderTreeWidget::HideStatistics));
    l->addWidget(ftw);

    auto ftv = ftw->folderTreeView();
    auto sourceModel = ftv->model();
    auto selectionModel = ftw->selectionModel();

    auto checkable = new KCheckableProxyModel(this);
    checkable->setSourceModel(sourceModel);
    checkable->setSelectionModel(selectionModel);
    const auto sources = mBox.sourceCollections();
    for (const auto source : sources) {
        const auto index = Akonadi::EntityTreeModel::modelIndexForCollection(selectionModel->model(), Akonadi::Collection(source));
        selectionModel->select(index, QItemSelectionModel::Select);
    }
    connect(checkable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &selected, const QItemSelection &deselected) {
                auto indexes = selected.indexes();
                for (const auto &index : indexes) {
                    mBox.addSourceCollection(index.data(Akonadi::EntityTreeModel::CollectionIdRole).toLongLong());
                }
                indexes = deselected.indexes();
                for (const auto &index : indexes) {
                    mBox.removeSourceCollection(index.data(Akonadi::EntityTreeModel::CollectionIdRole).toLongLong());
                }
            });

    auto selfFilter = new SelfFilterProxyModel(this);
    selfFilter->setSourceModel(checkable);

    ftv->setModel(selfFilter);
    ftv->expandAll();

    auto box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(nameEdit, &QLineEdit::textChanged,
            box, [box](const QString &name) {
                box->button(QDialogButtonBox::Ok)->setEnabled(!name.isEmpty());
            });
    box->button(QDialogButtonBox::Ok)->setEnabled(!nameEdit->text().isEmpty());
    l->addWidget(box);
}

UnifiedMailbox UnifiedMailboxEditor::box() const
{
    return mBox;
}

#include "unifiedmailboxeditor.moc"
