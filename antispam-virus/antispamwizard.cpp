/*
    This file is part of KMail.
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>

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

#include "antispamwizard.h"
#ifndef QT_NO_CURSOR
#include "messageviewer/utils/kcursorsaver.h"
#endif
#include "kmkernel.h"
#include "kmmainwin.h"
#include "folderrequester.h"
#include "foldertreewidget.h"
#include "foldertreeview.h"
#include "foldertreewidgetproxymodel.h"
#include "mailcommon/pop3settings.h"
#include "mailcommon/util/mailutil.h"
#include "pimcommon/imapresourcesettings.h"
#include "mailcommon/kernel/mailkernel.h"
#include "mailcommon/filter/mailfilter.h"
#include "mailcommon/filter/filteractions/filteraction.h"
#include "mailcommon/filter/filteractions/filteractiondict.h"
#include "mailcommon/filter/filtermanager.h"

#include "pimcommon/util/pimutil.h"

#include <AkonadiCore/AgentInstance>

#include "kmail_debug.h"
#include <QDialog>
#include <kiconloader.h>
#include <KLocalizedString>
#include <KProcess>
#include <ktoolinvocation.h>
#include <kconfiggroup.h>

#include <QLabel>
#include <QTextEdit>
#include <QTimer>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QListWidget>
#include <KHelpClient>
#include <QPushButton>
#include <KConfigGroup>
using namespace KMail;
using namespace MailCommon;

AntiSpamWizard::AntiSpamWizard(WizardMode mode,
                               QWidget *parent)
    : KAssistantDialog(parent),
      mInfoPage(Q_NULLPTR),
      mSpamRulesPage(Q_NULLPTR),
      mVirusRulesPage(Q_NULLPTR),
      mSummaryPage(Q_NULLPTR),
      mMode(mode)
{
    // read the configuration for the anti-spam tools
    ConfigReader reader(mMode, mToolList);
    reader.readAndMergeConfig();
    mToolList = reader.getToolList();
#ifndef NDEBUG
    if (mMode == AntiSpam) {
        qCDebug(KMAIL_LOG) << endl << "Considered anti-spam tools:";
    } else {
        qCDebug(KMAIL_LOG) << endl << "Considered anti-virus tools:";
    }
    QList<SpamToolConfig>::ConstIterator end(mToolList.constEnd());
    for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
            it != end; ++it) {
        qCDebug(KMAIL_LOG) << "Predefined tool:" << (*it).getId();
        qCDebug(KMAIL_LOG) << "Config version:" << (*it).getVersion();
        qCDebug(KMAIL_LOG) << "Selection priority:" << (*it).getPrio();
        qCDebug(KMAIL_LOG) << "Displayed name:" << (*it).getVisibleName();
        qCDebug(KMAIL_LOG) << "Executable:" << (*it).getExecutable();
        qCDebug(KMAIL_LOG) << "WhatsThis URL:" << (*it).getWhatsThisText();
        qCDebug(KMAIL_LOG) << "Filter name:" << (*it).getFilterName();
        qCDebug(KMAIL_LOG) << "Detection command:" << (*it).getDetectCmd();
        qCDebug(KMAIL_LOG) << "Learn spam command:" << (*it).getSpamCmd();
        qCDebug(KMAIL_LOG) << "Learn ham command:" << (*it).getHamCmd();
        qCDebug(KMAIL_LOG) << "Detection header:" << (*it).getDetectionHeader();
        qCDebug(KMAIL_LOG) << "Detection pattern:" << (*it).getDetectionPattern();
        qCDebug(KMAIL_LOG) << "Use as RegExp:" << (*it).isUseRegExp();
        qCDebug(KMAIL_LOG) << "Supports Bayes Filter:" << (*it).useBayesFilter();
        qCDebug(KMAIL_LOG) << "Type:" << (*it).getType();
    }
#endif

    const bool isAntiSpam = (mMode == AntiSpam);
    setWindowTitle(isAntiSpam  ? i18n("Anti-Spam Wizard")
                   : i18n("Anti-Virus Wizard"));
    mInfoPage = new ASWizInfoPage(mMode, Q_NULLPTR, QString());
    mInfoPageItem = addPage(mInfoPage,
                            isAntiSpam
                            ? i18n("Welcome to the KMail Anti-Spam Wizard")
                            : i18n("Welcome to the KMail Anti-Virus Wizard"));
    connect(mInfoPage, &ASWizInfoPage::selectionChanged, this, &AntiSpamWizard::checkProgramsSelections);

    if (isAntiSpam) {
        mSpamRulesPage = new ASWizSpamRulesPage(Q_NULLPTR, QString());
        mSpamRulesPageItem = addPage(mSpamRulesPage, i18n("Options to fine-tune the handling of spam messages"));
        connect(mSpamRulesPage, &ASWizSpamRulesPage::selectionChanged, this, &AntiSpamWizard::slotBuildSummary);

        mSummaryPage = new ASWizSummaryPage(Q_NULLPTR, QString());
        mSummaryPageItem = addPage(mSummaryPage, i18n("Summary of changes to be made by this wizard"));
    } else {
        mVirusRulesPage = new ASWizVirusRulesPage(Q_NULLPTR, QString());
        mVirusRulesPageItem = addPage(mVirusRulesPage, i18n("Options to fine-tune the handling of virus messages"));
        connect(mVirusRulesPage, &ASWizVirusRulesPage::selectionChanged, this, &AntiSpamWizard::checkVirusRulesSelections);
    }

    connect(button(QDialogButtonBox::Help), &QPushButton::clicked, this, &AntiSpamWizard::slotHelpClicked);

    QTimer::singleShot(0, this, SLOT(checkToolAvailability()));
}

void AntiSpamWizard::accept()
{
    if (mSpamRulesPage) {
        qCDebug(KMAIL_LOG) << "Folder name for messages classified as spam is"
                           << mSpamRulesPage->selectedSpamCollectionId();
        qCDebug(KMAIL_LOG) << "Folder name for messages classified as unsure is"
                           << mSpamRulesPage->selectedUnsureCollectionId();
    }
    if (mVirusRulesPage) {
        qCDebug(KMAIL_LOG) << "Folder name for viruses is"
                           << mVirusRulesPage->selectedFolderName();
    }

    FilterActionDict dict;
    QList<MailFilter *> filterList;
    bool replaceExistingFilters = false;

    // Let's start with virus detection and handling,
    // so we can avoid spam checks for viral messages
    if (mMode == AntiVirus) {
        if (!mVirusToolsUsed) {
            QDialog::accept();
            return;
        }
        QList<SpamToolConfig>::const_iterator end(mToolList.constEnd());
        for (QList<SpamToolConfig>::const_iterator it = mToolList.constBegin();
                it != end; ++it) {
            if (mInfoPage->isProgramSelected((*it).getVisibleName()) &&
                    (mVirusRulesPage->pipeRulesSelected() && (*it).isVirusTool())) {
                // pipe messages through the anti-virus tools,
                // one single filter for each tool
                // (could get combined but so it's easier to understand for the user)
                MailFilter *pipeFilter = new MailFilter();
                QList<FilterAction *> *pipeFilterActions = pipeFilter->actions();
                FilterAction *pipeFilterAction = dict.value(QLatin1String("filter app"))->create();
                pipeFilterAction->argsFromString((*it).getDetectCmd());
                pipeFilterActions->append(pipeFilterAction);
                SearchPattern *pipeFilterPattern = pipeFilter->pattern();
                pipeFilterPattern->setName(uniqueNameFor((*it).getFilterName()));
                pipeFilterPattern->append(SearchRule::createInstance("<size>",
                                          SearchRule::FuncIsGreaterOrEqual, QStringLiteral("0")));
                pipeFilter->setApplyOnOutbound(false);
                pipeFilter->setApplyOnInbound();
                pipeFilter->setApplyOnExplicit();
                pipeFilter->setStopProcessingHere(false);
                pipeFilter->setConfigureShortcut(false);

                filterList.append(pipeFilter);
            }
        }

        if (mVirusRulesPage->moveRulesSelected()) {
            // Sort out viruses depending on header fields set by the tools
            MailFilter *virusFilter = new MailFilter();
            QList<FilterAction *> *virusFilterActions = virusFilter->actions();
            FilterAction *virusFilterAction1 = dict.value(QLatin1String("transfer"))->create();
            virusFilterAction1->argsFromString(mVirusRulesPage->selectedFolderName());
            virusFilterActions->append(virusFilterAction1);
            if (mVirusRulesPage->markReadRulesSelected()) {
                FilterAction *virusFilterAction2 = dict.value(QLatin1String("set status"))->create();
                virusFilterAction2->argsFromString(QLatin1String("R"));   // Read
                virusFilterActions->append(virusFilterAction2);
            }
            SearchPattern *virusFilterPattern = virusFilter->pattern();
            virusFilterPattern->setName(uniqueNameFor(i18n("Virus handling")));
            virusFilterPattern->setOp(SearchPattern::OpOr);
            QList<SpamToolConfig>::ConstIterator endSpamTool(mToolList.constEnd());
            for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
                    it != endSpamTool; ++it) {
                if (mInfoPage->isProgramSelected((*it).getVisibleName())) {
                    if ((*it).isVirusTool()) {
                        const QByteArray header = (*it).getDetectionHeader().toLatin1();
                        const QString &pattern = (*it).getDetectionPattern();
                        if ((*it).isUseRegExp())
                            virusFilterPattern->append(
                                SearchRule::createInstance(header,
                                                           SearchRule::FuncRegExp, pattern));
                        else
                            virusFilterPattern->append(
                                SearchRule::createInstance(header,
                                                           SearchRule::FuncContains, pattern));
                    }
                }
            }
            virusFilter->setApplyOnOutbound(false);
            virusFilter->setApplyOnInbound();
            virusFilter->setApplyOnExplicit();
            virusFilter->setStopProcessingHere(true);
            virusFilter->setConfigureShortcut(false);

            filterList.append(virusFilter);
        }
    } else { // AntiSpam mode
        if (!mSpamToolsUsed) {
            QDialog::accept();
            return;
        }
        // TODO Existing filters with same name are replaced. This is hardcoded
        // ATM and needs to be replaced with a value from a (still missing)
        // checkbox in the GUI. At least, the replacement is announced in the GUI.
        replaceExistingFilters = true;
        QList<SpamToolConfig>::ConstIterator end(mToolList.constEnd());
        for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
                it != end; ++it) {
            if (mInfoPage->isProgramSelected((*it).getVisibleName()) &&
                    (*it).isSpamTool() && !(*it).isDetectionOnly()) {
                // pipe messages through the anti-spam tools,
                // one single filter for each tool
                // (could get combined but so it's easier to understand for the user)
                MailFilter *pipeFilter = new MailFilter();
                QList<FilterAction *> *pipeFilterActions = pipeFilter->actions();
                FilterAction *pipeFilterAction = dict.value(QLatin1String("filter app"))->create();
                pipeFilterAction->argsFromString((*it).getDetectCmd());
                pipeFilterActions->append(pipeFilterAction);
                SearchPattern *pipeFilterPattern = pipeFilter->pattern();
                if (replaceExistingFilters) {
                    pipeFilterPattern->setName((*it).getFilterName());
                } else {
                    pipeFilterPattern->setName(uniqueNameFor((*it).getFilterName()));
                }
                pipeFilterPattern->append(SearchRule::createInstance("<size>",
                                          SearchRule::FuncIsLessOrEqual, QStringLiteral("256000")));
                pipeFilter->setApplyOnOutbound(false);
                pipeFilter->setApplyOnInbound();
                pipeFilter->setApplyOnExplicit();
                pipeFilter->setStopProcessingHere(false);
                pipeFilter->setConfigureShortcut(false);

                filterList.append(pipeFilter);
            }
        }

        // Sort out spam depending on header fields set by the tools
        MailFilter *spamFilter = new MailFilter();
        QList<FilterAction *> *spamFilterActions = spamFilter->actions();
        if (mSpamRulesPage->moveSpamSelected()) {
            FilterAction *spamFilterAction1 = dict.value(QLatin1String("transfer"))->create();
            spamFilterAction1->argsFromString(mSpamRulesPage->selectedSpamCollectionId());
            spamFilterActions->append(spamFilterAction1);
        }
        FilterAction *spamFilterAction2 = dict.value(QLatin1String("set status"))->create();
        spamFilterAction2->argsFromString(QLatin1String("P"));   // Spam
        spamFilterActions->append(spamFilterAction2);
        if (mSpamRulesPage->markAsReadSelected()) {
            FilterAction *spamFilterAction3 = dict.value(QLatin1String("set status"))->create();
            spamFilterAction3->argsFromString(QLatin1String("R"));   // Read
            spamFilterActions->append(spamFilterAction3);
        }
        SearchPattern *spamFilterPattern = spamFilter->pattern();
        if (replaceExistingFilters) {
            spamFilterPattern->setName(i18n("Spam Handling"));
        } else {
            spamFilterPattern->setName(uniqueNameFor(i18n("Spam Handling")));
        }
        spamFilterPattern->setOp(SearchPattern::OpOr);
        QList<SpamToolConfig>::ConstIterator endToolList(mToolList.constEnd());
        for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
                it != endToolList; ++it) {
            if (mInfoPage->isProgramSelected((*it).getVisibleName())) {
                if ((*it).isSpamTool()) {
                    const QByteArray header = (*it).getDetectionHeader().toLatin1();
                    const QString &pattern = (*it).getDetectionPattern();
                    if ((*it).isUseRegExp())
                        spamFilterPattern->append(
                            SearchRule::createInstance(header,
                                                       SearchRule::FuncRegExp, pattern));
                    else
                        spamFilterPattern->append(
                            SearchRule::createInstance(header,
                                                       SearchRule::FuncContains, pattern));
                }
            }
        }
        spamFilter->setApplyOnOutbound(false);
        spamFilter->setApplyOnInbound();
        spamFilter->setApplyOnExplicit();
        spamFilter->setStopProcessingHere(true);
        spamFilter->setConfigureShortcut(false);
        filterList.append(spamFilter);

        if (mSpamRulesPage->moveUnsureSelected()) {
            // Sort out messages classified as unsure
            bool atLeastOneUnsurePattern = false;
            MailFilter *unsureFilter = new MailFilter();
            QList<FilterAction *> *unsureFilterActions = unsureFilter->actions();
            FilterAction *unsureFilterAction1 = dict.value(QLatin1String("transfer"))->create();
            unsureFilterAction1->argsFromString(mSpamRulesPage->selectedUnsureCollectionId());
            unsureFilterActions->append(unsureFilterAction1);
            SearchPattern *unsureFilterPattern = unsureFilter->pattern();
            if (replaceExistingFilters) {
                unsureFilterPattern->setName(i18n("Semi spam (unsure) handling"));
            } else {
                unsureFilterPattern->setName(uniqueNameFor(i18n("Semi spam (unsure) handling")));
            }
            unsureFilterPattern->setOp(SearchPattern::OpOr);
            QList<SpamToolConfig>::ConstIterator end(mToolList.constEnd());
            for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
                    it != end; ++it) {
                if (mInfoPage->isProgramSelected((*it).getVisibleName())) {
                    if ((*it).isSpamTool() && (*it).hasTristateDetection()) {
                        atLeastOneUnsurePattern = true;
                        const QByteArray header = (*it).getDetectionHeader().toLatin1();
                        const QString &pattern = (*it).getDetectionPattern2();
                        if ((*it).isUseRegExp())
                            unsureFilterPattern->append(
                                SearchRule::createInstance(header,
                                                           SearchRule::FuncRegExp, pattern));
                        else
                            unsureFilterPattern->append(
                                SearchRule::createInstance(header,
                                                           SearchRule::FuncContains, pattern));
                    }
                }
            }
            unsureFilter->setApplyOnOutbound(false);
            unsureFilter->setApplyOnInbound();
            unsureFilter->setApplyOnExplicit();
            unsureFilter->setStopProcessingHere(true);
            unsureFilter->setConfigureShortcut(false);

            if (atLeastOneUnsurePattern) {
                filterList.append(unsureFilter);
            } else {
                delete unsureFilter;
            }
        }

        // Classify messages manually as Spam
        MailFilter *classSpamFilter = new MailFilter();
        classSpamFilter->setIcon(QLatin1String("mail-mark-junk"));
        QList<FilterAction *> *classSpamFilterActions = classSpamFilter->actions();
        FilterAction *classSpamFilterActionFirst = dict.value(QLatin1String("set status"))->create();
        classSpamFilterActionFirst->argsFromString(QLatin1String("P"));
        classSpamFilterActions->append(classSpamFilterActionFirst);
        QList<SpamToolConfig>::ConstIterator endToolList2(mToolList.constEnd());
        for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
                it != endToolList2; ++it) {
            if (mInfoPage->isProgramSelected((*it).getVisibleName())
                    && (*it).useBayesFilter() && !(*it).isDetectionOnly()) {
                FilterAction *classSpamFilterAction = dict.value(QLatin1String("execute"))->create();
                classSpamFilterAction->argsFromString((*it).getSpamCmd());
                classSpamFilterActions->append(classSpamFilterAction);
            }
        }
        if (mSpamRulesPage->moveSpamSelected()) {
            FilterAction *classSpamFilterActionLast = dict.value(QLatin1String("transfer"))->create();
            classSpamFilterActionLast->argsFromString(mSpamRulesPage->selectedSpamCollectionId());
            classSpamFilterActions->append(classSpamFilterActionLast);
        }

        SearchPattern *classSpamFilterPattern = classSpamFilter->pattern();
        if (replaceExistingFilters) {
            classSpamFilterPattern->setName(i18n("Classify as Spam"));
        } else {
            classSpamFilterPattern->setName(uniqueNameFor(i18n("Classify as Spam")));
        }
        classSpamFilterPattern->append(SearchRule::createInstance("<size>",
                                       SearchRule::FuncIsGreaterOrEqual, QStringLiteral("0")));
        classSpamFilter->setApplyOnOutbound(false);
        classSpamFilter->setApplyOnInbound(false);
        classSpamFilter->setApplyOnExplicit(false);
        classSpamFilter->setStopProcessingHere(true);
        classSpamFilter->setConfigureShortcut(true);
        classSpamFilter->setConfigureToolbar(true);
        classSpamFilter->setToolbarName(i18n("Spam"));
        filterList.append(classSpamFilter);

        // Classify messages manually as not Spam / as Ham
        MailFilter *classHamFilter = new MailFilter();
        classHamFilter->setIcon(QLatin1String("mail-mark-notjunk"));
        QList<FilterAction *> *classHamFilterActions = classHamFilter->actions();
        FilterAction *classHamFilterActionFirst = dict.value(QLatin1String("set status"))->create();
        classHamFilterActionFirst->argsFromString(QLatin1String("H"));
        classHamFilterActions->append(classHamFilterActionFirst);
        end = mToolList.constEnd();
        for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
                it != end; ++it) {
            if (mInfoPage->isProgramSelected((*it).getVisibleName())
                    && (*it).useBayesFilter() && !(*it).isDetectionOnly()) {
                FilterAction *classHamFilterAction = dict.value(QLatin1String("execute"))->create();
                classHamFilterAction->argsFromString((*it).getHamCmd());
                classHamFilterActions->append(classHamFilterAction);
            }
        }
        end = mToolList.constEnd();
        for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
                it != end; ++it) {
            if (mInfoPage->isProgramSelected((*it).getVisibleName())
                    && (*it).useBayesFilter() && !(*it).isDetectionOnly()) {
                FilterAction *classHamFilterAction = dict.value(QLatin1String("filter app"))->create();
                classHamFilterAction->argsFromString((*it).getNoSpamCmd());
                classHamFilterActions->append(classHamFilterAction);
            }
        }
        SearchPattern *classHamFilterPattern = classHamFilter->pattern();
        if (replaceExistingFilters) {
            classHamFilterPattern->setName(i18n("Classify as NOT Spam"));
        } else {
            classHamFilterPattern->setName(uniqueNameFor(i18n("Classify as NOT Spam")));
        }
        classHamFilterPattern->append(SearchRule::createInstance("<size>",
                                      SearchRule::FuncIsGreaterOrEqual, QStringLiteral("0")));
        classHamFilter->setApplyOnOutbound(false);
        classHamFilter->setApplyOnInbound(false);
        classHamFilter->setApplyOnExplicit(false);
        classHamFilter->setStopProcessingHere(true);
        classHamFilter->setConfigureShortcut(true);
        classHamFilter->setConfigureToolbar(true);
        classHamFilter->setToolbarName(i18n("Ham"));
        filterList.append(classHamFilter);
    }

    /* Now that all the filters have been added to the list, tell
    * the filter manager about it. That will Q_EMIT filterListUpdate
    * which will result in the filter list in kmmainwidget being
    * initialized. This should happend only once. */
    if (!filterList.isEmpty()) {
        MailCommon::FilterManager::instance()->appendFilters(filterList, replaceExistingFilters);
    }

    QDialog::accept();
}

