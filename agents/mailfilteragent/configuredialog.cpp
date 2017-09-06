/*
   Copyright (C) 2015-2017 Montel Laurent <montel@kde.org>

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
#include "configuredialog.h"
#include "configurewidget.h"
#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <kconfiggroup.h>
#include <KSharedConfig>
#include <QPushButton>

ConfigureDialog::ConfigureDialog(QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mConfigureWidget = new ConfigureWidget(this);
    mConfigureWidget->setObjectName(QStringLiteral("configurewidget"));
    mainLayout->addWidget(mConfigureWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->setObjectName(QStringLiteral("buttonbox"));
    mainLayout->addWidget(buttonBox);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigureDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfigureDialog::reject);

    readConfig();
}

ConfigureDialog::~ConfigureDialog()
{
    writeConfig();
}

void ConfigureDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "ConfigureDialog");
    const QSize sizeDialog = group.readEntry("Size", QSize(600, 400));
    if (sizeDialog.isValid()) {
        resize(sizeDialog);
    }
}

void ConfigureDialog::writeConfig()
{
    KConfigGroup myGroup(KSharedConfig::openConfig(), "Geometry");
    myGroup.writeEntry("ConfigureDialog", size());
    myGroup.sync();
}
