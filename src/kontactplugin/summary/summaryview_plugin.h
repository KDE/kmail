/*
  This file is part of KDE Kontact.

  Copyright (C) 2003 Sven L�ppken <sven@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef SUMMARYVIEW_PLUGIN_H
#define SUMMARYVIEW_PLUGIN_H

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
    KParts::ReadOnlyPart *createPart() override;

private:
    void doSync();
    void syncAccount(QAction *act);
    void fillSyncActionSubEntries();
    SummaryViewPart *mPart = nullptr;
    KSelectAction *mSyncAction = nullptr;
    QAction *mAllSync = nullptr;
};

#endif
