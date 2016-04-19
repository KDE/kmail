/*
  This file is part of KDE Kontact.

  Copyright (C) 2003 Sven Lüppken <sven@kde.org>
  Copyright (C) 2003 Tobias König <tokoe@kde.org>
  Copyright (C) 2003 Daniel Molkentin <molkentin@kde.org>

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

#ifndef SUMMARYVIEW_PART_H
#define SUMMARYVIEW_PART_H

#include <KParts/ReadOnlyPart>
#include <kparts/readonlypart.h>

class DropWidget;

namespace KontactInterface
{
class Core;
class Summary;
}

class KAboutData;
class QAction;

class QFrame;
class QLabel;
class QVBoxLayout;

class SummaryViewPart : public KParts::ReadOnlyPart
{
    Q_OBJECT

public:
    SummaryViewPart(KontactInterface::Core *core,
                    const KAboutData &aboutData, QObject *parent = Q_NULLPTR);
    ~SummaryViewPart();

public Q_SLOTS:
    void slotTextChanged();
    void slotAdjustPalette();
    void setDate(const QDate &newDate);
    void updateSummaries();

Q_SIGNALS:
    void textChanged(const QString &);

protected:
    bool openFile() Q_DECL_OVERRIDE;
    void partActivateEvent(KParts::PartActivateEvent *event) Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void slotConfigure();
    void updateWidgets();
    void summaryWidgetMoved(QWidget *target, QObject *obj, int alignment);

private:
    void initGUI(KontactInterface::Core *core);
    void loadLayout();
    void saveLayout();
    QString widgetName(QWidget *) const;

    QStringList configModules() const;
    void drawLtoR(QWidget *target, QWidget *widget, int alignment);
    void drawRtoL(QWidget *target, QWidget *widget, int alignment);

    QMap<QString, KontactInterface::Summary *> mSummaries;
    QStringList mLeftColumnSummaries;
    QStringList mRightColumnSummaries;
    KontactInterface::Core *mCore;
    DropWidget *mFrame;
    QFrame *mMainWidget;
    QVBoxLayout *mMainLayout;
    QVBoxLayout *mLeftColumn;
    QVBoxLayout *mRightColumn;
    QLabel *mUsernameLabel;
    QLabel *mDateLabel;
    QAction *mConfigAction;

};

#endif