void AntiSpamWizard::checkProgramsSelections()
{
    bool supportUnsure = false;

    mSpamToolsUsed = false;
    mVirusToolsUsed = false;
    QList<SpamToolConfig>::ConstIterator end(mToolList.constEnd());
    for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
            it != end; ++it) {
        if (mInfoPage->isProgramSelected((*it).getVisibleName())) {
            if ((*it).isSpamTool()) {
                mSpamToolsUsed = true;
                if ((*it).hasTristateDetection()) {
                    supportUnsure = true;
                }
            }
            if ((*it).isVirusTool()) {
                mVirusToolsUsed = true;
            }

            if (mSpamToolsUsed && mVirusToolsUsed && supportUnsure) {
                break;
            }
        }
    }

    if (mMode == AntiSpam) {
        mSpamRulesPage->allowUnsureFolderSelection(supportUnsure);
        mSpamRulesPage->allowMoveSpam(mSpamToolsUsed);
        slotBuildSummary();
        setAppropriate(mSpamRulesPageItem, mSpamToolsUsed);
        setAppropriate(mSummaryPageItem, mSpamToolsUsed);
    } else if (mMode == AntiVirus) {
        if (mVirusToolsUsed) {
            checkVirusRulesSelections();
        }
        setAppropriate(mVirusRulesPageItem, mVirusToolsUsed);
    }
}

