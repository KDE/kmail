/*
   Copyright (C) 2019 Montel Laurent <montel@kde.org>

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

#ifndef REFRESHSETTINGSASSISTANT_H
#define REFRESHSETTINGSASSISTANT_H

#include <KAssistantDialog>
class RefreshSettingsCleanupPage;
class RefreshSettingsFirstPage;
class RefreshSettringsFinishPage;
class RefreshSettingsAssistant : public KAssistantDialog
{
    Q_OBJECT
public:
    explicit RefreshSettingsAssistant(QWidget *parent = nullptr);
    ~RefreshSettingsAssistant();

private:
    void initializePages();
    void cleanUpDone();
    KPageWidgetItem *mCleanUpPageItem = nullptr;
    RefreshSettingsCleanupPage *mCleanUpPage = nullptr;

    KPageWidgetItem *mFirstPageItem = nullptr;
    RefreshSettingsFirstPage *mFirstPage = nullptr;

    KPageWidgetItem *mFinishPageItem = nullptr;
    RefreshSettringsFinishPage *mFinishPage = nullptr;
};

#endif // REFRESHSETTINGSASSISTANT_H
