/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "potentialphishingdetaildialog.h"
#include <KSharedConfig>
#include <KLocalizedString>
#include <qboxlayout.h>
#include <QLabel>
#include <QListWidget>


PotentialPhishingDetailDialog::PotentialPhishingDetailDialog(QWidget *parent)
    : KDialog(parent)
{
    //Add i18n in kf5
    setCaption( QLatin1String( "Details" ) );
    setButtons( Ok|Cancel );
    setDefaultButton( Ok );

    setModal( true );
    QWidget *mainWidget = new QWidget( this );
    QVBoxLayout *mainLayout = new QVBoxLayout( mainWidget );
    //kf5 add i18n
    QLabel *lab = new QLabel(QLatin1String("Select email to put in whitelist:"));
    lab->setObjectName(QLatin1String("label"));
    mainLayout->addWidget(lab);

    mListWidget = new QListWidget;
    mListWidget->setObjectName(QLatin1String("list_widget"));
    mainLayout->addWidget(mListWidget);

    connect(this, SIGNAL(okClicked()), this, SLOT(slotSave()));
    setMainWidget(mainWidget);
    readConfig();
}

PotentialPhishingDetailDialog::~PotentialPhishingDetailDialog()
{
    writeConfig();
}

void PotentialPhishingDetailDialog::fillList(const QStringList &lst)
{
    mListWidget->clear();
    QStringList emailsAdded;
    Q_FOREACH(const QString & mail, lst) {
        if (!emailsAdded.contains(mail)) {
            QListWidgetItem *item = new QListWidgetItem(mListWidget);
            item->setCheckState(Qt::Unchecked);
            item->setText(mail);
            emailsAdded << mail;
        }
    }
}

void PotentialPhishingDetailDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), "PotentialPhishingDetailDialog" );
    const QSize sizeDialog = group.readEntry( "Size", QSize(800,600) );
    if ( sizeDialog.isValid() ) {
        resize( sizeDialog );
    }
}

void PotentialPhishingDetailDialog::writeConfig()
{
    KConfigGroup group( KGlobal::config(), "PotentialPhishingDetailDialog" );
    group.writeEntry( "Size", size() );
}

void PotentialPhishingDetailDialog::slotSave()
{
    KConfigGroup group( KGlobal::config(), "PotentialPhishing");
    QStringList potentialPhishing = group.readEntry("whiteList", QStringList());
    for (int i=0; i < mListWidget->count(); ++i) {
        QListWidgetItem *item = mListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            QString email = item->text();
            if (!potentialPhishing.contains(email)) {
                potentialPhishing << email;
            }
        }
    }
    group.writeEntry( "whiteList", potentialPhishing);
    accept();
}
