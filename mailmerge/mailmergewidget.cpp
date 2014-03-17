/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "mailmergewidget.h"

#include <KLocalizedString>
#include <KComboBox>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>


MailMergeWidget::MailMergeWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);

    QHBoxLayout *hbox = new QHBoxLayout;
    vbox->addLayout(hbox);

    QLabel *lab = new QLabel(i18n("Source:"));
    hbox->addWidget(lab);

    mSource = new KComboBox;
    mSource->setObjectName(QLatin1String("source"));
    mSource->addItem(i18n("Address Book"), AddressBook);
    mSource->addItem(i18n("CSV"), CSV);
    connect(mSource, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSourceChanged(int)));
    connect(mSource, SIGNAL(activated(int)), this, SLOT(slotSourceChanged(int)));

    hbox->addWidget(mSource);
}


MailMergeWidget::~MailMergeWidget()
{

}

void MailMergeWidget::slotSourceChanged(int index)
{
    if (index != -1) {
        Q_EMIT sourceModeChanged(static_cast<SourceType>(mSource->itemData(index).toInt()));
    }
}