void AntiSpamWizard::checkVirusRulesSelections()
{
    //setFinishEnabled( mVirusRulesPage, anyVirusOptionChecked() );
}

void AntiSpamWizard::checkToolAvailability()
{
    // this can take some time to find the tools
#ifndef QT_NO_CURSOR
    MessageViewer::KCursorSaver busy(MessageViewer::KBusyPtr::busy());
#endif
    bool found = false;
    QList<SpamToolConfig>::ConstIterator end(mToolList.constEnd());
    for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
            it != end; ++it) {
        const QString text(i18n("Scanning for %1...", (*it).getId()));
        mInfoPage->setScanProgressText(text);
        if ((*it).isSpamTool() && (*it).isServerBased()) {
            // check the configured account for pattern in <server>
            const QString pattern = (*it).getServerPattern();
            qCDebug(KMAIL_LOG) << "Testing for server pattern:" << pattern;
            const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
            foreach (const Akonadi::AgentInstance &type, lst) {
                if (type.status() == Akonadi::AgentInstance::Broken) {
                    continue;
                }
                if (PimCommon::Util::isImapResource(type.identifier())) {
                    OrgKdeAkonadiImapSettingsInterface *iface = PimCommon::Util::createImapSettingsInterface(type.identifier());
                    if (iface->isValid()) {
                        const QString host = iface->imapServer();
                        if (host.toLower().contains(pattern.toLower())) {
                            mInfoPage->addAvailableTool((*it).getVisibleName());
                            found = true;
                        }
                    }
                    delete iface;
                } else if (type.identifier().contains(POP3_RESOURCE_IDENTIFIER)) {
                    OrgKdeAkonadiPOP3SettingsInterface *iface = MailCommon::Util::createPop3SettingsInterface(type.identifier());
                    if (iface->isValid()) {
                        const QString host = iface->host();
                        if (host.toLower().contains(pattern.toLower())) {
                            mInfoPage->addAvailableTool((*it).getVisibleName());
                            found = true;
                        }
                    }
                    delete iface;
                }
            }
        } else {
            // check the availability of the application
            qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 200);
            if (!checkForProgram((*it).getExecutable())) {
                mInfoPage->addAvailableTool((*it).getVisibleName());
                found = true;
            }
        }
    }
    if (found)
        mInfoPage->setScanProgressText((mMode == AntiSpam)
                                       ? i18n("Scanning for anti-spam tools finished.")
                                       : i18n("Scanning for anti-virus tools finished."));
    else {
        mInfoPage->setScanProgressText((mMode == AntiSpam)
                                       ? i18n("<p>Sorry, no spam detection tools have been found. "
                                              "Install your spam detection software and "
                                              "re-run this wizard.</p>")
                                       : i18n("Scanning complete. No anti-virus tools found."));
    }
    checkProgramsSelections();
}

