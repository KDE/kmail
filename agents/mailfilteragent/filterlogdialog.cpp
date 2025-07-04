/*
    SPDX-FileCopyrightText: 2003 Andreas Gungl <a.gungl@gmx.de>
    SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#include "filterlogdialog.h"
#include "mailfilterpurposemenuwidget.h"
#include <MailCommon/FilterLog>
#include <PimCommon/PurposeMenuMessageWidget>
#include <TextCustomEditor/PlainTextEditorWidget>

#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardActions>
#include <QFileDialog>
#include <QIcon>
#include <QVBoxLayout>

#include <QAction>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QSpinBox>
#include <QStringList>

#include <KConfigGroup>
#include <KGuiItem>
#include <KSharedConfig>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <cerrno>

using namespace MailCommon;

FilterLogDialog::FilterLogDialog(QWidget *parent)
    : QDialog(parent)
    , mUser1Button(new QPushButton(this))
    , mUser2Button(new QPushButton(this))
    , mShareButton(new QPushButton(i18nc("@action:button", "Share…"), this))
{
    setWindowTitle(i18nc("@title:window", "Filter Log Viewer"));
    auto mainLayout = new QVBoxLayout(this);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonBox->addButton(mUser1Button, QDialogButtonBox::ActionRole);
    buttonBox->addButton(mUser2Button, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &FilterLogDialog::reject);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("view-filter")));
    setModal(false);

    buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
    KGuiItem::assign(mUser1Button, KStandardGuiItem::clear());
    KGuiItem::assign(mUser2Button, KStandardGuiItem::saveAs());
    auto page = new QFrame(this);

    auto pageVBoxLayout = new QVBoxLayout;
    page->setLayout(pageVBoxLayout);
    pageVBoxLayout->setContentsMargins({});
    mainLayout->addWidget(page);

    auto purposeMenuMessageWidget = new PimCommon::PurposeMenuMessageWidget(this);
    pageVBoxLayout->addWidget(purposeMenuMessageWidget);

    mLogActiveBox = new QCheckBox(i18nc("@option:check", "&Log filter activities"), page);
    pageVBoxLayout->addWidget(mLogActiveBox);
    mLogActiveBox->setChecked(FilterLog::instance()->isLogging());
    connect(mLogActiveBox, &QCheckBox::clicked, this, &FilterLogDialog::slotSwitchLogState);
    mLogActiveBox->setWhatsThis(
        i18n("You can turn logging of filter activities on and off here. "
             "Of course, log data is collected and shown only when logging "
             "is turned on. "));

    mTextEdit = new TextCustomEditor::PlainTextEditorWidget(new FilterLogTextEdit(this), page);
    pageVBoxLayout->addWidget(mTextEdit);

    mTextEdit->setReadOnly(true);
    mTextEdit->editor()->setWordWrapMode(QTextOption::NoWrap);
    const QStringList logEntries = FilterLog::instance()->logEntries();
    const QString log = logEntries.join(QStringLiteral("<br>"));
    mTextEdit->editor()->appendHtml(log);

    auto purposeMenu = new MailfilterPurposeMenuWidget(this, this);
    connect(purposeMenu, &MailfilterPurposeMenuWidget::shareError, purposeMenuMessageWidget, &PimCommon::PurposeMenuMessageWidget::slotShareError);
    connect(purposeMenu, &MailfilterPurposeMenuWidget::shareSuccess, purposeMenuMessageWidget, &PimCommon::PurposeMenuMessageWidget::slotShareSuccess);
    mShareButton->setMenu(purposeMenu->menu());
    mShareButton->setIcon(QIcon::fromTheme(QStringLiteral("document-share")));
    purposeMenu->setEditorWidget(mTextEdit->editor());
    buttonBox->addButton(mShareButton, QDialogButtonBox::ActionRole);

    mLogDetailsBox = new QGroupBox(i18n("Logging Details"), page);
    pageVBoxLayout->addWidget(mLogDetailsBox);
    auto layout = new QVBoxLayout;
    mLogDetailsBox->setLayout(layout);
    mLogDetailsBox->setEnabled(mLogActiveBox->isChecked());
    connect(mLogActiveBox, &QCheckBox::toggled, mLogDetailsBox, &QGroupBox::setEnabled);

    mLogPatternDescBox = new QCheckBox(i18nc("@option:check", "Log pattern description"), mLogDetailsBox);
    layout->addWidget(mLogPatternDescBox);
    mLogPatternDescBox->setChecked(FilterLog::instance()->isContentTypeEnabled(FilterLog::PatternDescription));
    connect(mLogPatternDescBox, &QCheckBox::clicked, this, &FilterLogDialog::slotChangeLogDetail);

    mLogRuleEvaluationBox = new QCheckBox(i18nc("@option:check", "Log filter &rule evaluation"), mLogDetailsBox);
    layout->addWidget(mLogRuleEvaluationBox);
    mLogRuleEvaluationBox->setChecked(FilterLog::instance()->isContentTypeEnabled(FilterLog::RuleResult));
    connect(mLogRuleEvaluationBox, &QCheckBox::clicked, this, &FilterLogDialog::slotChangeLogDetail);
    mLogRuleEvaluationBox->setWhatsThis(
        i18n("You can control the feedback in the log concerning the "
             "evaluation of the filter rules of applied filters: "
             "having this option checked will give detailed feedback "
             "for each single filter rule; alternatively, only "
             "feedback about the result of the evaluation of all rules "
             "of a single filter will be given."));

    mLogPatternResultBox = new QCheckBox(i18nc("@option:check", "Log filter pattern evaluation"), mLogDetailsBox);
    layout->addWidget(mLogPatternResultBox);
    mLogPatternResultBox->setChecked(FilterLog::instance()->isContentTypeEnabled(FilterLog::PatternResult));
    connect(mLogPatternResultBox, &QCheckBox::clicked, this, &FilterLogDialog::slotChangeLogDetail);

    mLogFilterActionBox = new QCheckBox(i18nc("@option:check", "Log filter actions"), mLogDetailsBox);
    layout->addWidget(mLogFilterActionBox);
    mLogFilterActionBox->setChecked(FilterLog::instance()->isContentTypeEnabled(FilterLog::AppliedAction));
    connect(mLogFilterActionBox, &QCheckBox::clicked, this, &FilterLogDialog::slotChangeLogDetail);

    auto hboxWidget = new QWidget(page);
    auto hboxHBoxLayout = new QHBoxLayout;
    hboxWidget->setLayout(hboxHBoxLayout);
    hboxHBoxLayout->setContentsMargins({});
    pageVBoxLayout->addWidget(hboxWidget);
    auto logSizeLab = new QLabel(i18nc("@label:textbox", "Log size limit:"), hboxWidget);
    hboxHBoxLayout->addWidget(logSizeLab);
    mLogMemLimitSpin = new QSpinBox(hboxWidget);
    hboxHBoxLayout->addWidget(mLogMemLimitSpin, 1);
    mLogMemLimitSpin->setMinimum(1);
    mLogMemLimitSpin->setMaximum(1024 * 256); // 256 MB
    // value in the QSpinBox is in KB while it's in Byte in the FilterLog
    mLogMemLimitSpin->setValue(FilterLog::instance()->maxLogSize() / 1024);
    mLogMemLimitSpin->setSuffix(i18n(" KB"));
    mLogMemLimitSpin->setSpecialValueText(i18nc("@label:spinbox Set the size of the logfile to unlimited.", "unlimited"));
    connect(mLogMemLimitSpin, &QSpinBox::valueChanged, this, &FilterLogDialog::slotChangeLogMemLimit);
    mLogMemLimitSpin->setWhatsThis(
        i18n("Collecting log data uses memory to temporarily store the "
             "log data; here you can limit the maximum amount of memory "
             "to be used: if the size of the collected log data exceeds "
             "this limit then the oldest data will be discarded until "
             "the limit is no longer exceeded. "));

    connect(mLogActiveBox, &QCheckBox::toggled, mLogMemLimitSpin, &QSpinBox::setEnabled);
    mLogMemLimitSpin->setEnabled(mLogActiveBox->isChecked());

    connect(FilterLog::instance(), &FilterLog::logEntryAdded, this, &FilterLogDialog::slotLogEntryAdded);
    connect(FilterLog::instance(), &FilterLog::logShrinked, this, &FilterLogDialog::slotLogShrinked);
    connect(FilterLog::instance(), &FilterLog::logStateChanged, this, &FilterLogDialog::slotLogStateChanged);

    mainLayout->addWidget(buttonBox);

    connect(mUser1Button, &QPushButton::clicked, this, &FilterLogDialog::slotUser1);
    connect(mUser2Button, &QPushButton::clicked, this, &FilterLogDialog::slotUser2);
    connect(mTextEdit->editor(), &TextCustomEditor::PlainTextEditor::textChanged, this, &FilterLogDialog::slotTextChanged);

    slotTextChanged();
    readConfig();
    mIsInitialized = true;
}

void FilterLogDialog::slotTextChanged()
{
    const bool hasText = !mTextEdit->isEmpty();
    mUser2Button->setEnabled(hasText);
    mUser1Button->setEnabled(hasText);
    mShareButton->setEnabled(hasText);
}

void FilterLogDialog::readConfig()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup group(config, QStringLiteral("FilterLog"));
    const bool isEnabled = group.readEntry("Enabled", false);
    const bool isLogPatternDescription = group.readEntry("LogPatternDescription", false);
    const bool isLogRuleResult = group.readEntry("LogRuleResult", false);
    const bool isLogPatternResult = group.readEntry("LogPatternResult", false);
    const bool isLogAppliedAction = group.readEntry("LogAppliedAction", false);
    const int maxLogSize = group.readEntry("maxLogSize", -1);

    if (isEnabled != FilterLog::instance()->isLogging()) {
        FilterLog::instance()->setLogging(isEnabled);
    }
    if (isLogPatternDescription != FilterLog::instance()->isContentTypeEnabled(FilterLog::PatternDescription)) {
        FilterLog::instance()->setContentTypeEnabled(FilterLog::PatternDescription, isLogPatternDescription);
    }
    if (isLogRuleResult != FilterLog::instance()->isContentTypeEnabled(FilterLog::RuleResult)) {
        FilterLog::instance()->setContentTypeEnabled(FilterLog::RuleResult, isLogRuleResult);
    }
    if (isLogPatternResult != FilterLog::instance()->isContentTypeEnabled(FilterLog::PatternResult)) {
        FilterLog::instance()->setContentTypeEnabled(FilterLog::PatternResult, isLogPatternResult);
    }
    if (isLogAppliedAction != FilterLog::instance()->isContentTypeEnabled(FilterLog::AppliedAction)) {
        FilterLog::instance()->setContentTypeEnabled(FilterLog::AppliedAction, isLogAppliedAction);
    }
    if (FilterLog::instance()->maxLogSize() != maxLogSize) {
        FilterLog::instance()->setMaxLogSize(maxLogSize);
    }

    KConfigGroup geometryGroup(KSharedConfig::openConfig(), QStringLiteral("Geometry"));
    const QSize size = geometryGroup.readEntry("filterLogSize", QSize(600, 400));
    if (size.isValid()) {
        resize(size);
    } else {
        adjustSize();
    }
}

FilterLogDialog::~FilterLogDialog()
{
    disconnect(mTextEdit->editor(), &TextCustomEditor::PlainTextEditor::textChanged, this, &FilterLogDialog::slotTextChanged);
    KConfigGroup myGroup(KSharedConfig::openConfig(), QStringLiteral("Geometry"));
    myGroup.writeEntry("filterLogSize", size());
    myGroup.sync();
}

void FilterLogDialog::writeConfig()
{
    if (!mIsInitialized) {
        return;
    }

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup group(config, QStringLiteral("FilterLog"));
    group.writeEntry("Enabled", FilterLog::instance()->isLogging());
    group.writeEntry("LogPatternDescription", FilterLog::instance()->isContentTypeEnabled(FilterLog::PatternDescription));
    group.writeEntry("LogRuleResult", FilterLog::instance()->isContentTypeEnabled(FilterLog::RuleResult));
    group.writeEntry("LogPatternResult", FilterLog::instance()->isContentTypeEnabled(FilterLog::PatternResult));
    group.writeEntry("LogAppliedAction", FilterLog::instance()->isContentTypeEnabled(FilterLog::AppliedAction));
    group.writeEntry("maxLogSize", static_cast<int>(FilterLog::instance()->maxLogSize()));
    group.sync();
}

void FilterLogDialog::slotLogEntryAdded(const QString &logEntry)
{
    mTextEdit->editor()->appendHtml(logEntry);
}

void FilterLogDialog::slotLogShrinked()
{
    // limit the size of the shown log lines as soon as
    // the log has reached it's memory limit
    if (mTextEdit->editor()->document()->maximumBlockCount() <= 0) {
        mTextEdit->editor()->document()->setMaximumBlockCount(mTextEdit->editor()->document()->blockCount());
    }
}

void FilterLogDialog::slotLogStateChanged()
{
    mLogActiveBox->setChecked(FilterLog::instance()->isLogging());
    mLogPatternDescBox->setChecked(FilterLog::instance()->isContentTypeEnabled(FilterLog::PatternDescription));
    mLogRuleEvaluationBox->setChecked(FilterLog::instance()->isContentTypeEnabled(FilterLog::RuleResult));
    mLogPatternResultBox->setChecked(FilterLog::instance()->isContentTypeEnabled(FilterLog::PatternResult));
    mLogFilterActionBox->setChecked(FilterLog::instance()->isContentTypeEnabled(FilterLog::AppliedAction));

    // value in the QSpinBox is in KB while it's in Byte in the FilterLog
    int newLogSize = FilterLog::instance()->maxLogSize() / 1024;
    if (mLogMemLimitSpin->value() != newLogSize) {
        if (newLogSize <= 0) {
            mLogMemLimitSpin->setValue(1);
        } else {
            mLogMemLimitSpin->setValue(newLogSize);
        }
    }
    writeConfig();
}

void FilterLogDialog::slotChangeLogDetail()
{
    if (mLogPatternDescBox->isChecked() != FilterLog::instance()->isContentTypeEnabled(FilterLog::PatternDescription)) {
        FilterLog::instance()->setContentTypeEnabled(FilterLog::PatternDescription, mLogPatternDescBox->isChecked());
    }

    if (mLogRuleEvaluationBox->isChecked() != FilterLog::instance()->isContentTypeEnabled(FilterLog::RuleResult)) {
        FilterLog::instance()->setContentTypeEnabled(FilterLog::RuleResult, mLogRuleEvaluationBox->isChecked());
    }

    if (mLogPatternResultBox->isChecked() != FilterLog::instance()->isContentTypeEnabled(FilterLog::PatternResult)) {
        FilterLog::instance()->setContentTypeEnabled(FilterLog::PatternResult, mLogPatternResultBox->isChecked());
    }

    if (mLogFilterActionBox->isChecked() != FilterLog::instance()->isContentTypeEnabled(FilterLog::AppliedAction)) {
        FilterLog::instance()->setContentTypeEnabled(FilterLog::AppliedAction, mLogFilterActionBox->isChecked());
    }
}

void FilterLogDialog::slotSwitchLogState()
{
    FilterLog::instance()->setLogging(mLogActiveBox->isChecked());
}

void FilterLogDialog::slotChangeLogMemLimit(int value)
{
    mTextEdit->editor()->document()->setMaximumBlockCount(0); // Reset value
    if (value == 1) { // unilimited
        FilterLog::instance()->setMaxLogSize(-1);
    } else {
        FilterLog::instance()->setMaxLogSize(value * 1024);
    }
}

void FilterLogDialog::slotUser1()
{
    FilterLog::instance()->clear();
    mTextEdit->editor()->clear();
}

void FilterLogDialog::slotUser2()
{
    QPointer<QFileDialog> fdlg(new QFileDialog(this));

    fdlg->setAcceptMode(QFileDialog::AcceptSave);
    fdlg->setFileMode(QFileDialog::AnyFile);
    fdlg->selectFile(QStringLiteral("kmail-filter.html"));
    if (fdlg->exec() == QDialog::Accepted) {
        const QStringList fileName = fdlg->selectedFiles();

        if (!fileName.isEmpty() && !FilterLog::instance()->saveToFile(fileName.at(0))) {
            KMessageBox::error(this,
                               i18n("Could not write the file %1:\n"
                                    "\"%2\" is the detailed error description.",
                                    fileName.at(0),
                                    QString::fromLocal8Bit(strerror(errno))),
                               i18nc("@title:window", "KMail Error"));
        }
    }
    delete fdlg;
}

FilterLogTextEdit::FilterLogTextEdit(QWidget *parent)
    : TextCustomEditor::PlainTextEditor(parent)
{
}

void FilterLogTextEdit::addExtraMenuEntry(QMenu *menu, QPoint pos)
{
    Q_UNUSED(pos)
    if (!document()->isEmpty()) {
        auto sep = new QAction(menu);
        sep->setSeparator(true);
        menu->addAction(sep);
        QAction *clearAllAction = KStandardActions::clear(this, &FilterLogTextEdit::clear, menu);
        menu->addAction(clearAllAction);
    }
}

#include "moc_filterlogdialog.cpp"
