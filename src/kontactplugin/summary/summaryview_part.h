/*
  This file is part of KDE Kontact.

  SPDX-FileCopyrightText: 2003 Sven Lüppken <sven@kde.org>
  SPDX-FileCopyrightText: 2003 Tobias König <tokoe@kde.org>
  SPDX-FileCopyrightText: 2003 Daniel Molkentin <molkentin@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KParts/Part>
#include <QDate>
#include <QMap>

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

class SummaryViewPart : public KParts::Part
{
    Q_OBJECT

public:
    SummaryViewPart(KontactInterface::Core *core, const KAboutData &aboutData, QObject *parent = nullptr);
    ~SummaryViewPart() override;

public Q_SLOTS:
    void slotTextChanged();
    void slotAdjustPalette();
    void setDate(QDate newDate);
    void updateSummaries();

Q_SIGNALS:
    void textChanged(const QString &);

protected:
    void partActivateEvent(KParts::PartActivateEvent *event) override;

protected Q_SLOTS:
    void slotConfigure();
    void updateWidgets();
    void summaryWidgetMoved(QWidget *target, QObject *obj, int alignment);

private:
    void initGUI(KontactInterface::Core *core);
    void loadLayout();
    void saveLayout();
    QString widgetName(QWidget *) const;

    void drawLtoR(QWidget *target, QWidget *widget, int alignment);
    void drawRtoL(QWidget *target, QWidget *widget, int alignment);

    QMap<QString, KontactInterface::Summary *> mSummaries;
    QStringList mLeftColumnSummaries;
    QStringList mRightColumnSummaries;
    KontactInterface::Core *mCore = nullptr;
    DropWidget *mFrame = nullptr;
    QFrame *mMainWidget = nullptr;
    QVBoxLayout *mMainLayout = nullptr;
    QVBoxLayout *mLeftColumn = nullptr;
    QVBoxLayout *mRightColumn = nullptr;
    QLabel *mUsernameLabel = nullptr;
    QLabel *mDateLabel = nullptr;
    QAction *mConfigAction = nullptr;
};

