/*
   SPDX-FileCopyrightText: 2011-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmknotify.h"
#include "kmkernel.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KNotifyConfigWidget>
#include <KSeparator>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

using namespace KMail;

KMKnotify::KMKnotify(QWidget *parent)
    : QDialog(parent)
    , m_comboNotify(new QComboBox(this))
    , m_notifyWidget(new KNotifyConfigWidget(this))
{
    setWindowTitle(i18nc("@title:window", "Notification"));
    auto mainLayout = new QVBoxLayout(this);

    m_comboNotify->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mainLayout->addWidget(m_comboNotify);

    mainLayout->addWidget(m_notifyWidget);
    m_comboNotify->setFocus();

    mainLayout->addWidget(new KSeparator(this));

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
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
    const QStringList lstNotify = QStringList() << QStringLiteral("kmail2.notifyrc") << QStringLiteral("akonadi_maildispatcher_agent.notifyrc")
                                                << QStringLiteral("akonadi_mailfilter_agent.notifyrc") << QStringLiteral("akonadi_archivemail_agent.notifyrc")
                                                << QStringLiteral("akonadi_sendlater_agent.notifyrc")
                                                << QStringLiteral("akonadi_newmailnotifier_agent.notifyrc")
                                                << QStringLiteral("akonadi_followupreminder_agent.notifyrc") << QStringLiteral("messageviewer.notifyrc");
    // TODO add other notifyrc here if necessary

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
