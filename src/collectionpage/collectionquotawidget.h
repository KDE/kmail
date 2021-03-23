/*
 * folderdialogquotatab_p.h
 *
 * SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <QWidget>

class QProgressBar;
class QLabel;

class CollectionQuotaWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CollectionQuotaWidget(QWidget *parent);
    ~CollectionQuotaWidget() override = default;

    void setQuotaInfo(qint64 currentValue, qint64 maxValue);

private:
    QProgressBar *const mProgressBar;
    QLabel *const mUsage;
};

