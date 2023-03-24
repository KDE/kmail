/*
  This file is part of Kontact.

  SPDX-FileCopyrightText: 2004 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2013-2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "kcmkmailsummary.h"

#include <Akonadi/ETMViewStateSaver>
#include <KMime/KMimeMessage>
#include <PimCommonAkonadi/CheckedCollectionWidget>

#include "kmailplugin_debug.h"
#include <KAboutData>
#include <KAcceleratorManager>
#include <KConfig>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QCheckBox>
#include <QTreeView>
#include <QVBoxLayout>

K_PLUGIN_CLASS_WITH_JSON(KCMKMailSummary, "kcmkmailsummary.json")
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
KCMKMailSummary::KCMKMailSummary(QWidget *parent, const QVariantList &args)
    : KCModule(parent, args)
    , mFullPath(new QCheckBox(i18n("Show full path for folders"), this))
#else
KCMKMailSummary::KCMKMailSummary(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KCModule(parent, data, args)
    , mFullPath(new QCheckBox(i18n("Show full path for folders"), widget()))
#endif
{
    initGUI();

    connect(mCheckedCollectionWidget->folderTreeView(), &QAbstractItemView::clicked, this, &KCMKMailSummary::modified);
    connect(mFullPath, &QCheckBox::toggled, this, &KCMKMailSummary::modified);
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    KAcceleratorManager::manage(this);
#else
    KAcceleratorManager::manage(widget());
#endif

    load();
}

void KCMKMailSummary::modified()
{
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    Q_EMIT changed(true);
#else
    markAsChanged();
#endif
}

void KCMKMailSummary::initGUI()
{
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    auto layout = new QVBoxLayout(this);
#else
    auto layout = new QVBoxLayout(widget());
#endif
    layout->setContentsMargins({});

    mCheckedCollectionWidget = new PimCommon::CheckedCollectionWidget(KMime::Message::mimeType());

    mFullPath->setToolTip(i18nc("@info:tooltip", "Show full path for each folder"));
    mFullPath->setWhatsThis(i18nc("@info:whatsthis",
                                  "Enable this option if you want to see the full path "
                                  "for each folder listed in the summary. If this option is "
                                  "not enabled, then only the base folder path will be shown."));
    layout->addWidget(mCheckedCollectionWidget);
    layout->addWidget(mFullPath);
}

void KCMKMailSummary::initFolders()
{
    KSharedConfigPtr _config = KSharedConfig::openConfig(QStringLiteral("kcmkmailsummaryrc"));

    mModelState = new KViewStateMaintainer<Akonadi::ETMViewStateSaver>(_config->group("CheckState"), this);
    mModelState->setSelectionModel(mCheckedCollectionWidget->selectionModel());
}

void KCMKMailSummary::loadFolders()
{
    KConfig _config(QStringLiteral("kcmkmailsummaryrc"));
    KConfigGroup config(&_config, "General");
    mModelState->restoreState();
    const bool showFolderPaths = config.readEntry("showFolderPaths", false);
    mFullPath->setChecked(showFolderPaths);
}

void KCMKMailSummary::storeFolders()
{
    KConfig _config(QStringLiteral("kcmkmailsummaryrc"));
    KConfigGroup config(&_config, "General");
    mModelState->saveState();
    config.writeEntry("showFolderPaths", mFullPath->isChecked());
    config.sync();
}

void KCMKMailSummary::load()
{
    initFolders();
    loadFolders();

#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    Q_EMIT changed(false);
#else
    setNeedsSave(false);
#endif
}

void KCMKMailSummary::save()
{
    storeFolders();

#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    Q_EMIT changed(false);
#else
    setNeedsSave(false);
#endif
}

void KCMKMailSummary::defaults()
{
    mFullPath->setChecked(true);
#if KCMUTILS_VERSION < QT_VERSION_CHECK(5, 240, 0)
    Q_EMIT changed(true);
#else
    setNeedsSave(true);
#endif
}
#include "kcmkmailsummary.moc"
