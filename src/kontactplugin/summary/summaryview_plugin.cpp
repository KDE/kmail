/*
  This file is part of the KDE project

  SPDX-FileCopyrightText: 2003 Sven Lüppken <sven@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "summaryview_plugin.h"

#include "kmail-version.h"
#include "kmailinterface.h"
#include "summaryview_part.h"

#include <KontactInterface/Core>

#include <KAboutData>
#include <KActionCollection>
#include <KLocalizedString>
#include <KSelectAction>
#include <QIcon>

#include <QMenu>
using namespace Qt::Literals::StringLiterals;

EXPORT_KONTACT_PLUGIN_WITH_JSON(SummaryView, "summaryplugin.json")

SummaryView::SummaryView(KontactInterface::Core *core, const KPluginMetaData &data, const QVariantList &)
    : KontactInterface::Plugin(core, core, data, nullptr)
    , mSyncAction(new KSelectAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("Sync All"), this))
{
    actionCollection()->addAction(QStringLiteral("kontact_summary_sync"), mSyncAction);
    connect(mSyncAction, &KSelectAction::actionTriggered, this, &SummaryView::syncAccount);
    connect(mSyncAction->menu(), &QMenu::aboutToShow, this, &SummaryView::fillSyncActionSubEntries);

    insertSyncAction(mSyncAction);
    fillSyncActionSubEntries();
}

void SummaryView::fillSyncActionSubEntries()
{
    mSyncAction->clear();

    mAllSync = mSyncAction->addAction(i18nc("@action:inmenu sync everything", "All"));

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.kmail"))) {
        QStringList menuItems;
        org::kde::kmail::kmail kmail(QStringLiteral("org.kde.kmail"), QStringLiteral("/KMail"), QDBusConnection::sessionBus());
        const QDBusReply<QStringList> reply = kmail.accounts();
        if (reply.isValid()) {
            menuItems << reply.value();
        }

        for (const QString &acc : std::as_const(menuItems)) {
            mSyncAction->addAction(acc);
        }
    }
}

void SummaryView::syncAccount(QAction *act)
{
    if (act == mAllSync) {
        doSync();
    } else {
        org::kde::kmail::kmail kmail(QStringLiteral("org.kde.kmail"), QStringLiteral("/KMail"), QDBusConnection::sessionBus());
        kmail.checkAccount(act->text());
    }
    fillSyncActionSubEntries();
}

SummaryView::~SummaryView() = default;

void SummaryView::doSync()
{
    if (mPart) {
        mPart->updateSummaries();
    }

    const QList<KontactInterface::Plugin *> pluginList = core()->pluginList();
    for (const KontactInterface::Plugin *i : pluginList) {
        // execute all sync actions but our own
        const QList<QAction *> actList = i->syncActions();
        for (QAction *j : actList) {
            if (j != mSyncAction) {
                j->trigger();
            }
        }
    }
    fillSyncActionSubEntries();
}

KParts::Part *SummaryView::createPart()
{
    mPart = new SummaryViewPart(core(), aboutData(), this);
    mPart->setObjectName("summaryPart"_L1);
    return mPart;
}

const KAboutData SummaryView::aboutData()
{
    KAboutData aboutData = KAboutData(QStringLiteral("kontactsummary"),
                                      i18n("Kontact Summary"),
                                      QStringLiteral(KDEPIM_VERSION),
                                      i18n("Kontact Summary View"),
                                      KAboutLicense::LGPL,
                                      i18n("© 2003-2025 The Kontact developers"));

    aboutData.addAuthor(i18nc("@info:credit", "Sven Lueppken"), QString(), QStringLiteral("sven@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Cornelius Schumacher"), QString(), QStringLiteral("schumacher@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Tobias Koenig"), QString(), QStringLiteral("tokoe@kde.org"));
    aboutData.setProductName(QByteArrayLiteral("kontact/summary"));
    return aboutData;
}

#include "summaryview_plugin.moc"

#include "moc_summaryview_plugin.cpp"
