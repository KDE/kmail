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

#ifndef FOLLOWUPREMINDERINFOCONFIGWIDGET_H
#define FOLLOWUPREMINDERINFOCONFIGWIDGET_H

#include <QWidget>
#include <KConfigGroup>
#include <QTreeWidgetItem>
#include <QVariantList>
#include <AkonadiCore/Item>
#include <AkonadiCore/AgentConfigurationBase>
class QTreeWidget;
class FollowUpReminderInfoWidget;
namespace FollowUpReminder {
class FollowUpReminderInfo;
}

class FollowUpReminderInfoConfigWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit FollowUpReminderInfoConfigWidget(const KSharedConfigPtr &config, QWidget *parentWidget, const QVariantList &args);
    ~FollowUpReminderInfoConfigWidget() override;

    bool save() const override;
    void load() override;
private:
    FollowUpReminderInfoWidget *mWidget = nullptr;
};
AKONADI_AGENTCONFIG_FACTORY(FollowUpReminderInfoAgentConfigFactory, "followupreminderagentconfig.json", FollowUpReminderInfoConfigWidget)
#endif // FOLLOWUPREMINDERINFOConfigWIDGET_H
