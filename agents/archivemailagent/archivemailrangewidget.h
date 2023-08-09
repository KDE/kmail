/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QWidget>
class KTimeComboBox;
class QCheckBox;
class ArchiveMailRangeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ArchiveMailRangeWidget(QWidget *parent = nullptr);
    ~ArchiveMailRangeWidget() override;

    Q_REQUIRED_RESULT bool isEnabled() const;
    void setEnabled(bool isEnabled);

    Q_REQUIRED_RESULT QList<int> range() const;
    void setRange(const QList<int> &hours);

private:
    KTimeComboBox *const mStartRange;
    KTimeComboBox *const mEndRange;
    QCheckBox *const mEnabled;
};