void AntiSpamWizard::slotHelpClicked()
{
    KHelpClient::invokeHelp((mMode == AntiSpam) ? QLatin1String("the-anti-spam-wizard") : QLatin1String("the-anti-virus-wizard") , QStringLiteral("kmail"));
}

void AntiSpamWizard::slotBuildSummary()
{
    QString text;
    QString newFilters;
    QString replaceFilters;

    if (mMode == AntiVirus) {
        text.clear(); // TODO add summary for the virus part
    } else { // AntiSpam mode
        if (mSpamRulesPage->markAsReadSelected()) {
            if (mSpamRulesPage->moveSpamSelected())
                text = i18n("<p>Messages classified as spam are marked as read."
                            "<br />Spam messages are moved into the folder named <i>%1</i>.</p>"
                            , mSpamRulesPage->selectedSpamCollectionName());
            else
                text = i18n("<p>Messages classified as spam are marked as read."
                            "<br />Spam messages are not moved into a certain folder.</p>");
        } else {
            if (mSpamRulesPage->moveSpamSelected())
                text = i18n("<p>Messages classified as spam are not marked as read."
                            "<br />Spam messages are moved into the folder named <i>%1</i>.</p>"
                            , mSpamRulesPage->selectedSpamCollectionName());
            else
                text = i18n("<p>Messages classified as spam are not marked as read."
                            "<br />Spam messages are not moved into a certain folder.</p>");
        }
        QList<SpamToolConfig>::ConstIterator end(mToolList.constEnd());
        for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
                it != end; ++it) {
            if (mInfoPage->isProgramSelected((*it).getVisibleName()) &&
                    (*it).isSpamTool() && !(*it).isDetectionOnly()) {
                sortFilterOnExistance((*it).getFilterName(), newFilters, replaceFilters);
            }
        }
        sortFilterOnExistance(i18n("Spam Handling"), newFilters, replaceFilters);

        // The need for a andling of status "probably spam" depends on the tools chosen
        if (mSpamRulesPage->moveUnsureSelected()) {
            bool atLeastOneUnsurePattern = false;
            end =  mToolList.constEnd();
            for (QList<SpamToolConfig>::ConstIterator it = mToolList.constBegin();
                    it != end; ++it) {
                if (mInfoPage->isProgramSelected((*it).getVisibleName())) {
                    if ((*it).isSpamTool() && (*it).hasTristateDetection()) {
                        atLeastOneUnsurePattern = true;
                        break;
                    }
                }
            }
            if (atLeastOneUnsurePattern) {
                sortFilterOnExistance(i18n("Semi spam (unsure) handling"),
                                      newFilters, replaceFilters);
                text += i18n("<p>The folder for messages classified as unsure (probably spam) is <i>%1</i>.</p>"
                             , mSpamRulesPage->selectedUnsureCollectionName());
            }
        }

        // Manual classification via toolbar icon / manually applied filter action
        sortFilterOnExistance(i18n("Classify as Spam"),
                              newFilters, replaceFilters);
        sortFilterOnExistance(i18n("Classify as NOT Spam"),
                              newFilters, replaceFilters);

        // Show the filters in the summary
        if (!newFilters.isEmpty())
            text += i18n("<p>The wizard will create the following filters:<ul>%1</ul></p>"
                         , newFilters);
        if (!replaceFilters.isEmpty())
            text += i18n("<p>The wizard will replace the following filters:<ul>%1</ul></p>"
                         , replaceFilters);
    }

    mSummaryPage->setSummaryText(text);
}

