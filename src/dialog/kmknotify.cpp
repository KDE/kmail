/*
   Copyright (C) 2011-2019 Montel Laurent <montel@kde.org>

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

#include "kmknotify.h"
#include "kmkernel.h"

#include <QVBoxLayout>
#include <QComboBox>
#include <KNotifyConfigWidget>
#include <KLocalizedString>
#include <KConfig>
#include <KSeparator>
#include <QStandardPaths>
#include <QDialogButtonBox>
#include <KConfigGroup>
#include <QPushButton>

using namespace KMail;

KMKnotify::KMKnotify(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Notification"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_comboNotify = new QComboBox(this);
    m_comboNotify->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mainLayout->addWidget(m_comboNotify);

    m_notifyWidget = new KNotifyConfigWidget(this);
    mainLayout->addWidget(m_notifyWidget);
    m_comboNotify->setFocus();

    mainLayout->addWidget(new KSeparator);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &KMKnotify::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &KMKnotify::reject);

    mainLayout->addWidget(buttonBox);

    connect(m_comboNotify, qOverload<int>(&QComboBox::activated), this, &KMKnotify::slotComboChanged);
    connect(okButton, &QPushButton::clicked, this, &KMKnotify::slotOk);
    connect(m_notifyWidget, &KNotifyConfigWidget::changed, this, &KMKnotify::slotConfigChanged);
    initCombobox();
    readConfig();
}

KMKnotify::~KMKnotify()
{
    writeConfig();
}

void KMKnotify::slotConfigChanged(bool changed)
{
    m_changed = changed;
}

void KMKnotify::slotComboChanged(int index)
{
    QString text(m_comboNotify->itemData(index).toString());
    if (m_changed) {
        m_notifyWidget->save();
        m_changed = false;
    }
    m_notifyWidget->setApplication(text);
}

void KMKnotify::setCurrentNotification(const QString &name)
{
    const int index = m_comboNotify->findData(name);
    if (index > -1) {
        m_comboNotify->setCurrentIndex(index);
        slotComboChanged(index);
    }
}

void KMKnotify::initCombobox()
{
    const QStringList lstNotify = QStringList() << QStringLiteral("kmail2.notifyrc")
                                                << QStringLiteral("akonadi_maildispatcher_agent.notifyrc")
                                                << QStringLiteral("akonadi_mailfilter_agent.notifyrc")
                                                << QStringLiteral("akonadi_archivemail_agent.notifyrc")
                                                << QStringLiteral("akonadi_sendlater_agent.notifyrc")
                                                << QStringLiteral("akonadi_newmailnotifier_agent.notifyrc")
                                                << QStringLiteral("akonadi_followupreminder_agent.notifyrc")
                                                << QStringLiteral("messageviewer.notifyrc");
    //TODO add other notifyrc here if necessary

    for (const QString &notify : lstNotify) {
        const QString fullPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("knotifications5/") + notify);

        if (!fullPath.isEmpty()) {
            const int slash = fullPath.lastIndexOf(QLatin1Char('/'));
            QString appname = fullPath.right(fullPath.length() - slash - 1);
            appname.remove(QStringLiteral(".notifyrc"));
            if (!appname.isEmpty()) {
                KConfig config(fullPath, KConfig::NoGlobals, QStandardPaths::AppLocalDataLocation);
                KConfigGroup globalConfig(&config, QStringLiteral("Global"));
                const QString icon = globalConfig.readEntry(QStringLiteral("IconName"), QStringLiteral("misc"));
                const QString description = globalConfig.readEntry(QStringLiteral("Comment"), appname);
                m_comboNotify->addItem(QIcon::fromTheme(icon), description, appname);
            }
        }
    }

    m_comboNotify->model()->sort(0);
    if (m_comboNotify->count() > 0) {
        m_comboNotify->setCurrentIndex(0);
        m_notifyWidget->setApplication(m_comboNotify->itemData(0).toString());
    }
}

void KMKnotify::slotOk()
{
    if (m_changed) {
        m_notifyWidget->save();
    }
}

void KMKnotify::readConfig()
{
    KConfigGroup notifyDialog(KMKernel::self()->config(), "KMKnotifyDialog");
    const QSize size = notifyDialog.readEntry("Size", QSize(600, 400));
    if (size.isValid()) {
        resize(size);
    }
}

void KMKnotify::writeConfig()
{
    KConfigGroup notifyDialog(KMKernel::self()->config(), "KMKnotifyDialog");
    notifyDialog.writeEntry("Size", size());
    notifyDialog.sync();
}
