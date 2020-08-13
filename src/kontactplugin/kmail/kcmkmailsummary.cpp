/*
  This file is part of Kontact.

  SPDX-FileCopyrightText: 2004 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "kcmkmailsummary.h"

#include <PimCommonAkonadi/CheckedCollectionWidget>
#include <AkonadiWidgets/ETMViewStateSaver>
#include <KMime/KMimeMessage>

#include <KAboutData>
#include <KAcceleratorManager>
#include "kmailplugin_debug.h"
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfig>

#include <QCheckBox>
#include <QTreeView>
#include <QVBoxLayout>

extern "C"
{
Q_DECL_EXPORT KCModule *create_kmailsummary(QWidget *parent, const char *)
{
    return new KCMKMailSummary(parent);
}
}

KCMKMailSummary::KCMKMailSummary(QWidget *parent)
    : KCModule(parent)
{
    initGUI();

    connect(mCheckedCollectionWidget->folderTreeView(), &QAbstractItemView::clicked,
            this, &KCMKMailSummary::modified);
    connect(mFullPath, &QCheckBox::toggled, this, &KCMKMailSummary::modified);

    KAcceleratorManager::manage(this);

    load();
    KAboutData *about = new KAboutData(QStringLiteral("kcmkmailsummary"),
                                       i18n("kcmkmailsummary"),
                                       QString(),
                                       i18n("Mail Summary Configuration Dialog"),
                                       KAboutLicense::GPL,
                                       i18n("Copyright © 2004–2010 Tobias Koenig"));
    about->addAuthor(ki18n("Tobias Koenig").toString(), QString(), QStringLiteral("tokoe@kde.org"));
    setAboutData(about);
}

void KCMKMailSummary::modified()
{
    Q_EMIT changed(true);
}

void KCMKMailSummary::initGUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    mCheckedCollectionWidget = new PimCommon::CheckedCollectionWidget(KMime::Message::mimeType());

    mFullPath = new QCheckBox(i18n("Show full path for folders"), this);
    mFullPath->setToolTip(
        i18nc("@info:tooltip", "Show full path for each folder"));
    mFullPath->setWhatsThis(
        i18nc("@info:whatsthis",
              "Enable this option if you want to see the full path "
              "for each folder listed in the summary. If this option is "
              "not enabled, then only the base folder path will be shown."));
    layout->addWidget(mCheckedCollectionWidget);
    layout->addWidget(mFullPath);
}

void KCMKMailSummary::initFolders()
{
    KSharedConfigPtr _config = KSharedConfig::openConfig(QStringLiteral("kcmkmailsummaryrc"));

    mModelState
        = new KViewStateMaintainer<Akonadi::ETMViewStateSaver>(_config->group("CheckState"), this);
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

    Q_EMIT changed(false);
}

void KCMKMailSummary::save()
{
    storeFolders();

    Q_EMIT changed(false);
}

void KCMKMailSummary::defaults()
{
    mFullPath->setChecked(true);

    Q_EMIT changed(true);
}