int AntiSpamWizard::checkForProgram(const QString &executable) const
{
    qCDebug(KMAIL_LOG) << "Testing for executable:" << executable;
    KProcess process;
    process.setShellCommand(executable);
    return process.execute();
}

bool AntiSpamWizard::anyVirusOptionChecked() const
{
    return (mVirusRulesPage->moveRulesSelected()
            || mVirusRulesPage->pipeRulesSelected());
}

const QString AntiSpamWizard::uniqueNameFor(const QString &name)
{
    return MailCommon::FilterManager::instance()->createUniqueFilterName(name);
}

void AntiSpamWizard::sortFilterOnExistance(
    const QString &intendedFilterName,
    QString &newFilters, QString &replaceFilters)
{
    if (uniqueNameFor(intendedFilterName) == intendedFilterName) {
        newFilters += QLatin1String("<li>") + intendedFilterName + QLatin1String("</li>");
    } else {
        replaceFilters += QLatin1String("<li>") + intendedFilterName + QLatin1String("</li>");
    }
}

//---------------------------------------------------------------------------
AntiSpamWizard::SpamToolConfig::SpamToolConfig(const QString &toolId,
        int configVersion, int prio, const QString &name, const QString &exec,
        const QString &url, const QString &filter, const QString &detection,
        const QString &spam, const QString &ham, const QString &noSpam,
        const QString &header, const QString &pattern, const QString &pattern2,
        const QString &serverPattern, bool detectionOnly, bool regExp,
        bool bayesFilter, bool tristateDetection, WizardMode type)
    : mId(toolId), mVersion(configVersion), mPrio(prio),
      mVisibleName(name), mExecutable(exec), mWhatsThisText(url),
      mFilterName(filter), mDetectCmd(detection), mSpamCmd(spam),
      mHamCmd(ham), mNoSpamCmd(noSpam), mDetectionHeader(header),
      mDetectionPattern(pattern), mDetectionPattern2(pattern2),
      mServerPattern(serverPattern), mDetectionOnly(detectionOnly),
      mUseRegExp(regExp), mSupportsBayesFilter(bayesFilter),
      mSupportsUnsure(tristateDetection), mType(type)
{
}

bool AntiSpamWizard::SpamToolConfig::isServerBased() const
{
    return !mServerPattern.isEmpty();
}

//---------------------------------------------------------------------------
AntiSpamWizard::ConfigReader::ConfigReader(WizardMode mode,
        QList<SpamToolConfig> &configList)
    : mToolList(configList),
      mMode(mode)
{
    if (mMode == AntiSpam) {
        mConfig = KSharedConfig::openConfig(QLatin1String("kmail.antispamrc"));
    } else {
        mConfig = KSharedConfig::openConfig(QLatin1String("kmail.antivirusrc"));
    }
}

AntiSpamWizard::ConfigReader::~ConfigReader()
{
}

