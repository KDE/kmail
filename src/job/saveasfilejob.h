/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/
#pragma once

#include <QObject>
class QTextDocument;
class SaveAsFileJob : public QObject
{
    Q_OBJECT
public:
    explicit SaveAsFileJob(QObject *parent = nullptr);
    ~SaveAsFileJob() override;
    void start();

    void setHtmlMode(bool htmlMode);

    void setTextDocument(QTextDocument *textDocument);

    void setParentWidget(QWidget *parentWidget);

private:
    Q_DISABLE_COPY(SaveAsFileJob)
    bool mHtmlMode = false;
    QTextDocument *mTextDocument = nullptr;
    QWidget *mParentWidget = nullptr;
};

