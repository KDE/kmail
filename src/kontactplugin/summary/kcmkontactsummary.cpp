/*
  This file is part of KDE Kontact.

  SPDX-FileCopyrightText: 2004 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2008 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "kcmkontactsummary.h"
#include <KAboutData>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <KontactInterface/Plugin>
#include <QIcon>

#include <QLabel>
#include <QVBoxLayout>

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

    [[nodiscard]] virtual QString text(int column) const
    {
        if (column == 0) {
            return mInfo.name();
        } else if (column == 1) {
            return mInfo.description();
        } else {
            return {};
        }
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
    const QList<KPluginMetaData> pluginMetaDatas = KPluginMetaData::findPlugins(QStringLiteral("pim6/kontact"), [](const KPluginMetaData &data) {
        return data.rawData().value(QStringLiteral("X-KDE-KontactPluginVersion")).toInt() == KONTACT_PLUGIN_VERSION;
    });

    QStringList activeSummaries;

    KConfig config(QStringLiteral("kontact_summaryrc"));
    KConfigGroup grp(&config, QString());
    if (grp.hasKey("ActiveSummaries")) {
        activeSummaries = grp.readEntry("ActiveSummaries", QStringList());
    } else {
        activeSummaries << QStringLiteral("kontact_kaddressbookplugin");
        activeSummaries << QStringLiteral("kontact_specialdatesplugin");
        activeSummaries << QStringLiteral("kontact_korganizerplugin");
        activeSummaries << QStringLiteral("kontact_todoplugin");
        activeSummaries << QStringLiteral("kontact_knotesplugin");
        activeSummaries << QStringLiteral("kontact_kmailplugin");
    }

    mPluginView->clear();

    for (const auto &plugin : std::as_const(pluginMetaDatas)) {
        const QVariant var = plugin.value(QStringLiteral("X-KDE-KontactPluginHasSummary"), false);
        if (var.isValid() && var.toBool() == true) {
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
        auto item = static_cast<PluginItem *>(*it);
        if (item->checkState(0) == Qt::Checked) {
            activeSummaries.append(item->pluginInfo().pluginId());
        }
        ++it;
    }

    KConfig config(QStringLiteral("kontact_summaryrc"));
    KConfigGroup grp(&config, QString());
    grp.writeEntry("ActiveSummaries", activeSummaries);
    setNeedsSave(false);
}
#include "kcmkontactsummary.moc"

#include "moc_kcmkontactsummary.cpp"
