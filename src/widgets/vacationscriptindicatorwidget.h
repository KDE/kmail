/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
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
    ~ServerLabel() override;

Q_SIGNALS:
    void clicked(const QString &serverName);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    const QString mServerName;
};

class VacationLabel : public QLabel
{
    Q_OBJECT
public:
    explicit VacationLabel(const QString &text, QWidget *parent = nullptr);
    ~VacationLabel() override;

Q_SIGNALS:
    void vacationLabelClicked();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
};

class VacationScriptIndicatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VacationScriptIndicatorWidget(QWidget *parent = nullptr);
    ~VacationScriptIndicatorWidget() override;

    void setVacationScriptActive(bool active, const QString &serverName);

    void updateIndicator();

    Q_REQUIRED_RESULT bool hasVacationScriptActive() const;

Q_SIGNALS:
    void clicked(const QString &serverName);

private:
    void slotVacationLabelClicked();
    void createIndicator();
    QStringList mServerActive;
    QHBoxLayout *mBoxLayout = nullptr;
    VacationLabel *mInfo = nullptr;
};
}

#endif // VACATIONSCRIPTINDICATORWIDGET_H