void AntiSpamWizard::ConfigReader::readAndMergeConfig()
{
    QString groupName = (mMode == AntiSpam)
                        ? QStringLiteral("Spamtool #%1")
                        : QStringLiteral("Virustool #%1");
    // read the configuration from the global config file
    mConfig->setReadDefaults(true);
    KConfigGroup general(mConfig, "General");
    const int registeredTools = general.readEntry("tools", 0);
    for (int i = 1; i <= registeredTools; ++i) {
        KConfigGroup toolConfig(mConfig, groupName.arg(i));
        if (!toolConfig.readEntry("HeadersOnly", false)) {
            mToolList.append(readToolConfig(toolConfig));
        }
    }

    // read the configuration from the user config file
    // and merge newer config data
    mConfig->setReadDefaults(false);
    KConfigGroup user_general(mConfig, "General");
    const int user_registeredTools = user_general.readEntry("tools", 0);
    for (int i = 1; i <= user_registeredTools; ++i) {
        KConfigGroup toolConfig(mConfig, groupName.arg(i));
        if (!toolConfig.readEntry("HeadersOnly", false)) {
            mergeToolConfig(readToolConfig(toolConfig));
        }
    }
    // Make sure to have add least one tool listed even when the
    // config file was not found or whatever went wrong
    // Currently only works for spam tools
    if (mMode == AntiSpam) {
        if (registeredTools < 1 && user_registeredTools < 1) {
            mToolList.append(createDummyConfig());
        }
        sortToolList();
    }
}

AntiSpamWizard::SpamToolConfig
AntiSpamWizard::ConfigReader::readToolConfig(KConfigGroup &configGroup)
{
    const QString id = configGroup.readEntry("Ident");
    const int version = configGroup.readEntry("Version", 0);
#ifndef NDEBUG
    qCDebug(KMAIL_LOG) << "Found predefined tool:" << id;
    qCDebug(KMAIL_LOG) << "With config version  :" << version;
#endif
    const int prio = configGroup.readEntry("Priority", 1);
    const QString name = configGroup.readEntry("VisibleName");
    const QString executable = configGroup.readEntry("Executable");
    const QString url = configGroup.readEntry("URL");
    const QString filterName = configGroup.readEntry("PipeFilterName");
    const QString detectCmd = configGroup.readEntry("PipeCmdDetect");
    const QString spamCmd = configGroup.readEntry("ExecCmdSpam");
    const QString hamCmd = configGroup.readEntry("ExecCmdHam");
    const QString noSpamCmd = configGroup.readEntry("PipeCmdNoSpam");
    const QString header = configGroup.readEntry("DetectionHeader");
    const QString pattern = configGroup.readEntry("DetectionPattern");
    const QString pattern2 = configGroup.readEntry("DetectionPattern2");
    const QString serverPattern = configGroup.readEntry("ServerPattern");
    const bool detectionOnly = configGroup.readEntry("DetectionOnly", false);
    const bool useRegExp = configGroup.readEntry("UseRegExp", false);
    const bool supportsBayes = configGroup.readEntry("SupportsBayes", false);
    const bool supportsUnsure = configGroup.readEntry("SupportsUnsure", false);
    return SpamToolConfig(id, version, prio, name, executable, url,
                          filterName, detectCmd, spamCmd, hamCmd, noSpamCmd,
                          header, pattern, pattern2, serverPattern,
                          detectionOnly, useRegExp,
                          supportsBayes, supportsUnsure, mMode);
}

AntiSpamWizard::SpamToolConfig AntiSpamWizard::ConfigReader::createDummyConfig()
{
    return SpamToolConfig(QLatin1String("spamassassin"), 0, 1,
                          QLatin1String("SpamAssassin"), QStringLiteral("spamassassin -V"),
                          QLatin1String("http://spamassassin.org"), QStringLiteral("SpamAssassin Check"),
                          QLatin1String("spamassassin -L"),
                          QLatin1String("sa-learn -L --spam --no-sync --single"),
                          QLatin1String("sa-learn -L --ham --no-sync --single"),
                          QLatin1String("spamassassin -d"),
                          QLatin1String("X-Spam-Flag"), QStringLiteral("yes"), QString(), QString(),
                          false, false, true, false, AntiSpam);
}

void AntiSpamWizard::ConfigReader::mergeToolConfig(const AntiSpamWizard::SpamToolConfig &config)
{
    bool found = false;
    QList<SpamToolConfig>::Iterator end(mToolList.end());
    for (QList<SpamToolConfig>::Iterator it = mToolList.begin();
            it != end; ++it) {
#ifndef NDEBUG
        qCDebug(KMAIL_LOG) << "Check against tool:" << (*it).getId();
        qCDebug(KMAIL_LOG) << "Against version   :" << (*it).getVersion();
#endif
        if ((*it).getId() == config.getId()) {
            found = true;
            if ((*it).getVersion() < config.getVersion()) {
#ifndef NDEBUG
                qCDebug(KMAIL_LOG) << "Replacing config ...";
#endif
                mToolList.erase(it);
                mToolList.append(config);
            }
            break;
        }
    }
    if (!found) {
        mToolList.append(config);
    }
}

void AntiSpamWizard::ConfigReader::sortToolList()
{
    QList<SpamToolConfig> tmpList;
    SpamToolConfig config;

    while (!mToolList.isEmpty()) {
        QList<SpamToolConfig>::Iterator highest;
        int priority = 0; // ascending
        QList<SpamToolConfig>::Iterator end(mToolList.end());
        for (QList<SpamToolConfig>::Iterator it = mToolList.begin();
                it != end; ++it) {
            if ((*it).getPrio() > priority) {
                priority = (*it).getPrio();
                highest = it;
            }
        }
        config = (*highest);
        tmpList.append(config);
        mToolList.erase(highest);
    }
    QList<SpamToolConfig>::ConstIterator end(tmpList.constEnd());
    for (QList<SpamToolConfig>::ConstIterator it = tmpList.constBegin();
            it != end; ++it) {
        mToolList.append((*it));
    }
}

