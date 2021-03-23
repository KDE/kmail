/*
    SPDX-FileCopyrightText: 2003 Andreas Gungl <a.gungl@gmx.de>
    SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/
#pragma once

#include <QDialog>

#include <KPIMTextEdit/PlainTextEditor>

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
namespace KPIMTextEdit
{
class PlainTextEditorWidget;
}

class FilterLogTextEdit : public KPIMTextEdit::PlainTextEditor
{
    Q_OBJECT
public:
    explicit FilterLogTextEdit(QWidget *parent = nullptr);
    ~FilterLogTextEdit() override = default;

protected:
    void addExtraMenuEntry(QMenu *menu, QPoint pos) override;
};

class FilterLogDialog : public QDialog
{
    Q_OBJECT

public:
    /** constructor */
    explicit FilterLogDialog(QWidget *parent);
    ~FilterLogDialog() override;

private:
    void slotTextChanged();
    void slotLogEntryAdded(const QString &logEntry);
    void slotLogShrinked();
    void slotLogStateChanged();
    void slotChangeLogDetail();
    void slotSwitchLogState();
    void slotChangeLogMemLimit(int value);

    void slotUser1();
    void slotUser2();

    void readConfig();
    void writeConfig();

    KPIMTextEdit::PlainTextEditorWidget *mTextEdit = nullptr;
    QCheckBox *mLogActiveBox = nullptr;
    QGroupBox *mLogDetailsBox = nullptr;
    QCheckBox *mLogPatternDescBox = nullptr;
    QCheckBox *mLogRuleEvaluationBox = nullptr;
    QCheckBox *mLogPatternResultBox = nullptr;
    QCheckBox *mLogFilterActionBox = nullptr;
    QSpinBox *mLogMemLimitSpin = nullptr;
    QPushButton *mUser1Button = nullptr;
    QPushButton *mUser2Button = nullptr;

    bool mIsInitialized = false;
};

