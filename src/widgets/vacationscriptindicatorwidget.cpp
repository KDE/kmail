/*
   Copyright (C) 2013-2016 Montel Laurent <montel@kde.org>

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

#include "vacationscriptindicatorwidget.h"
#include <QIcon>
#include <KLocalizedString>

#include <QHBoxLayout>

using namespace KMail;

ServerLabel::ServerLabel(const QString &serverName, QWidget *parent)
    : QLabel(parent),
      mServerName(serverName)
{
    setToolTip(serverName);
    setPixmap(QIcon::fromTheme(QStringLiteral("network-server")).pixmap(16, 16));
    setStyleSheet(QStringLiteral("background-color: %1;").arg(QColor(Qt::yellow).name()));
    setContentsMargins(2, 0, 4, 0);
}

ServerLabel::~ServerLabel()
{

}

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

VacationLabel::~VacationLabel()
{

}

void VacationLabel::mouseReleaseEvent(QMouseEvent *event)
{
    Q_EMIT vacationLabelClicked();
    QLabel::mouseReleaseEvent(event);
}

VacationScriptIndicatorWidget::VacationScriptIndicatorWidget(QWidget *parent)
    : QWidget(parent),
      mBoxLayout(Q_NULLPTR),
      mInfo(Q_NULLPTR)
{
}

VacationScriptIndicatorWidget::~VacationScriptIndicatorWidget()
{

}

void VacationScriptIndicatorWidget::setVacationScriptActive(bool active, const QString &serverName)
{
    if (active) {
        if (!mServerActive.contains(serverName)) {
            mServerActive.append(serverName);
            updateIndicator();
        }
    } else {
        if (mServerActive.contains(serverName)) {
            mServerActive.removeAll(serverName);
            updateIndicator();
        }
    }
}

void VacationScriptIndicatorWidget::createIndicator()
{
    delete mBoxLayout;
    mBoxLayout = new QHBoxLayout;
    mBoxLayout->setMargin(0);
    mBoxLayout->setSpacing(0);
    mInfo = new VacationLabel(i18np("Out of office reply active on server", "Out of office reply active on servers", mServerActive.count()));
    connect(mInfo, &VacationLabel::vacationLabelClicked, this, &VacationScriptIndicatorWidget::slotVacationLabelClicked);
    mBoxLayout->addWidget(mInfo);
    Q_FOREACH (const QString &server, mServerActive) {
        ServerLabel *lab = new ServerLabel(server);
        connect(lab, &ServerLabel::clicked, this, &VacationScriptIndicatorWidget::clicked);
        mBoxLayout->addWidget(lab);
    }
    setLayout(mBoxLayout);
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
