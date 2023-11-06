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
KCMKMailSummary::KCMKMailSummary(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
    , mFullPath(new QCheckBox(i18n("Show full path for folders"), widget()))
{
    initGUI();

    connect(mCheckedCollectionWidget->folderTreeView(), &QAbstractItemView::clicked, this, &KCMKMailSummary::modified);
    connect(mFullPath, &QCheckBox::toggled, this, &KCMKMailSummary::modified);
    KAcceleratorManager::manage(widget());

    load();
}

void KCMKMailSummary::modified()
{
    markAsChanged();
}

void KCMKMailSummary::initGUI()
{
    auto layout = new QVBoxLayout(widget());
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

    mModelState = new KViewStateMaintainer<Akonadi::ETMViewStateSaver>(_config->group(QLatin1String("CheckState")), this);
    mModelState->setSelectionModel(mCheckedCollectionWidget->selectionModel());
}

void KCMKMailSummary::loadFolders()
{
    KConfig _config(QStringLiteral("kcmkmailsummaryrc"));
    KConfigGroup config(&_config, QLatin1String("General"));
    mModelState->restoreState();
    const bool showFolderPaths = config.readEntry("showFolderPaths", false);
    mFullPath->setChecked(showFolderPaths);
}

void KCMKMailSummary::storeFolders()
{
    KConfig _config(QStringLiteral("kcmkmailsummaryrc"));
    KConfigGroup config(&_config, QLatin1String("General"));
    mModelState->saveState();
    config.writeEntry("showFolderPaths", mFullPath->isChecked());
    config.sync();
}

void KCMKMailSummary::load()
{
    initFolders();
    loadFolders();

    setNeedsSave(false);
}

void KCMKMailSummary::save()
{
    storeFolders();

    setNeedsSave(false);
}

void KCMKMailSummary::defaults()
{
    mFullPath->setChecked(true);
    setNeedsSave(true);
}
#include "kcmkmailsummary.moc"

#include "moc_kcmkmailsummary.cpp"
