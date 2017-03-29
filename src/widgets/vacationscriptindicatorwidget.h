/*
   Copyright (C) 2013-2017 Montel Laurent <montel@kde.org>

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

#ifndef VACATIONSCRIPTINDICATORWIDGET_H
#define VACATIONSCRIPTINDICATORWIDGET_H

#include <QLabel>
class QHBoxLayout;
class QLabel;
namespace KMail
{

class ServerLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ServerLabel(const QString &toolTip, QWidget *parent = nullptr);
    ~ServerLabel();

Q_SIGNALS:
    void clicked(const QString &serverName);

protected:
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    QString mServerName;
};

class VacationLabel : public QLabel
{
    Q_OBJECT
public:
    explicit VacationLabel(const QString &text, QWidget *parent = nullptr);
    ~VacationLabel();

Q_SIGNALS:
    void vacationLabelClicked();

protected:
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
};

class VacationScriptIndicatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VacationScriptIndicatorWidget(QWidget *parent = nullptr);
    ~VacationScriptIndicatorWidget();

    void setVacationScriptActive(bool active, const QString &serverName);

    void updateIndicator();

    bool hasVacationScriptActive() const;

Q_SIGNALS:
    void clicked(const QString &serverName);

private Q_SLOTS:
    void slotVacationLabelClicked();
private:
    void createIndicator();
    QStringList mServerActive;
    QHBoxLayout *mBoxLayout;
    VacationLabel *mInfo;
};
}

#endif // VACATIONSCRIPTINDICATORWIDGET_H
