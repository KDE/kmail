/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "vacationscriptindicatorwidget.h"
#include <KIcon>
#include <KLocalizedString>

#include <QHBoxLayout>

using namespace KMail;

ServerLabel::ServerLabel(const QString &serverName, QWidget * parent)
    : QLabel( parent )
{
    setToolTip(serverName);
    setPixmap(KIcon( QLatin1String("network-server") ).pixmap( 16, 16 ) );
    setStyleSheet( QString::fromLatin1("background-color: %1;" ).arg( QColor(Qt::yellow).name() ) );
}

ServerLabel::~ServerLabel()
{

}

void ServerLabel::mouseReleaseEvent(QMouseEvent * event)
{
    Q_EMIT clicked();
    QLabel::mouseReleaseEvent( event );
}


VacationLabel::VacationLabel(const QString &text, QWidget * parent)
    : QLabel( text, parent )
{
    // changing the palette doesn't work, seems to be overwriten by the
    // statusbar again, stylesheets seems to work though
    setStyleSheet( QString::fromLatin1("background-color: %1;" ).arg( QColor(Qt::yellow).name() ) );
    setCursor( QCursor( Qt::PointingHandCursor ) );
}

VacationLabel::~VacationLabel()
{

}

void VacationLabel::mouseReleaseEvent(QMouseEvent * event)
{
    Q_EMIT clicked();
    QLabel::mouseReleaseEvent( event );
}

VacationScriptIndicatorWidget::VacationScriptIndicatorWidget(QWidget *parent)
    : QWidget(parent),
      mBoxLayout(0),
      mInfo(0)
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
    mInfo = new VacationLabel(i18n("Out of office reply active on server"));
    connect(mInfo, SIGNAL(clicked()), this, SIGNAL(clicked()));
    mBoxLayout->addWidget(mInfo);
    Q_FOREACH (const QString &server, mServerActive) {
        ServerLabel *lab = new ServerLabel(server);
        connect(lab, SIGNAL(clicked()), this, SIGNAL(clicked()));
        mBoxLayout->addWidget(lab);
    }
    setLayout(mBoxLayout);
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
