/*
   Copyright (C) 2018 Montel Laurent <montel@kde.org>

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

#include <QHBoxLayout>
#include <QHeaderView>
#include <KAboutData>
#include <QLocale>
#include <QIcon>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KMessageBox>


FollowUpReminderInfoConfigWidget::FollowUpReminderInfoConfigWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
{
    QWidget *w = new QWidget(parent);
    parent->layout()->addWidget(w);
    mWidget = new FollowUpReminderInfoWidget(w);

    KAboutData aboutData = KAboutData(
        QStringLiteral("followupreminderagent"),
        i18n("Follow Up Reminder Agent"),
        QStringLiteral(KDEPIM_VERSION),
        i18n("Follow Up Reminder"),
        KAboutLicense::GPL_V2,
        i18n("Copyright (C) 2014-2018 Laurent Montel"));

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

