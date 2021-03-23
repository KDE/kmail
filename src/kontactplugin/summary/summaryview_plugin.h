/*
  This file is part of KDE Kontact.

  SPDX-FileCopyrightText: 2003 Sven Lüppken <sven@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KontactInterface/Plugin>

class SummaryViewPart;
class KSelectAction;
class KAboutData;
class SummaryView : public KontactInterface::Plugin
{
    Q_OBJECT

public:
    SummaryView(KontactInterface::Core *core, const QVariantList &);
    ~SummaryView() override;

    int weight() const override
    {
        return 100;
    }

    const KAboutData aboutData() override;

protected:
    KParts::Part *createPart() override;

private:
    void doSync();
    void syncAccount(QAction *act);
    void fillSyncActionSubEntries();
    SummaryViewPart *mPart = nullptr;
    KSelectAction *mSyncAction = nullptr;
    QAction *mAllSync = nullptr;
};

