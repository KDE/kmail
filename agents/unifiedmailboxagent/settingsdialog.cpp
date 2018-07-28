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

#include "settingsdialog.h"
#include "unifiedmailboxmanager.h"
#include "unifiedmailboxeditor.h"
#include "unifiedmailbox.h"
#include "mailkernel.h"

#include <QStandardItemModel>
#include <QStandardItem>
#include <QListView>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QItemSelectionModel>

#include <KLocalizedString>
#include <KMessageBox>

#include <memory>

SettingsDialog::SettingsDialog(KSharedConfigPtr config, UnifiedMailboxManager &boxManager, WId windowId, QWidget *parent)
    : QDialog(parent)
    , mBoxManager(boxManager)
    , mKernel(new MailKernel(config, this))
{
    resize(500, 500);

    auto l = new QVBoxLayout;
    setLayout(l);

    auto h = new QHBoxLayout;
    l->addLayout(h);
    mBoxModel = new QStandardItemModel(this);
    auto view = new QListView(this);
    view->setEditTriggers(QListView::NoEditTriggers);
    view->setModel(mBoxModel);
    h->addWidget(view);

    auto v = new QVBoxLayout;
    h->addLayout(v);
    auto addButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add-symbolic")), i18n("Add"));
    v->addWidget(addButton);
    connect(addButton, &QPushButton::clicked,
            this, [this]() {
                auto mailbox = std::make_unique<UnifiedMailbox>();
                auto editor = new UnifiedMailboxEditor(mailbox.get(), this);
                if (editor->exec()) {
                    mailbox->setId(mailbox->name()); // assign ID
                    addBox(mailbox.get());
                    mBoxManager.insertBox(std::move(mailbox));
                }
            });
    auto editButton = new QPushButton(QIcon::fromTheme(QStringLiteral("entry-edit")), i18n("Modify"));
    editButton->setEnabled(false);
    v->addWidget(editButton);
    connect(editButton, &QPushButton::clicked,
            this, [this, view]() {
                const auto indexes = view->selectionModel()->selectedIndexes();
                if (!indexes.isEmpty()) {
                    auto item = mBoxModel->itemFromIndex(indexes[0]);
                    auto mailbox = item->data().value<UnifiedMailbox*>();
                    auto editor = new UnifiedMailboxEditor(mailbox, this);
                    if (editor->exec()) {
                        item->setText(mailbox->name());
                        item->setIcon(QIcon::fromTheme(mailbox->icon()));
                    }
                }
            });
    auto removeButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove-symbolic")), i18n("Remove"));
    removeButton->setEnabled(false);
    v->addWidget(removeButton);
    connect(removeButton, &QPushButton::clicked,
            this, [this, view]() {
                const auto indexes = view->selectionModel()->selectedIndexes();
                if (!indexes.isEmpty()) {
                    auto item = mBoxModel->itemFromIndex(indexes[0]);
                    const auto mailbox = item->data().value<UnifiedMailbox*>();
                    if (KMessageBox::warningYesNo(
                            this, i18n("Do you really want to remove unified mailbox <b>%1</b>?", mailbox->name()),
                            i18n("Really Remove?"), KStandardGuiItem::remove(), KStandardGuiItem::cancel()) == KMessageBox::Yes)
                    {
                        mBoxModel->removeRow(item->row());
                        mBoxManager.removeBox(mailbox->id());
                    }
                }
            });
    v->addStretch(1);

    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [view, editButton, removeButton]() {
                const bool hasSelection = view->selectionModel()->hasSelection();
                editButton->setEnabled(hasSelection);
                removeButton->setEnabled(hasSelection);
            });

    auto box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(box, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    l->addWidget(box);

    loadBoxes();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::accept()
{
    mBoxManager.saveBoxes();

    QDialog::accept();
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
