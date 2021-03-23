/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
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
    ~RefreshSettingsAssistant() override;

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
