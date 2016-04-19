/*
  This file is part of KDE Kontact.

  Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>
  Copyright (c) 2008 Allen Winter <winter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "kcmkontactsummary.h"

#include <KontactInterface/Plugin>

#include <KAboutData>
#include <QIcon>
#include <KLocalizedString>
#include <KPluginInfo>
#include <KService>
#include <KServiceTypeTrader>
#include <KConfig>

#include <QLabel>
#include <QVBoxLayout>

extern "C"
{
    Q_DECL_EXPORT KCModule *create_kontactsummary(QWidget *parent, const char *)
    {
        return new KCMKontactSummary(parent);
    }
}

class PluginItem : public QTreeWidgetItem
{
public:
    PluginItem(const KPluginInfo &info, QTreeWidget *parent)
        : QTreeWidgetItem(parent), mInfo(info)
    {
        setIcon(0, QIcon::fromTheme(mInfo.icon()));
        setText(0, mInfo.name());
        setToolTip(0, mInfo.comment());
        setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    }

    KPluginInfo pluginInfo() const
    {
        return mInfo;
    }

    virtual QString text(int column) const
    {
        if (column == 0) {
            return mInfo.name();
        } else if (column == 1) {
            return mInfo.comment();
        } else {
            return QString();
        }
    }

private:
    KPluginInfo mInfo;
};

PluginView::PluginView(QWidget *parent)
    : QTreeWidget(parent)
{
    setColumnCount(1);
    setHeaderLabel(i18nc("@title:column plugin name", "Summary Plugin Name"));
    setRootIsDecorated(false);
}

PluginView::~PluginView()
{
}

KCMKontactSummary::KCMKontactSummary(QWidget *parent)
    : KCModule(parent)
{
    setButtons(NoAdditionalButton);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    QLabel *label =
        new QLabel(i18n("Select the plugin summaries to show on the summary page."), this);
    layout->addWidget(label);

    mPluginView = new PluginView(this);
    layout->addWidget(mPluginView);

    layout->setStretchFactor(mPluginView, 1);

    load();
    connect(mPluginView, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(changed()));

    KAboutData *about = new KAboutData(QStringLiteral("kontactsummary"),
                                       i18n("kontactsummary"),
                                       QString(),
                                       i18n("KDE Kontact Summary"),
                                       KAboutLicense::GPL,
                                       i18n("(c), 2004 Tobias Koenig"));
    about->addAuthor(ki18n("Tobias Koenig").toString(), QString(), QStringLiteral("tokoe@kde.org"));
    setAboutData(about);
}

void KCMKontactSummary::load()
{
    KService::List offers = KServiceTypeTrader::self()->query(
                                QStringLiteral("Kontact/Plugin"),
                                QStringLiteral("[X-KDE-KontactPluginVersion] == %1").arg(KONTACT_PLUGIN_VERSION));

    QStringList activeSummaries;

    KConfig config(QStringLiteral("kontact_summaryrc"));
    KConfigGroup grp(&config, QString());
    if (!grp.hasKey("ActiveSummaries")) {
        activeSummaries << QStringLiteral("kontact_kaddressbookplugin");
        activeSummaries << QStringLiteral("kontact_specialdatesplugin");
        activeSummaries << QStringLiteral("kontact_korganizerplugin");
        activeSummaries << QStringLiteral("kontact_todoplugin");
        activeSummaries << QStringLiteral("kontact_knotesplugin");
        activeSummaries << QStringLiteral("kontact_kmailplugin");
        activeSummaries << QStringLiteral("kontact_weatherplugin");
        activeSummaries << QStringLiteral("kontact_newstickerplugin");
        activeSummaries << QStringLiteral("kontact_plannerplugin");
    } else {
        activeSummaries = grp.readEntry("ActiveSummaries", QStringList());
    }

    mPluginView->clear();

    KPluginInfo::List pluginList =
        KPluginInfo::fromServices(offers, KConfigGroup(&config, "Plugins"));
    KPluginInfo::List::Iterator it;
    KPluginInfo::List::Iterator end(pluginList.end());
    for (it = pluginList.begin(); it != end; ++it) {
        it->load();

        if (!it->isPluginEnabled()) {
            continue;
        }

        QVariant var = it->property(QStringLiteral("X-KDE-KontactPluginHasSummary"));
        if (var.isValid() && var.toBool() == true) {
            PluginItem *item = new PluginItem(*it, mPluginView);

            if (activeSummaries.contains(it->pluginName())) {
                item->setCheckState(0, Qt::Checked);
            } else {
                item->setCheckState(0, Qt::Unchecked);
            }
        }
    }
}

void KCMKontactSummary::save()
{
    QStringList activeSummaries;

    QTreeWidgetItemIterator it(mPluginView);
    while (*it) {
        PluginItem *item = static_cast<PluginItem *>(*it);
        if (item->checkState(0) == Qt::Checked) {
            activeSummaries.append(item->pluginInfo().pluginName());
        }
        ++it;
    }

    KConfig config(QStringLiteral("kontact_summaryrc"));
    KConfigGroup grp(&config, QString());
    grp.writeEntry("ActiveSummaries", activeSummaries);
}

