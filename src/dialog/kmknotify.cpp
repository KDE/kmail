/*
   SPDX-FileCopyrightText: 2011-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmknotify.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KNotifyConfigWidget>
#include <KSeparator>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QIcon>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWindow>
#include <TextAddonsWidgets/LoadDialogSizeUtils>
#include <array>

using namespace KMail;
using namespace Qt::Literals::StringLiterals;

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

    connect(m_comboNotify, &QComboBox::activated, this, &KMKnotify::slotComboChanged);
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
    if (index < 0 || index >= m_comboNotify->count()) {
        return;
    }
    const QString text = m_comboNotify->itemData(index).toString();
    if (m_changed) {
        m_notifyWidget->save();
        m_changed = false;
    }
    m_notifyWidget->setApplication(text);
}

void KMKnotify::setCurrentNotification(const QString &name)
{
    if (const int index = m_comboNotify->findData(name); index > -1) {
        m_comboNotify->setCurrentIndex(index);
        slotComboChanged(index);
    }
}

void KMKnotify::initCombobox()
{
    static constexpr std::array<QLatin1StringView, 8> lstNotify = {
        "kmail2"_L1,
        "akonadi_maildispatcher_agent"_L1,
        "akonadi_mailfilter_agent"_L1,
        "akonadi_archivemail_agent"_L1,
        "akonadi_sendlater_agent"_L1,
        "akonadi_newmailnotifier_agent"_L1,
        "akonadi_followupreminder_agent"_L1,
        "messageviewer"_L1,
    };
    for (const QLatin1StringView notify : lstNotify) {
        const QString appname = notify;
        const QString fullPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"knotifications6/"_s + appname + ".notifyrc"_L1);
        if (fullPath.isEmpty()) {
            continue;
        }
        KConfig config(fullPath, KConfig::NoGlobals, QStandardPaths::AppLocalDataLocation);
        KConfigGroup globalConfig(&config, u"Global"_s);
        const QString icon = globalConfig.readEntry(u"IconName"_s, u"misc"_s);
        const QString description = globalConfig.readEntry(u"Comment"_s, appname);
        m_comboNotify->addItem(QIcon::fromTheme(icon), description, appname);
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
#if TEXTADDONSWIDGETS_VERSION >= QT_VERSION_CHECK(2, 1, 49)
    TextAddonsWidgets::LoadDialogSizeUtils::manageDialogSize(this, u"KMKnotifyDialog"_s, QSize(600, 400));
#else
    create(); // ensure a window is created
    TextAddonsWidgets::LoadDialogSizeUtils::loadDialogSizeScaled(this, u"KMKnotifyDialog"_s, 600, 400);
#endif
}

void KMKnotify::writeConfig()
{
#if TEXTADDONSWIDGETS_VERSION < QT_VERSION_CHECK(2, 1, 49)
    KConfigGroup notifyDialog(KSharedConfig::openStateConfig(), u"KMKnotifyDialog"_s);
    KWindowConfig::saveWindowSize(windowHandle(), notifyDialog);
    notifyDialog.sync();
#endif
}

#include "moc_kmknotify.cpp"
