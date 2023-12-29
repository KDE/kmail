/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "addarchivemaildialog.h"

#include "addarchivemailwidget.h"
#include <KLocalizedString>
#include <KSeparator>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

AddArchiveMailDialog::AddArchiveMailDialog(ArchiveMailInfo *info, QWidget *parent)
    : QDialog(parent)
    , mAddArchiveMailWidget(new AddArchiveMailWidget(info, this))
{
    if (info) {
        setWindowTitle(i18nc("@title:window", "Modify Archive Mail"));
    } else {
        setWindowTitle(i18nc("@title:window", "Add Archive Mail"));
    }
    setModal(true);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));

    auto topLayout = new QVBoxLayout(this);
    topLayout->setObjectName(QStringLiteral("topLayout"));

    mAddArchiveMailWidget->setObjectName(QStringLiteral("mAddArchiveMailWidget"));
    topLayout->addWidget(mAddArchiveMailWidget);
    connect(mAddArchiveMailWidget, &AddArchiveMailWidget::enableOkButton, this, [this](bool state) {
        mOkButton->setEnabled(state);
    });

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddArchiveMailDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AddArchiveMailDialog::reject);

    if (!info) {
        mOkButton->setEnabled(false);
    }
    topLayout->addWidget(new KSeparator);
    topLayout->addWidget(buttonBox);

    // Make it a bit bigger, else the folder requester cuts off the text too early
    resize(500, minimumSize().height());
}

AddArchiveMailDialog::~AddArchiveMailDialog() = default;

ArchiveMailInfo *AddArchiveMailDialog::info()
{
    return mAddArchiveMailWidget->info();
}

#include "moc_addarchivemaildialog.cpp"
