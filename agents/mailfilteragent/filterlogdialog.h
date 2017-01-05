/*
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>
    Copyright (c) 2012-2017 Laurent Montel <montel@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/
#ifndef FILTERLOGDIALOG_H
#define FILTERLOGDIALOG_H

#include <QDialog>

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
class FilterLogDialog : public QDialog
{
    Q_OBJECT

public:
    /** constructor */
    explicit FilterLogDialog(QWidget *parent);
    ~FilterLogDialog();

private Q_SLOTS:
    void slotTextChanged();
    void slotLogEntryAdded(const QString &logEntry);
    void slotLogShrinked();
    void slotLogStateChanged();
    void slotChangeLogDetail();
    void slotSwitchLogState();
    void slotChangeLogMemLimit(int value);

    void slotUser1();
    void slotUser2();
private:
    void readConfig();
    void writeConfig();
private:
    KPIMTextEdit::PlainTextEditorWidget *mTextEdit;
    QCheckBox *mLogActiveBox;
    QGroupBox *mLogDetailsBox;
    QCheckBox *mLogPatternDescBox;
    QCheckBox *mLogRuleEvaluationBox;
    QCheckBox *mLogPatternResultBox;
    QCheckBox *mLogFilterActionBox;
    QSpinBox   *mLogMemLimitSpin;
    QPushButton *mUser1Button;
    QPushButton *mUser2Button;

    bool mIsInitialized;

};

#endif
