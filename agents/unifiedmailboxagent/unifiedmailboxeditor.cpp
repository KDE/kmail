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
#include "unifiedmailbox.h"
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
#include <KConfigGroup>

#include <MailCommon/FolderTreeView>
#include <MailCommon/FolderTreeWidget>

namespace {

static constexpr const char *EditorGroup = "UnifiedMailboxEditorDialog";

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
        return !UnifiedMailboxManager::isUnifiedMailbox(col);
    }
};

}

UnifiedMailboxEditor::UnifiedMailboxEditor(const KSharedConfigPtr &config, QWidget* parent)
    : UnifiedMailboxEditor({}, config, parent)
{
}

UnifiedMailboxEditor::UnifiedMailboxEditor(UnifiedMailbox *mailbox, const KSharedConfigPtr &config, QWidget *parent)
    : QDialog(parent)
    , mMailbox(mailbox)
    , mConfig(config)
{
    auto l = new QVBoxLayout(this);

    auto f = new QFormLayout;
    l->addLayout(f);
    auto nameEdit = new QLineEdit(mMailbox->name());
    nameEdit->setClearButtonEnabled(true);
    f->addRow(i18n("Name:"), nameEdit);
    connect(nameEdit, &QLineEdit::textChanged,
            this, [this](const QString &name) {
                mMailbox->setName(name.trimmed());
            });

    auto iconButton = new QPushButton(QIcon::fromTheme(mMailbox->icon(), QIcon::fromTheme(QStringLiteral("folder-mail"))),
                                                           i18n("Pick icon..."));
    f->addRow(i18n("Icon:"), iconButton);
    connect(iconButton, &QPushButton::clicked,
            this, [iconButton, this]() {
                const auto iconName = KIconDialog::getIcon();
                if (!iconName.isEmpty()) {
                    mMailbox->setIcon(iconName);
                    iconButton->setIcon(QIcon::fromTheme(iconName));
                }
            });
    mMailbox->setIcon(iconButton->icon().name());

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
    const auto sources = mMailbox->sourceCollections();
    for (const auto source : sources) {
        const auto index = Akonadi::EntityTreeModel::modelIndexForCollection(selectionModel->model(), Akonadi::Collection(source));
        selectionModel->select(index, QItemSelectionModel::Select);
    }
    connect(checkable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &selected, const QItemSelection &deselected) {
                auto indexes = selected.indexes();
                for (const auto &index : indexes) {
                    mMailbox->addSourceCollection(index.data(Akonadi::EntityTreeModel::CollectionIdRole).toLongLong());
                }
                indexes = deselected.indexes();
                for (const auto &index : indexes) {
                    mMailbox->removeSourceCollection(index.data(Akonadi::EntityTreeModel::CollectionIdRole).toLongLong());
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
                box->button(QDialogButtonBox::Ok)->setEnabled(!name.trimmed().isEmpty());
            });
    box->button(QDialogButtonBox::Ok)->setEnabled(!nameEdit->text().trimmed().isEmpty());
    l->addWidget(box);

    const auto editorGroup = config->group(EditorGroup);
    if (editorGroup.hasKey("geometry")) {
        restoreGeometry(editorGroup.readEntry("geometry", QByteArray()));
    } else {
        resize(500, 900);
    }
}

UnifiedMailboxEditor::~UnifiedMailboxEditor()
{
    auto editorGrp = mConfig->group(EditorGroup);
    editorGrp.writeEntry("geometry", saveGeometry());
}


#include "unifiedmailboxeditor.moc"
