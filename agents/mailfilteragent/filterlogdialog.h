/*
    SPDX-FileCopyrightText: 2003 Andreas Gungl <a.gungl@gmx.de>
    SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/
#pragma once

#include <QDialog>

#include "mailfilteragentprivate_export.h"
#include <TextCustomEditor/PlainTextEditor>

class QCheckBox;
class QSpinBox;
class QGroupBox;
class QPushButton;
/**
  @short KMail Filter Log Collector.
  @author Andreas Gungl <a.gungl@gmx.de>

  The filter log dialog allows a continued observation of the
  filter log of MailFilterAgent.
*/
namespace TextCustomEditor
{
class PlainTextEditorWidget;
}
class FilterLogTextEdit : public TextCustomEditor::PlainTextEditor
{
    Q_OBJECT
public:
    explicit FilterLogTextEdit(QWidget *parent = nullptr);
    ~FilterLogTextEdit() override = default;

protected:
    void addExtraMenuEntry(QMenu *menu, QPoint pos) override;
};

class MAILFILTERAGENTPRIVATE_EXPORT FilterLogDialog : public QDialog
{
    Q_OBJECT

public:
    /** constructor */
    explicit FilterLogDialog(QWidget *parent);
    ~FilterLogDialog() override;

private:
    MAILFILTERAGENTPRIVATE_NO_EXPORT void slotTextChanged();
    MAILFILTERAGENTPRIVATE_NO_EXPORT void slotLogEntryAdded(const QString &logEntry);
    MAILFILTERAGENTPRIVATE_NO_EXPORT void slotLogShrinked();
    MAILFILTERAGENTPRIVATE_NO_EXPORT void slotLogStateChanged();
    MAILFILTERAGENTPRIVATE_NO_EXPORT void slotChangeLogDetail();
    MAILFILTERAGENTPRIVATE_NO_EXPORT void slotSwitchLogState();
    MAILFILTERAGENTPRIVATE_NO_EXPORT void slotChangeLogMemLimit(int value);

    MAILFILTERAGENTPRIVATE_NO_EXPORT void slotUser1();
    MAILFILTERAGENTPRIVATE_NO_EXPORT void slotUser2();

    MAILFILTERAGENTPRIVATE_NO_EXPORT void readConfig();
    MAILFILTERAGENTPRIVATE_NO_EXPORT void writeConfig();

    TextCustomEditor::PlainTextEditorWidget *mTextEdit = nullptr;
    QCheckBox *mLogActiveBox = nullptr;
    QGroupBox *mLogDetailsBox = nullptr;
    QCheckBox *mLogPatternDescBox = nullptr;
    QCheckBox *mLogRuleEvaluationBox = nullptr;
    QCheckBox *mLogPatternResultBox = nullptr;
    QCheckBox *mLogFilterActionBox = nullptr;
    QSpinBox *mLogMemLimitSpin = nullptr;
    QPushButton *const mUser1Button;
    QPushButton *const mUser2Button;
    QPushButton *const mShareButton;
    bool mIsInitialized = false;
};
