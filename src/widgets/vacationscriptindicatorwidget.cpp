/*
   SPDX-FileCopyrightText: 2013-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "vacationscriptindicatorwidget.h"

#include <KLocalizedString>
#include <QEvent>
#include <QIcon>
#include <QStyle>

#include <QHBoxLayout>

using namespace KMail;
using namespace Qt::Literals::StringLiterals;

ServerLabel::ServerLabel(const QString &serverName, QWidget *parent)
    : QLabel(parent)
    , mServerName(serverName)
{
    setToolTip(serverName);
    updateIcon();
    setStyleSheet(u"background-color: %1; color: %2;"_s.arg(QColor(Qt::yellow).name(), QColor(Qt::black).name()));
    setContentsMargins(2, 0, 4, 0);
}

ServerLabel::~ServerLabel() = default;

void ServerLabel::updateIcon()
{
    const int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize);
    setPixmap(QIcon::fromTheme(u"network-server"_s).pixmap(QSize(iconSize, iconSize), devicePixelRatioF()));
}

bool ServerLabel::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::DevicePixelRatioChange: // moved to a screen with a different scale factor
    case QEvent::StyleChange: // PM_SmallIconSize may have changed
        updateIcon();
        break;
    default:
        break;
    }
    return QLabel::event(e);
}

void ServerLabel::mouseReleaseEvent(QMouseEvent *event)
{
    Q_EMIT clicked(mServerName);
    QLabel::mouseReleaseEvent(event);
}

VacationLabel::VacationLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    // changing the palette doesn't work, seems to be overwritten by the
    // statusbar again, stylesheets seems to work though
    setStyleSheet(u"background-color: %1; color: %2;"_s.arg(QColor(Qt::yellow).name(), QColor(Qt::black).name()));
    setContentsMargins(4, 0, 2, 0);
    setCursor(QCursor(Qt::PointingHandCursor));
}

VacationLabel::~VacationLabel() = default;

void VacationLabel::mouseReleaseEvent(QMouseEvent *event)
{
    Q_EMIT vacationLabelClicked();
    QLabel::mouseReleaseEvent(event);
}

VacationScriptIndicatorWidget::VacationScriptIndicatorWidget(QWidget *parent)
    : QWidget(parent)
{
}

VacationScriptIndicatorWidget::~VacationScriptIndicatorWidget() = default;

void VacationScriptIndicatorWidget::setVacationScriptActive(bool active, const QString &serverName)
{
    if (!active && serverName.isEmpty()) { // reset global
        if (!mServerActive.isEmpty()) {
            mServerActive.clear();
            updateIndicator();
        }
        return;
    }
    if (serverName.isEmpty()) {
        return;
    }

    if (active) {
        if (!mServerActive.contains(serverName)) {
            mServerActive.append(serverName);
            updateIndicator();
        }
    } else {
        if (int countRemoveServerName = mServerActive.removeAll(serverName); countRemoveServerName > 0) {
            updateIndicator();
        }
    }
}

void VacationScriptIndicatorWidget::clearIndicator()
{
    // Deleting the layout doesn't delete the widgets it manages: they would stay
    // as (unmanaged) children of this widget and overlap the newly created ones.
    if (mBoxLayout) {
        while (QLayoutItem *item = mBoxLayout->takeAt(0)) {
            delete item->widget();
            delete item;
        }
        delete mBoxLayout;
        mBoxLayout = nullptr;
    }
    mInfo = nullptr;
}

void VacationScriptIndicatorWidget::createIndicator()
{
    clearIndicator();
    mBoxLayout = new QHBoxLayout(this);
    mBoxLayout->setContentsMargins({});
    mBoxLayout->setSpacing(0);
    mInfo = new VacationLabel(i18np("Out of office reply active on server", "Out of office reply active on %1 servers", mServerActive.count()), this);
    connect(mInfo, &VacationLabel::vacationLabelClicked, this, &VacationScriptIndicatorWidget::slotVacationLabelClicked);
    mBoxLayout->addWidget(mInfo);
    for (const QString &server : std::as_const(mServerActive)) {
        auto lab = new ServerLabel(server, this);
        connect(lab, &ServerLabel::clicked, this, &VacationScriptIndicatorWidget::clicked);
        mBoxLayout->addWidget(lab);
    }
}

void VacationScriptIndicatorWidget::slotVacationLabelClicked()
{
    Q_EMIT clicked(QString());
}

void VacationScriptIndicatorWidget::updateIndicator()
{
    if (mServerActive.isEmpty()) {
        hide();
    } else {
        createIndicator();
        show();
    }
}

bool VacationScriptIndicatorWidget::hasVacationScriptActive() const
{
    return !mServerActive.isEmpty();
}

#include "moc_vacationscriptindicatorwidget.cpp"
