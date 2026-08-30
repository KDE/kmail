/*
  This file is part of KDE Kontact.

  SPDX-FileCopyrightText: 2004 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2008 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "kcmkontactsummary.h"
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <KontactInterface/Plugin>
#include <QIcon>

#include <QLabel>
#include <QVBoxLayout>
using namespace Qt::Literals::StringLiterals;

class PluginItem : public QTreeWidgetItem
{
public:
    PluginItem(const KPluginMetaData &info, QTreeWidget *parent)
        : QTreeWidgetItem(parent)
        , mInfo(info)
    {
        setIcon(0, QIcon::fromTheme(mInfo.iconName()));
        setText(0, mInfo.name());
        setToolTip(0, mInfo.description());
        setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    }

    [[nodiscard]] KPluginMetaData pluginInfo() const
    {
        return mInfo;
    }

private:
    Q_DISABLE_COPY(PluginItem)
    const KPluginMetaData mInfo;
};

PluginView::PluginView(QWidget *parent)
    : QTreeWidget(parent)
{
    setColumnCount(1);
    setHeaderLabel(i18nc("@title:column plugin name", "Summary Plugin Name"));
    setRootIsDecorated(false);
}

PluginView::~PluginView() = default;

K_PLUGIN_CLASS_WITH_JSON(KCMKontactSummary, "kcmkontactsummary.json")

KCMKontactSummary::KCMKontactSummary(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
    , mPluginView(new PluginView(widget()))
{
    auto layout = new QVBoxLayout(widget());

    layout->setContentsMargins({});
    auto label = new QLabel(i18nc("@label:textbox", "Select the plugin summaries to show on the summary page."), widget());
    layout->addWidget(label);

    layout->addWidget(mPluginView);

    layout->setStretchFactor(mPluginView, 1);

    load();
    connect(mPluginView, &QTreeWidget::itemChanged, this, &KCMKontactSummary::markAsChanged);
}

void KCMKontactSummary::load()
{
    const QList<KPluginMetaData> pluginMetaDatas = KPluginMetaData::findPlugins(u"pim6/kontact"_s, [](const KPluginMetaData &data) {
        return data.rawData().value(u"X-KDE-KontactPluginVersion"_s).toInt() == KONTACT_PLUGIN_VERSION;
    });

    QStringList activeSummaries;

    const KConfig config(u"kontact_summaryrc"_s);
    if (const KConfigGroup grp(&config, QString()); grp.hasKey("ActiveSummaries")) {
        activeSummaries = grp.readEntry("ActiveSummaries", QStringList());
    } else {
        activeSummaries << u"kontact_kaddressbookplugin"_s;
        activeSummaries << u"kontact_specialdatesplugin"_s;
        activeSummaries << u"kontact_korganizerplugin"_s;
        activeSummaries << u"kontact_todoplugin"_s;
        activeSummaries << u"kontact_knotesplugin"_s;
        activeSummaries << u"kontact_kmailplugin"_s;
    }

    mPluginView->clear();

    for (const auto &plugin : std::as_const(pluginMetaDatas)) {
        if (const QVariant var = plugin.value(u"X-KDE-KontactPluginHasSummary"_s, false); var.isValid() && var.toBool() == true) {
            auto item = new PluginItem(plugin, mPluginView);

            if (activeSummaries.contains(plugin.pluginId())) {
                item->setCheckState(0, Qt::Checked);
            } else {
                item->setCheckState(0, Qt::Unchecked);
            }
        }
    }
    setNeedsSave(false);
}

void KCMKontactSummary::save()
{
    QStringList activeSummaries;

    QTreeWidgetItemIterator it(mPluginView);
    while (*it) {
        if (auto item = static_cast<PluginItem *>(*it); item->checkState(0) == Qt::Checked) {
            activeSummaries.append(item->pluginInfo().pluginId());
        }
        ++it;
    }

    KConfig config(u"kontact_summaryrc"_s);
    KConfigGroup grp(&config, QString());
    grp.writeEntry("ActiveSummaries", activeSummaries);
    setNeedsSave(false);
}
#include "kcmkontactsummary.moc"

#include "moc_kcmkontactsummary.cpp"
