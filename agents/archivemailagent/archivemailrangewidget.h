/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QWidget>
class HourComboBox;
class QCheckBox;
class ArchiveMailRangeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ArchiveMailRangeWidget(QWidget *parent = nullptr);
    ~ArchiveMailRangeWidget() override;

    [[nodiscard]] bool isRangeEnabled() const;
    void setRangeEnabled(bool isEnabled);

    [[nodiscard]] QList<int> range() const;
    void setRange(const QList<int> &hours);

private:
    void changeRangeState(bool enabled);
    HourComboBox *const mStartRange;
    HourComboBox *const mEndRange;
    QCheckBox *const mRangeEnabled;
};