//---------------------------------------------------------------------------
ASWizPage::ASWizPage(QWidget *parent, const QString &name)
    : QWidget(parent)
{
    setObjectName(name);
    const QString banner = QStringLiteral("kmwizard.png");
    mLayout = new QHBoxLayout(this);

    QVBoxLayout *sideLayout = new QVBoxLayout();
    mLayout->addItem(sideLayout);
    mLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QLabel *bannerLabel = new QLabel(this);
    bannerLabel->setPixmap(UserIcon(banner));
    bannerLabel->setScaledContents(false);
    bannerLabel->setFrameShape(QFrame::StyledPanel);
    bannerLabel->setFrameShadow(QFrame::Sunken);
    bannerLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    sideLayout->addWidget(bannerLabel);
    sideLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

//---------------------------------------------------------------------------
ASWizInfoPage::ASWizInfoPage(AntiSpamWizard::WizardMode mode,
                             QWidget *parent, const QString &name)
    : ASWizPage(parent, name)
{
    QBoxLayout *layout = new QVBoxLayout();
    mLayout->addItem(layout);

    QTextEdit *introText = new QTextEdit(this);
    introText->setText(
        (mode == AntiSpamWizard::AntiSpam)
        ? i18n(
            "The wizard will search for any tools to do spam detection\n"
            "and setup KMail to work with them."
        )
        : i18n(
            "<p>Here you can get some assistance in setting up KMail's filter "
            "rules to use some commonly-known anti-virus tools.</p>"
            "<p>The wizard can detect those tools on your computer as "
            "well as create filter rules to classify messages using these "
            "tools and to separate messages containing viruses. "
            "The wizard will not take any existing filter "
            "rules into consideration: it will always append the new rules.</p>"
            "<p><b>Warning:</b> As KMail appears to be frozen during the scan of the "
            "messages for viruses, you may encounter problems with "
            "the responsiveness of KMail because anti-virus tool "
            "operations are usually time consuming; please consider "
            "deleting the filter rules created by the wizard to get "
            "back to the former behavior.</p>"
        ));
    introText->setReadOnly(true);
    introText->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,  QSizePolicy::Expanding));
    layout->addWidget(introText);

    mScanProgressText = new QLabel(this);
    mScanProgressText->clear();
    mScanProgressText->setWordWrap(true);
    layout->addWidget(mScanProgressText);

    mToolsList = new QListWidget(this);
    mToolsList->hide();
    mToolsList->setSelectionMode(QAbstractItemView::MultiSelection);
    mToolsList->setLayoutMode(QListView::Batched);
    mToolsList->setBatchSize(10);
    mToolsList->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,  QSizePolicy::Maximum));
    layout->addWidget(mToolsList);
    connect(mToolsList->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ASWizInfoPage::processSelectionChange);

    mSelectionHint = new QLabel(this);
    mSelectionHint->clear();
    mSelectionHint->setWordWrap(true);
    layout->addWidget(mSelectionHint);
}

void ASWizInfoPage::setScanProgressText(const QString &toolName)
{
    mScanProgressText->setText(toolName);
}

void ASWizInfoPage::addAvailableTool(const QString &visibleName)
{
    mToolsList->addItem(visibleName);
    if (!mToolsList->isVisible()) {
        mToolsList->show();
        mToolsList->selectionModel()->clearSelection();
        mToolsList->setCurrentRow(0);
        mSelectionHint->setText(i18n("<p>Please select the tools to be used "
                                     "for the detection and go "
                                     "to the next page.</p>"));
    }
}

bool ASWizInfoPage::isProgramSelected(const QString &visibleName) const
{
    const QList<QListWidgetItem *> foundItems = mToolsList->findItems(visibleName, Qt::MatchFixedString);
    return (!foundItems.isEmpty() && foundItems[0]->isSelected());
}

void ASWizInfoPage::processSelectionChange()
{
    Q_EMIT selectionChanged();
}

//---------------------------------------------------------------------------
ASWizSpamRulesPage::ASWizSpamRulesPage(QWidget *parent, const QString &name)
    : ASWizPage(parent, name)
{
    QVBoxLayout *layout = new QVBoxLayout();
    mLayout->addItem(layout);

    mMarkRules = new QCheckBox(i18n("&Mark detected spam messages as read"), this);
    mMarkRules->setWhatsThis(
        i18n("Mark messages which have been classified as spam as read."));
    layout->addWidget(mMarkRules);

    mMoveSpamRules = new QCheckBox(i18n("Move &known spam to:"), this);
    mMoveSpamRules->setWhatsThis(
        i18n("The default folder for spam messages is the trash folder, "
             "but you may change that in the folder view below."));
    layout->addWidget(mMoveSpamRules);

    mFolderReqForSpamFolder = new FolderRequester(this);
    mFolderReqForSpamFolder->setCollection(CommonKernel->trashCollectionFolder());
    mFolderReqForSpamFolder->setMustBeReadWrite(true);
    mFolderReqForSpamFolder->setShowOutbox(false);

    QHBoxLayout *hLayout1 = new QHBoxLayout();
    layout->addItem(hLayout1);
    hLayout1->addWidget(mFolderReqForSpamFolder);

    mMoveUnsureRules = new QCheckBox(i18n("Move &probable spam to:"), this);
    mMoveUnsureRules->setWhatsThis(
        i18n("The default folder is the inbox folder, but you may change that "
             "in the folder view below.<p>"
             "Not all tools support a classification as unsure. If you have not "
             "selected a capable tool, you cannot select a folder as well.</p>"));
    layout->addWidget(mMoveUnsureRules);

    mFolderReqForUnsureFolder = new FolderRequester(this);
    mFolderReqForUnsureFolder->setCollection(CommonKernel->inboxCollectionFolder());
    mFolderReqForUnsureFolder->setMustBeReadWrite(true);
    mFolderReqForUnsureFolder->setShowOutbox(false);

    QHBoxLayout *hLayout2 = new QHBoxLayout();
    layout->addItem(hLayout2);
    hLayout2->addWidget(mFolderReqForUnsureFolder);

    layout->addStretch();

    connect(mMarkRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange()));
    connect(mMoveSpamRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange()));
    connect(mMoveUnsureRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange()));
    connect(mFolderReqForSpamFolder, SIGNAL(folderChanged(Akonadi::Collection)),
            this, SLOT(processSelectionChange(Akonadi::Collection)));
    connect(mFolderReqForUnsureFolder, SIGNAL(folderChanged(Akonadi::Collection)),
            this, SLOT(processSelectionChange(Akonadi::Collection)));

    mMarkRules->setChecked(true);
    mMoveSpamRules->setChecked(true);
}

bool ASWizSpamRulesPage::markAsReadSelected() const
{
    return mMarkRules->isChecked();
}

bool ASWizSpamRulesPage::moveSpamSelected() const
{
    return mMoveSpamRules->isChecked();
}

bool ASWizSpamRulesPage::moveUnsureSelected() const
{
    return mMoveUnsureRules->isChecked();
}

