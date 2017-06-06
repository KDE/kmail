/*
  This file is part of KDE Kontact.

  Copyright (C) 2003 Sven Lüppken <sven@kde.org>
  Copyright (C) 2003 Tobias König <tokoe@kde.org>
  Copyright (C) 2003 Daniel Molkentin <molkentin@kde.org>
  Copyright (C) 2008 Allen Winter <winter@kde.org>

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

#include "summaryview_part.h"
#include "dropwidget.h"

#include <Libkdepim/BroadcastStatus>
using KPIM::BroadcastStatus;

#include <KIdentityManagement/Identity>
#include <KIdentityManagement/IdentityManager>

#include <KontactInterface/Core>
#include <KontactInterface/Plugin>
#include <KontactInterface/Summary>

#include <QAction>
#include <KActionCollection>
#include <KCMultiDialog>
#include <KConfigGroup>
#include <QDialog>
#include <QIcon>
#include <KLocalizedString>
#include <KParts/PartActivateEvent>
#include <KConfig>
#include <QApplication>
#include <QDate>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QLocale>

SummaryViewPart::SummaryViewPart(KontactInterface::Core *core, const KAboutData &aboutData, QObject *parent)
    : KParts::ReadOnlyPart(parent)
    , mCore(core)
    , mFrame(nullptr)
    , mConfigAction(nullptr)
{
    Q_UNUSED(aboutData);
    loadLayout();

    initGUI(core);

    setDate(QDate::currentDate());
    connect(mCore, &KontactInterface::Core::dayChanged, this, &SummaryViewPart::setDate);

    mConfigAction = new QAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("&Configure Summary View..."), this);
    actionCollection()->addAction(QStringLiteral("summaryview_configure"), mConfigAction);
    connect(mConfigAction, &QAction::triggered, this, &SummaryViewPart::slotConfigure);
    const QString str = i18n("Configure the summary view");
    mConfigAction->setStatusTip(str);
    mConfigAction->setToolTip(str);
    mConfigAction->setWhatsThis(
        i18nc("@info:whatsthis",
              "Choosing this will show a dialog where you can select which "
              "summaries you want to see and also allow you to configure "
              "the summaries to your liking."));

    setXMLFile(QStringLiteral("kontactsummary_part.rc"));

    QTimer::singleShot(0, this, &SummaryViewPart::slotTextChanged);
}

SummaryViewPart::~SummaryViewPart()
{
    saveLayout();
}

bool SummaryViewPart::openFile()
{
    return true;
}

void SummaryViewPart::partActivateEvent(KParts::PartActivateEvent *event)
{
    // inform the plugins that the part has been activated so that they can
    // update the displayed information
    if (event->activated() && (event->part() == this)) {
        updateSummaries();
    }

    KParts::ReadOnlyPart::partActivateEvent(event);
}

void SummaryViewPart::updateSummaries()
{
    QMap<QString, KontactInterface::Summary *>::ConstIterator it;
    QMap<QString, KontactInterface::Summary *>::ConstIterator end(mSummaries.constEnd());
    for (it = mSummaries.constBegin(); it != end; ++it) {
        it.value()->updateSummary(false);
    }
}

void SummaryViewPart::updateWidgets()
{
    mMainWidget->setUpdatesEnabled(false);

    delete mFrame;

    KIdentityManagement::IdentityManager idm(true, this);
    const KIdentityManagement::Identity &id = idm.defaultIdentity();

    QString currentUser = i18n("Summary for %1", id.fullName());
    mUsernameLabel->setText(QStringLiteral("<b>%1</b>").arg(currentUser));

    mSummaries.clear();

    mFrame = new DropWidget(mMainWidget);
    connect(mFrame, &DropWidget::summaryWidgetDropped, this, &SummaryViewPart::summaryWidgetMoved);

    mMainLayout->insertWidget(2, mFrame);

    QStringList activeSummaries;

    KConfig config(QStringLiteral("kontact_summaryrc"));
    KConfigGroup grp(&config, QString());
    activeSummaries = grp.readEntry("ActiveSummaries", QStringList());
    if (grp.hasKey("ActiveSummaries")) {
        activeSummaries = grp.readEntry("ActiveSummaries", QStringList());
    } else {
        activeSummaries << QStringLiteral("kontact_korganizerplugin");
        activeSummaries << QStringLiteral("kontact_todoplugin");
        activeSummaries << QStringLiteral("kontact_specialdatesplugin");
        activeSummaries << QStringLiteral("kontact_kmailplugin");
        activeSummaries << QStringLiteral("kontact_knotesplugin");
    }

    // Collect all summary widgets with a summaryHeight > 0
    QStringList loadedSummaries;

    QList<KontactInterface::Plugin *> plugins = mCore->pluginList();
    QList<KontactInterface::Plugin *>::ConstIterator end = plugins.constEnd();
    QList<KontactInterface::Plugin *>::ConstIterator it = plugins.constBegin();
    for (; it != end; ++it) {
        KontactInterface::Plugin *plugin = *it;
        if (!activeSummaries.contains(plugin->identifier())) {
            continue;
        }

        KontactInterface::Summary *summary = plugin->createSummaryWidget(mFrame);
        if (summary) {
            if (summary->summaryHeight() > 0) {
                mSummaries.insert(plugin->identifier(), summary);

                connect(summary, &KontactInterface::Summary::message,
                        BroadcastStatus::instance(), &KPIM::BroadcastStatus::setStatusMsg);
                connect(summary, &KontactInterface::Summary::summaryWidgetDropped,
                        this, &SummaryViewPart::summaryWidgetMoved);

                if (!mLeftColumnSummaries.contains(plugin->identifier())
                    && !mRightColumnSummaries.contains(plugin->identifier())) {
                    mLeftColumnSummaries.append(plugin->identifier());
                }

                loadedSummaries.append(plugin->identifier());
            } else {
                summary->hide();
            }
        }
    }

    // Remove all unavailable summary widgets
    {
        QStringList::Iterator strIt;
        for (strIt = mLeftColumnSummaries.begin(); strIt != mLeftColumnSummaries.end(); ++strIt) {
            if (!loadedSummaries.contains(*strIt)) {
                strIt = mLeftColumnSummaries.erase(strIt);
                --strIt;
            }
        }
        for (strIt = mRightColumnSummaries.begin(); strIt != mRightColumnSummaries.end(); ++strIt) {
            if (!loadedSummaries.contains(*strIt)) {
                strIt = mRightColumnSummaries.erase(strIt);
                --strIt;
            }
        }
    }

    // Add vertical line between the two rows of summary widgets.
    QFrame *vline = new QFrame(mFrame);
    vline->setFrameStyle(QFrame::VLine | QFrame::Plain);

    QHBoxLayout *layout = new QHBoxLayout(mFrame);

    int margin = 20; // margin width: insert margins so there is space to dnd a
    // summary when either column is empty. looks nicer too.

    layout->insertSpacing(0, margin);
    mLeftColumn = new QVBoxLayout();
    layout->addLayout(mLeftColumn);
    layout->addWidget(vline);
    mRightColumn = new QVBoxLayout();
    layout->addLayout(mRightColumn);
    layout->insertSpacing(-1, margin);

    QStringList::ConstIterator strIt;
    QStringList::ConstIterator strEnd(mLeftColumnSummaries.constEnd());
    for (strIt = mLeftColumnSummaries.constBegin(); strIt != strEnd; ++strIt) {
        if (mSummaries.contains(*strIt)) {
            mLeftColumn->addWidget(mSummaries[ *strIt ]);
        }
    }
    strEnd = mRightColumnSummaries.constEnd();
    for (strIt = mRightColumnSummaries.constBegin(); strIt != strEnd; ++strIt) {
        if (mSummaries.contains(*strIt)) {
            mRightColumn->addWidget(mSummaries[ *strIt ]);
        }
    }

    QSpacerItem *lspacer = new QSpacerItem(1, 10,
                                           QSizePolicy::MinimumExpanding,
                                           QSizePolicy::MinimumExpanding);
    QSpacerItem *rspacer = new QSpacerItem(1, 10,
                                           QSizePolicy::MinimumExpanding,
                                           QSizePolicy::MinimumExpanding);
    mLeftColumn->addSpacerItem(lspacer);
    mRightColumn->addSpacerItem(rspacer);

    mFrame->show();

    mMainWidget->setUpdatesEnabled(true);
    mMainWidget->update();

    mLeftColumn->addStretch();
    mRightColumn->addStretch();
}

void SummaryViewPart::summaryWidgetMoved(QWidget *target, QObject *obj, int alignment)
{
    QWidget *widget = qobject_cast<QWidget *>(obj);
    if (!widget || (target == widget)) {
        return;
    }

    if (target == mFrame) {
        if (mLeftColumn->indexOf(widget) == -1 && mRightColumn->indexOf(widget) == -1) {
            return;
        }
    } else {
        if ((mLeftColumn->indexOf(target) == -1 && mRightColumn->indexOf(target) == -1)
            || (mLeftColumn->indexOf(widget) == -1 && mRightColumn->indexOf(widget) == -1)) {
            return;
        }
    }

    if (!QApplication::isRightToLeft()) {
        drawLtoR(target, widget, alignment);
    } else {
        drawRtoL(target, widget, alignment);
    }
}

void SummaryViewPart::drawLtoR(QWidget *target, QWidget *widget, int alignment)
{
    if (mLeftColumn->indexOf(widget) != -1) {
        mLeftColumn->removeWidget(widget);
        mLeftColumnSummaries.removeAll(widgetName(widget));
    } else if (mRightColumn->indexOf(widget) != -1) {
        mRightColumn->removeWidget(widget);
        mRightColumnSummaries.removeAll(widgetName(widget));
    }

    if (target == mFrame) {
        int pos = 0;

        if (alignment & Qt::AlignTop) {
            pos = 0;
        }

        if (alignment & Qt::AlignLeft) {
            if (alignment & Qt::AlignBottom) {
                pos = mLeftColumnSummaries.count();
            }

            mLeftColumn->insertWidget(pos, widget);
            mLeftColumnSummaries.insert(pos, widgetName(widget));
        } else {
            if (alignment & Qt::AlignBottom) {
                pos = mRightColumnSummaries.count();
            }

            mRightColumn->insertWidget(pos, widget);
            mRightColumnSummaries.insert(pos, widgetName(widget));
        }

        mFrame->updateGeometry();
        return;
    }

    int targetPos = mLeftColumn->indexOf(target);
    if (targetPos != -1) {
        if (alignment == Qt::AlignBottom) {
            targetPos++;
        }

        mLeftColumn->insertWidget(targetPos, widget);
        mLeftColumnSummaries.insert(targetPos, widgetName(widget));
    } else {
        targetPos = mRightColumn->indexOf(target);

        if (alignment == Qt::AlignBottom) {
            targetPos++;
        }

        mRightColumn->insertWidget(targetPos, widget);
        mRightColumnSummaries.insert(targetPos, widgetName(widget));
    }
    mFrame->updateGeometry();
}

void SummaryViewPart::drawRtoL(QWidget *target, QWidget *widget, int alignment)
{
    if (mRightColumn->indexOf(widget) != -1) {
        mRightColumn->removeWidget(widget);
        mRightColumnSummaries.removeAll(widgetName(widget));
    } else if (mLeftColumn->indexOf(widget) != -1) {
        mLeftColumn->removeWidget(widget);
        mLeftColumnSummaries.removeAll(widgetName(widget));
    }

    if (target == mFrame) {
        int pos = 0;

        if (alignment & Qt::AlignTop) {
            pos = 0;
        }

        if (alignment & Qt::AlignLeft) {
            if (alignment & Qt::AlignBottom) {
                pos = mRightColumnSummaries.count();
            }

            mRightColumn->insertWidget(pos, widget);
            mRightColumnSummaries.insert(pos, widgetName(widget));
        } else {
            if (alignment & Qt::AlignBottom) {
                pos = mLeftColumnSummaries.count();
            }

            mLeftColumn->insertWidget(pos, widget);
            mLeftColumnSummaries.insert(pos, widgetName(widget));
        }

        mFrame->updateGeometry();
        return;
    }

    int targetPos = mRightColumn->indexOf(target);
    if (targetPos != -1) {
        if (alignment == Qt::AlignBottom) {
            targetPos++;
        }

        mRightColumn->insertWidget(targetPos, widget);
        mRightColumnSummaries.insert(targetPos, widgetName(widget));
    } else {
        targetPos = mLeftColumn->indexOf(target);

        if (alignment == Qt::AlignBottom) {
            targetPos++;
        }

        mLeftColumn->insertWidget(targetPos, widget);
        mLeftColumnSummaries.insert(targetPos, widgetName(widget));
    }
    mFrame->updateGeometry();
}

void SummaryViewPart::slotTextChanged()
{
    Q_EMIT textChanged(i18n("What's next?"));
}

void SummaryViewPart::slotAdjustPalette()
{
    if (!QApplication::isRightToLeft()) {
        mMainWidget->setStyleSheet(
            QStringLiteral("#mMainWidget { "
                           " background: palette(base);"
                           " color: palette(text);"
                           " background-image: url(:/summaryview/kontact_bg.png);"
                           " background-position: bottom right;"
                           " background-repeat: no-repeat; }"
                           "QLabel { "
                           " color: palette(text); }"
                           "KUrlLabel { "
                           " color: palette(link); }"));
    } else {
        mMainWidget->setStyleSheet(
            QStringLiteral("#mMainWidget { "
                           " background: palette(base);"
                           " color: palette(text);"
                           " background-image: url(:/summaryview/kontact_bg.png);"
                           " background-position: bottom left;"
                           " background-repeat: no-repeat; }"
                           "QLabel { "
                           " color: palette(text); }"
                           "KUrlLabel { "
                           " color: palette(link); }"));
    }
}

void SummaryViewPart::setDate(const QDate &newDate)
{
    QString date(QStringLiteral("<b>%1</b>"));
    date = date.arg(QLocale().toString(newDate));
    mDateLabel->setText(date);
}

void SummaryViewPart::slotConfigure()
{
    KCMultiDialog dlg(mMainWidget);
    dlg.setObjectName(QStringLiteral("ConfigDialog"));
    dlg.setModal(true);

    QStringList modules = configModules();
    modules.prepend(QStringLiteral("kcmkontactsummary.desktop"));
    connect(&dlg, SIGNAL(configCommitted()),
            this, SLOT(updateWidgets()));

    QStringList::ConstIterator strIt;
    QStringList::ConstIterator end(modules.constEnd());
    for (strIt = modules.constBegin(); strIt != end; ++strIt) {
        dlg.addModule(*strIt);
    }

    dlg.exec();
}

QStringList SummaryViewPart::configModules() const
{
    QStringList modules;

    QMap<QString, KontactInterface::Summary *>::ConstIterator it;
    QMap<QString, KontactInterface::Summary *>::ConstIterator end(mSummaries.constEnd());
    for (it = mSummaries.constBegin(); it != end; ++it) {
        QStringList cm = it.value()->configModules();
        QStringList::ConstIterator strIt;
        QStringList::ConstIterator strEnd(cm.constEnd());
        for (strIt = cm.constBegin(); strIt != strEnd; ++strIt) {
            if (!(*strIt).isEmpty() && !modules.contains(*strIt)) {
                modules.append(*strIt);
            }
        }
    }

    return modules;
}

void SummaryViewPart::initGUI(KontactInterface::Core *core)
{
    QScrollArea *sa = new QScrollArea(core);

    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sa->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    sa->setWidgetResizable(true);

    mMainWidget = new QFrame;
    mMainWidget->setObjectName(QStringLiteral("mMainWidget"));
    sa->setWidget(mMainWidget);
    mMainWidget->setFocusPolicy(Qt::StrongFocus);
    setWidget(sa);

    //KF5: port it
    //connect(KGlobalSettings::self(), &KGlobalSettings::kdisplayPaletteChanged, this, &SummaryViewPart::slotAdjustPalette);
    slotAdjustPalette();

    mMainLayout = new QVBoxLayout(mMainWidget);

    QHBoxLayout *hbl = new QHBoxLayout();
    mMainLayout->addItem(hbl);
    mUsernameLabel = new QLabel(mMainWidget);
    mDateLabel = new QLabel(mMainWidget);
    if (!QApplication::isRightToLeft()) {
        mUsernameLabel->setAlignment(Qt::AlignLeft);
        hbl->addWidget(mUsernameLabel);
        mDateLabel->setAlignment(Qt::AlignRight);
        hbl->addWidget(mDateLabel);
    } else {
        mDateLabel->setAlignment(Qt::AlignRight);
        hbl->addWidget(mDateLabel);
        mUsernameLabel->setAlignment(Qt::AlignLeft);
        hbl->addWidget(mUsernameLabel);
    }

    QFrame *hline = new QFrame(mMainWidget);
    hline->setFrameStyle(QFrame::HLine | QFrame::Plain);
    mMainLayout->insertWidget(1, hline);

    mFrame = new DropWidget(mMainWidget);
    mMainLayout->insertWidget(2, mFrame);

    connect(mFrame, &DropWidget::summaryWidgetDropped, this, &SummaryViewPart::summaryWidgetMoved);

    updateWidgets();
}

void SummaryViewPart::loadLayout()
{
    KConfig config(QStringLiteral("kontact_summaryrc"));
    KConfigGroup grp(&config, QString());

    if (grp.hasKey("LeftColumnSummaries")) {
        mLeftColumnSummaries = grp.readEntry("LeftColumnSummaries", QStringList());
    } else {
        mLeftColumnSummaries << QStringLiteral("kontact_korganizerplugin");
        mLeftColumnSummaries << QStringLiteral("kontact_todoplugin");
        mLeftColumnSummaries << QStringLiteral("kontact_specialdatesplugin");
    }

    if (grp.hasKey("RightColumnSummaries")) {
        mRightColumnSummaries = grp.readEntry("RightColumnSummaries", QStringList());
    } else {
        mRightColumnSummaries << QStringLiteral("kontact_kmailplugin");
        mRightColumnSummaries << QStringLiteral("kontact_knotesplugin");
    }
}

void SummaryViewPart::saveLayout()
{
    KConfig config(QStringLiteral("kontact_summaryrc"));
    KConfigGroup grp(&config, QString());

    grp.writeEntry("LeftColumnSummaries", mLeftColumnSummaries);
    grp.writeEntry("RightColumnSummaries", mRightColumnSummaries);

    config.sync();
}

QString SummaryViewPart::widgetName(QWidget *widget) const
{
    QMap<QString, KontactInterface::Summary *>::ConstIterator it;
    QMap<QString, KontactInterface::Summary *>::ConstIterator end(mSummaries.constEnd());
    for (it = mSummaries.constBegin(); it != end; ++it) {
        if (it.value() == widget) {
            return it.key();
        }
    }

    return QString();
}
