/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "vacationscriptindicatorwidget.h"

#include "util.h"
#include <KLocalizedString>
#include <QIcon>

#include <QHBoxLayout>

using namespace KMail;

ServerLabel::ServerLabel(const QString &serverName, QWidget *parent)
    : QLabel(parent)
    , mServerName(serverName)
{
    setToolTip(serverName);
    setPixmap(QIcon::fromTheme(QStringLiteral("network-server")).pixmap(16, 16));
    setStyleSheet(QStringLiteral("background-color: %1;").arg(QColor(Qt::yellow).name()));
    setContentsMargins(2, 0, 4, 0);
}

ServerLabel::~ServerLabel() = default;

void ServerLabel::mouseReleaseEvent(QMouseEvent *event)
{
    Q_EMIT clicked(mServerName);
    QLabel::mouseReleaseEvent(event);
}

VacationLabel::VacationLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    // changing the palette doesn't work, seems to be overwriten by the
    // statusbar again, stylesheets seems to work though
    setStyleSheet(QStringLiteral("background-color: %1; color: %2;").arg(QColor(Qt::yellow).name(), QColor(Qt::black).name()));
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
    if (serverName.isEmpty()) {
        return;
    }

    if (active) {
        if (!mServerActive.contains(serverName)) {
            mServerActive.append(serverName);
            updateIndicator();
        }
    } else {
        int countRemoveServerName = mServerActive.removeAll(serverName);
        if (countRemoveServerName > 0) {
            updateIndicator();
        }
    }
}

void VacationScriptIndicatorWidget::createIndicator()
{
    delete mBoxLayout;
    mBoxLayout = new QHBoxLayout(this);
    mBoxLayout->setContentsMargins({});
    mBoxLayout->setSpacing(0);
    mInfo = new VacationLabel(i18np("Out of office reply active on server", "Out of office reply active on servers", mServerActive.count()));
    connect(mInfo, &VacationLabel::vacationLabelClicked, this, &VacationScriptIndicatorWidget::slotVacationLabelClicked);
    mBoxLayout->addWidget(mInfo);
    for (const QString &server : std::as_const(mServerActive)) {
        auto lab = new ServerLabel(server);
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
