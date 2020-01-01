/*
   Copyright (C) 2018-2020 Laurent Montel <montel@kde.org>

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
#include "followupreminderinfoconfigwidget.h"
#include "followupreminderinfowidget.h"
#include "kmail-version.h"
#include <QLayout>
#include <KAboutData>
#include <KLocalizedString>
#include <KSharedConfig>
namespace {
static const char myConfigGroupName[] = "FollowUpReminderInfoDialog";
}

FollowUpReminderInfoConfigWidget::FollowUpReminderInfoConfigWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
{
    mWidget = new FollowUpReminderInfoWidget(parent);
    parent->layout()->addWidget(mWidget);

    KAboutData aboutData = KAboutData(
        QStringLiteral("followupreminderagent"),
        i18n("Follow Up Reminder Agent"),
        QStringLiteral(KDEPIM_VERSION),
        i18n("Follow Up Reminder"),
        KAboutLicense::GPL_V2,
        i18n("Copyright (C) 2014-2020 Laurent Montel"));

    aboutData.addAuthor(i18n("Laurent Montel"),
                        i18n("Maintainer"), QStringLiteral("montel@kde.org"));

    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"),
                            i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    setKAboutData(aboutData);
}

FollowUpReminderInfoConfigWidget::~FollowUpReminderInfoConfigWidget()
{
}

void FollowUpReminderInfoConfigWidget::load()
{
    mWidget->load();
}

bool FollowUpReminderInfoConfigWidget::save() const
{
    return mWidget->save();
}

QSize FollowUpReminderInfoConfigWidget::restoreDialogSize() const
{
    auto group = config()->group(myConfigGroupName);
    const QSize size = group.readEntry("Size", QSize(800, 600));
    return size;
}

void FollowUpReminderInfoConfigWidget::saveDialogSize(const QSize &size)
{
    auto group = config()->group(myConfigGroupName);
    group.writeEntry("Size", size);
}
