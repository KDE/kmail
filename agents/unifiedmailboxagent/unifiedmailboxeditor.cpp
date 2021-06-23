/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "unifiedmailboxeditor.h"
#include "mailkernel.h"
#include "unifiedmailbox.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include <KCheckableProxyModel>
#include <KConfigGroup>
#include <KIconDialog>
#include <KLocalizedString>

#include <MailCommon/FolderTreeView>
#include <MailCommon/FolderTreeWidget>

namespace
{
static constexpr const char *EditorGroup = "UnifiedMailboxEditorDialog";

class SelfFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SelfFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::CheckStateRole) {
            // Make top-level collections uncheckable
            const auto col = data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
            if (col.parentCollection() == Akonadi::Collection::root()) {
                return {};
            }
        }

        return QSortFilterProxyModel::data(index, role);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        // Make top-level collections uncheckable
        const auto col = data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
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
        const auto col = sourceModel()->data(sourceIndex, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        return !UnifiedMailboxManager::isUnifiedMailbox(col);
    }
};
}

UnifiedMailboxEditor::UnifiedMailboxEditor(const KSharedConfigPtr &config, QWidget *parent)
    : UnifiedMailboxEditor({}, config, parent)
{
}

UnifiedMailboxEditor::UnifiedMailboxEditor(UnifiedMailbox *mailbox, const KSharedConfigPtr &config, QWidget *parent)
    : QDialog(parent)
    , mMailbox(mailbox)
    , mConfig(config)
{
    setWindowTitle(i18nc("@title:window", "Configure Unified Mailbox"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    auto l = new QVBoxLayout(this);

    auto f = new QFormLayout;
    l->addLayout(f);
    const QString mailBoxName = mMailbox->name();
    auto nameEdit = new QLineEdit(mailBoxName);
    nameEdit->setClearButtonEnabled(true);
    if (!mailBoxName.isEmpty()) {
        nameEdit->setReadOnly(true);
    }
    f->addRow(i18n("Name:"), nameEdit);
    connect(nameEdit, &QLineEdit::textChanged, this, [this](const QString &name) {
        mMailbox->setName(name.trimmed());
    });

    auto iconButton = new QPushButton(QIcon::fromTheme(mMailbox->icon(), QIcon::fromTheme(QStringLiteral("folder-mail"))), i18n("Pick icon..."));
    f->addRow(i18n("Icon:"), iconButton);
    connect(iconButton, &QPushButton::clicked, this, [iconButton, this]() {
        const auto iconName = KIconDialog::getIcon();
        if (!iconName.isEmpty()) {
            mMailbox->setIcon(iconName);
            iconButton->setIcon(QIcon::fromTheme(iconName));
        }
    });
    mMailbox->setIcon(iconButton->icon().name());

    l->addSpacing(10);

    auto ftw = new MailCommon::FolderTreeWidget(nullptr,
                                                nullptr,
                                                MailCommon::FolderTreeWidget::TreeViewOptions(MailCommon::FolderTreeWidget::UseDistinctSelectionModel
                                                                                              | MailCommon::FolderTreeWidget::HideStatistics
                                                                                              | MailCommon::FolderTreeWidget::HideHeaderViewMenu));
    ftw->folderTreeView()->setDragEnabled(false);
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

    auto selfFilter = new SelfFilterProxyModel(this);
    selfFilter->setSourceModel(checkable);

    ftv->setModel(selfFilter);
    ftv->expandAll();

    auto box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(box, &QDialogButtonBox::accepted, this, [checkable, this]() {
        const auto indexes = checkable->selectionModel()->selectedIndexes();
        QSet<qint64> list;
        list.reserve(indexes.count());
        for (const auto &index : indexes) {
            list << index.data(Akonadi::EntityTreeModel::CollectionIdRole).toLongLong();
        }
        mMailbox->setSourceCollections(list);
        accept();
    });
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(nameEdit, &QLineEdit::textChanged, box, [box](const QString &name) {
        box->button(QDialogButtonBox::Ok)->setEnabled(!name.trimmed().isEmpty());
    });
    box->button(QDialogButtonBox::Ok)->setEnabled(!nameEdit->text().trimmed().isEmpty());
    l->addWidget(box);
    readConfig();
}

UnifiedMailboxEditor::~UnifiedMailboxEditor()
{
    writeConfig();
}

void UnifiedMailboxEditor::readConfig()
{
    auto editorGrp = mConfig->group(EditorGroup);
    const QSize size = editorGrp.readEntry("Size", QSize(600, 700));
    if (size.isValid()) {
        resize(size);
    }
}

void UnifiedMailboxEditor::writeConfig()
{
    auto editorGrp = mConfig->group(EditorGroup);
    editorGrp.writeEntry("Size", size());
    editorGrp.sync();
}

#include "unifiedmailboxeditor.moc"