QString ASWizSpamRulesPage::selectedSpamCollectionId() const
{
    return QString::number(selectedSpamCollection().id());
}

QString ASWizSpamRulesPage::selectedSpamCollectionName() const
{
    return selectedSpamCollection().name();
}

Akonadi::Collection ASWizSpamRulesPage::selectedSpamCollection() const
{
    if (mFolderReqForSpamFolder->hasCollection()) {
        return mFolderReqForSpamFolder->collection();
    } else {
        return CommonKernel->trashCollectionFolder();
    }
}

Akonadi::Collection ASWizSpamRulesPage::selectedUnsureCollection() const
{
    if (mFolderReqForUnsureFolder->hasCollection()) {
        return mFolderReqForUnsureFolder->collection();
    } else {
        return CommonKernel->inboxCollectionFolder();
    }
}

QString ASWizSpamRulesPage::selectedUnsureCollectionName() const
{
    return selectedUnsureCollection().name();
}

QString ASWizSpamRulesPage::selectedUnsureCollectionId() const
{
    return QString::number(selectedUnsureCollection().id());
}

void ASWizSpamRulesPage::processSelectionChange()
{
    mFolderReqForSpamFolder->setEnabled(mMoveSpamRules->isChecked());
    mFolderReqForUnsureFolder->setEnabled(mMoveUnsureRules->isChecked());
    Q_EMIT selectionChanged();
}

void ASWizSpamRulesPage::processSelectionChange(const Akonadi::Collection &)
{
    processSelectionChange();
}

void ASWizSpamRulesPage::allowUnsureFolderSelection(bool enabled)
{
    mMoveUnsureRules->setEnabled(enabled);
    mMoveUnsureRules->setVisible(enabled);
    mFolderReqForUnsureFolder->setEnabled(enabled);
    mFolderReqForUnsureFolder->setVisible(enabled);
}

void ASWizSpamRulesPage::allowMoveSpam(bool enabled)
{
    mMarkRules->setEnabled(enabled);
    mMarkRules->setChecked(enabled);
    mMoveSpamRules->setEnabled(enabled);
    mMoveSpamRules->setChecked(enabled);
}

//---------------------------------------------------------------------------
ASWizVirusRulesPage::ASWizVirusRulesPage(QWidget *parent, const QString &name)
    : ASWizPage(parent, name)
{
    QGridLayout *grid = new QGridLayout();
    mLayout->addItem(grid);

    mPipeRules = new QCheckBox(i18n("Check messages using the anti-virus tools"), this);
    mPipeRules->setWhatsThis(
        i18n("Let the anti-virus tools check your messages. The wizard "
             "will create appropriate filters. The messages are usually "
             "marked by the tools so that following filters can react "
             "on this and, for example, move virus messages to a special folder."));
    grid->addWidget(mPipeRules, 0, 0);

    mMoveRules = new QCheckBox(i18n("Move detected viral messages to the selected folder"), this);
    mMoveRules->setWhatsThis(
        i18n("A filter to detect messages classified as virus-infected and to move "
             "those messages into a predefined folder is created. The "
             "default folder is the trash folder, but you may change that "
             "in the folder view."));
    grid->addWidget(mMoveRules, 1, 0);

    mMarkRules = new QCheckBox(i18n("Additionally, mark detected viral messages as read"), this);
    mMarkRules->setEnabled(false);
    mMarkRules->setWhatsThis(
        i18n("Mark messages which have been classified as "
             "virus-infected as read, as well as moving them "
             "to the selected folder."));
    grid->addWidget(mMarkRules, 2, 0);
    FolderTreeWidget::TreeViewOptions opt = FolderTreeWidget::None;
    opt |= FolderTreeWidget::UseDistinctSelectionModel;

    FolderTreeWidgetProxyModel::FolderTreeWidgetProxyModelOptions optReadableProxy = FolderTreeWidgetProxyModel::None;
    optReadableProxy |= FolderTreeWidgetProxyModel::HideVirtualFolder;
    optReadableProxy |= FolderTreeWidgetProxyModel::HideOutboxFolder;

    mFolderTree = new FolderTreeWidget(this, Q_NULLPTR, opt, optReadableProxy);
    mFolderTree->readConfig();
    mFolderTree->folderTreeView()->expandAll();
    mFolderTree->folderTreeWidgetProxyModel()->setAccessRights(Akonadi::Collection::CanCreateCollection);

    mFolderTree->selectCollectionFolder(CommonKernel->trashCollectionFolder());
    mFolderTree->folderTreeView()->setDragDropMode(QAbstractItemView::NoDragDrop);

    mFolderTree->disableContextMenuAndExtraColumn();
    grid->addWidget(mFolderTree, 3, 0);

    connect(mPipeRules, &QCheckBox::clicked, this, &ASWizVirusRulesPage::processSelectionChange);
    connect(mMoveRules, &QCheckBox::clicked, this, &ASWizVirusRulesPage::processSelectionChange);
    connect(mMarkRules, &QCheckBox::clicked, this, &ASWizVirusRulesPage::processSelectionChange);
    connect(mMoveRules, &QCheckBox::toggled, mMarkRules, &QCheckBox::setEnabled);

}

bool ASWizVirusRulesPage::pipeRulesSelected() const
{
    return mPipeRules->isChecked();
}

bool ASWizVirusRulesPage::moveRulesSelected() const
{
    return mMoveRules->isChecked();
}

bool ASWizVirusRulesPage::markReadRulesSelected() const
{
    return mMarkRules->isChecked();
}

QString ASWizVirusRulesPage::selectedFolderName() const
{
    if (mFolderTree->selectedCollection().isValid()) {
        return QString::number(mFolderTree->selectedCollection().id());
    } else {
        return QString::number(CommonKernel->trashCollectionFolder().id());
    }
}

void ASWizVirusRulesPage::processSelectionChange()
{
    Q_EMIT selectionChanged();
}

//---------------------------------------------------------------------------
ASWizSummaryPage::ASWizSummaryPage(QWidget *parent, const QString &name)
    : ASWizPage(parent, name)
{
    QBoxLayout *layout = new QVBoxLayout();
    mLayout->addItem(layout);

    mSummaryText = new QLabel(this);
    layout->addWidget(mSummaryText);
    layout->addStretch();
}

void ASWizSummaryPage::setSummaryText(const QString &text)
{
    mSummaryText->setText(text);
}

