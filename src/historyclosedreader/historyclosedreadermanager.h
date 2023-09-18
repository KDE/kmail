/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/
#pragma once

#include "historyclosedreaderinfo.h"

#include <QObject>

class HistoryClosedReaderManager : public QObject
{
    Q_OBJECT
public:
    explicit HistoryClosedReaderManager(QObject *parent = nullptr);
    ~HistoryClosedReaderManager() override;

    void addInfo(const HistoryClosedReaderInfo &info);

    Q_REQUIRED_RESULT HistoryClosedReaderInfo lastInfo() const;

private:
    QList<HistoryClosedReaderInfo> mClosedReaderInfos;
};
