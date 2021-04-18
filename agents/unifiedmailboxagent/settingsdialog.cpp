/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "settingsdialog.h"
#include "mailkernel.h"
#include "unifiedmailbox.h"
#include "unifiedmailboxeditor.h"

#include <QDialogButtonBox>
#include <QItemSelectionModel>
#include <QListView>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>

#include <memory>

namespace
{
static constexpr const char *DialogGroup = "UnifiedMailboxSettingsDialog";
}

SettingsDialog::SettingsDialog(const KSharedConfigPtr &config, UnifiedMailboxManager &boxManager, WId, QWidget *parent)
    : QDialog(parent)
    , mBoxModel(new QStandardItemModel(this))
    , mBoxManager(boxManager)
    , mKernel(new MailKernel(config, this))
    , mConfig(config)
{
    setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    auto l = new QVBoxLayout(this);

    auto h = new QHBoxLayout;
    l->addLayout(h);
    auto view = new QListView(this);
    view->setEditTriggers(QListView::NoEditTriggers);
    view->setModel(mBoxModel);
    h->addWidget(view);

    auto v = new QVBoxLayout;
    h->addLayout(v);
    auto addButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add-symbolic")), i18n("Add"));
    v->addWidget(addButton);
    connect(addButton, &QPushButton::clicked, this, [this]() {
        auto mailbox = std::make_unique<UnifiedMailbox>();
        auto editor = new UnifiedMailboxEditor(mailbox.get(), mConfig, this);
        if (editor->exec()) {
            mailbox->setId(mailbox->name()); // assign ID
            addBox(mailbox.get());
            mBoxManager.insertBox(std::move(mailbox));
            mBoxManager.saveBoxes();
        }
        delete editor;
    });
    auto editButton = new QPushButton(QIcon::fromTheme(QStringLiteral("entry-edit")), i18n("Modify"));
    editButton->setEnabled(false);
    v->addWidget(editButton);

    const auto modifyMailBox = [this, view]() {
        const auto indexes = view->selectionModel()->selectedIndexes();
        if (!indexes.isEmpty()) {
            auto item = mBoxModel->itemFromIndex(indexes[0]);
            auto mailbox = item->data().value<UnifiedMailbox *>();
            auto editor = new UnifiedMailboxEditor(mailbox, mConfig, this);
            if (editor->exec()) {
                item->setText(mailbox->name());
                item->setIcon(QIcon::fromTheme(mailbox->icon()));
            }
            delete editor;
            mBoxManager.saveBoxes();
        }
    };

    connect(view, &QListView::doubleClicked, this, modifyMailBox);
    connect(editButton, &QPushButton::clicked, this, modifyMailBox);

    auto removeButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove-symbolic")), i18n("Remove"));
    removeButton->setEnabled(false);
    v->addWidget(removeButton);
    connect(removeButton, &QPushButton::clicked, this, [this, view]() {
        const auto indexes = view->selectionModel()->selectedIndexes();
        if (!indexes.isEmpty()) {
            auto item = mBoxModel->itemFromIndex(indexes[0]);
            const auto mailbox = item->data().value<UnifiedMailbox *>();
            if (KMessageBox::warningYesNo(this,
                                          i18n("Do you really want to remove unified mailbox <b>%1</b>?", mailbox->name()),
                                          i18n("Really Remove?"),
                                          KStandardGuiItem::remove(),
                                          KStandardGuiItem::cancel())
                == KMessageBox::Yes) {
                mBoxModel->removeRow(item->row());
                mBoxManager.removeBox(mailbox->id());
                mBoxManager.saveBoxes();
            }
        }
    });
    v->addStretch(1);

    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, [view, editButton, removeButton]() {
        const bool hasSelection = view->selectionModel()->hasSelection();
        editButton->setEnabled(hasSelection);
        removeButton->setEnabled(hasSelection);
    });

    auto box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    l->addWidget(box);

    loadBoxes();
    readConfig();
}

SettingsDialog::~SettingsDialog()
{
    writeConfig();
}

void SettingsDialog::readConfig()
{
    auto dlgGroup = mConfig->group(DialogGroup);
    const QSize size = dlgGroup.readEntry("Size", QSize(500, 500));
    if (size.isValid()) {
        resize(size);
    }
}

void SettingsDialog::writeConfig()
{
    auto dlgGroup = mConfig->group(DialogGroup);
    dlgGroup.writeEntry("Size", size());
    dlgGroup.sync();
}

void SettingsDialog::loadBoxes()
{
    mBoxModel->clear();
    for (const auto &mailboxIt : mBoxManager) {
        addBox(mailboxIt.second.get());
    }
}

void SettingsDialog::addBox(UnifiedMailbox *box)
{
    auto item = new QStandardItem(QIcon::fromTheme(box->icon()), box->name());
    item->setData(QVariant::fromValue(box));
    mBoxModel->appendRow(item);
}
