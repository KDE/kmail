/*
    This file is part of KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2002 Don Sanders <sanders@kde.org>
    SPDX-FileCopyrightText: 2011-2024 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

//
// A toplevel KMainWindow derived class for displaying
// single messages or single message parts.
//
// Could be extended to include support for normal main window
// widgets like a toolbar.

#include "kmreadermainwin.h"
using namespace Qt::Literals::StringLiterals;

#include "historyclosedreader/historyclosedreadermanager.h"
#include "job/composenewmessagejob.h"
#include "kmmainwidget.h"
#include "kmreaderwin.h"
#include "widgets/zoomlabelwidget.h"

#include "kmcommands.h"
#include "messageactions.h"
#include "util.h"
#include <KAcceleratorManager>
#include <KActionMenu>
#include <KEditToolBar>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>
#include <KStandardShortcut>
#include <KToolBar>
#include <MailCommon/FolderSettings>
#include <MailCommon/MailKernel>
#include <MessageViewer/HeaderStyle>
#include <MessageViewer/HeaderStylePlugin>
#include <MessageViewer/MessageViewerSettings>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <TemplateParser/CustomTemplatesMenu>

#include "tag/tagactionmanager.h"
#include "tag/tagselectdialog.h"
#include <Akonadi/ContactSearchJob>
#include <KActionCollection>
#include <KEmailAddress>
#include <WebEngineViewer/WebHitTestResult>

#include <Akonadi/ItemCopyJob>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemMoveJob>
#include <Akonadi/MessageFlags>
#include <MailCommon/MailUtil>
#include <MessageViewer/DKIMViewerMenu>
#include <MessageViewer/DKIMWidgetInfo>
#include <MessageViewer/RemoteContentMenu>
using namespace MailCommon;

KMReaderMainWin::KMReaderMainWin(MessageViewer::Viewer::DisplayFormatMessage format, bool htmlLoadExtDefault, const QString &name)
    : KMReaderMainWin(name)
{
    mReaderWin->setDisplayFormatMessageOverwrite(format);
    mReaderWin->setHtmlLoadExtDefault(htmlLoadExtDefault);
    mReaderWin->setDecryptMessageOverwrite(true);
}

KMReaderMainWin::KMReaderMainWin(const QString &name)
    : KMail::SecondaryWindow(!name.isEmpty() ? name : QStringLiteral("readerwindow#"))
    , mReaderWin(new KMReaderWin(this, this, actionCollection()))
{
    initKMReaderMainWin();
}

KMReaderMainWin::KMReaderMainWin(KMime::Content *aMsgPart, MessageViewer::Viewer::DisplayFormatMessage format, const QString &encoding, const QString &name)
    : KMReaderMainWin(name)
{
    mReaderWin->setOverrideEncoding(encoding);
    mReaderWin->setDisplayFormatMessageOverwrite(format);
    mReaderWin->setMsgPart(aMsgPart);
}

void KMReaderMainWin::initKMReaderMainWin()
{
    setCentralWidget(mReaderWin);
    setupAccel();
    setupGUI(Keys | StatusBar | Create, QStringLiteral("kmreadermainwin.rc"));
    mMsgActions->setupForwardingActionsList(this);

    setStateConfigGroup(QStringLiteral("Separate Reader Window"));
    setAutoSaveSettings(stateConfigGroup(), true);

    mZoomLabelIndicator = new ZoomLabelWidget(statusBar());
    statusBar()->addPermanentWidget(mZoomLabelIndicator);
    setZoomChanged(mReaderWin->viewer()->webViewZoomFactor());
    statusBar()->addPermanentWidget(mReaderWin->viewer()->dkimWidgetInfo());
    if (!mReaderWin->messageItem().isValid()) {
        menuBar()->hide();
        toolBar(QStringLiteral("mainToolBar"))->hide();
    } else {
        slotToggleMenubar(true);
    }
    connect(kmkernel, &KMKernel::configChanged, this, &KMReaderMainWin::slotConfigChanged);
    connect(mReaderWin, &KMReaderWin::showStatusBarMessage, this, &KMReaderMainWin::slotShowMessageStatusBar);
    connect(mReaderWin, &KMReaderWin::zoomChanged, this, &KMReaderMainWin::setZoomChanged);
    connect(mReaderWin, &KMReaderWin::showPreviousMessage, this, &KMReaderMainWin::showPreviousMessage);
    connect(mReaderWin, &KMReaderWin::showNextMessage, this, &KMReaderMainWin::showNextMessage);
}

KMReaderMainWin::~KMReaderMainWin()
{
    KConfigGroup grp(KSharedConfig::openConfig(QStringLiteral("kmail2rc"))->group(QStringLiteral("Separate Reader Window")));
    saveMainWindowSettings(grp);
    if (mMsg.isValid()) {
        HistoryClosedReaderInfo info;
        info.setItem(mMsg.id());
        KMime::Message::Ptr message = MessageComposer::Util::message(mMsg);
        if (message) {
            if (auto subject = message->subject(false)) {
                info.setSubject(subject->asUnicodeString());
            }
        }
        HistoryClosedReaderManager::self()->addInfo(std::move(info));
    }
}

void KMReaderMainWin::setZoomChanged(qreal zoomFactor)
{
    if (mZoomLabelIndicator) {
        mZoomLabelIndicator->setZoom(zoomFactor);
    }
}

void KMReaderMainWin::slotShowMessageStatusBar(const QString &msg)
{
    statusBar()->showMessage(msg);
}

void KMReaderMainWin::setUseFixedFont(bool useFixedFont)
{
    mReaderWin->setUseFixedFont(useFixedFont);
}

MessageViewer::Viewer *KMReaderMainWin::viewer() const
{
    return mReaderWin->viewer();
}

void KMReaderMainWin::showMessage(const QString &encoding, const Akonadi::Item &msg, const Akonadi::Collection &parentCollection)
{
    mParentCollection = parentCollection;
    mReaderWin->setOverrideEncoding(encoding);
    mReaderWin->setMessage(msg, MimeTreeParser::Force);
    KMime::Message::Ptr message = MessageComposer::Util::message(msg);
    QString caption;
    if (message) {
        if (auto subject = message->subject(false)) {
            caption = subject->asUnicodeString();
        }
    }
    if (mParentCollection.isValid()) {
        caption += " - "_L1;
        caption += MailCommon::Util::fullCollectionPath(mParentCollection);
    }
    if (!caption.isEmpty()) {
        setCaption(caption);
    }
    mMsg = msg;
    mMsgActions->setCurrentMessage(msg);
    mAkonadiStandardActionManager->setItems({mMsg});

    const bool canChange = mParentCollection.isValid() ? static_cast<bool>(mParentCollection.rights() & Akonadi::Collection::CanDeleteItem) : false;
    mTrashAction->setEnabled(canChange);

    if (mReaderWin->viewer() && mReaderWin->viewer()->headerStylePlugin() && mReaderWin->viewer()->headerStylePlugin()->headerStyle()) {
        mReaderWin->viewer()->headerStylePlugin()->headerStyle()->setReadOnlyMessage(!canChange);
    }

    const bool isInTrashFolder = mParentCollection.isValid() ? CommonKernel->folderIsTrash(mParentCollection) : false;
    QAction *moveToTrash = actionCollection()->action(QStringLiteral("move_to_trash"));
    KMail::Util::setActionTrashOrDelete(moveToTrash, isInTrashFolder);
    updateActions();
}

void KMReaderMainWin::updateButtons()
{
    if (mListMessage.count() <= 1) {
        return;
    }
    mReaderWin->updateShowMultiMessagesButton((mCurrentMessageIndex > 0), (mCurrentMessageIndex < (mListMessage.count() - 1)));
}

void KMReaderMainWin::showNextMessage()
{
    if (mCurrentMessageIndex >= (mListMessage.count() - 1)) {
        return;
    }
    mCurrentMessageIndex++;
    initializeMessage(mListMessage.at(mCurrentMessageIndex));
    updateButtons();
}

void KMReaderMainWin::showPreviousMessage()
{
    if (mCurrentMessageIndex <= 0) {
        return;
    }
    mCurrentMessageIndex--;
    initializeMessage(mListMessage.at(mCurrentMessageIndex));
    updateButtons();
}

void KMReaderMainWin::showMessage(const QString &encoding, const QList<KMime::Message::Ptr> &message)
{
    if (message.isEmpty()) {
        return;
    }

    mListMessage = message;
    mReaderWin->setOverrideEncoding(encoding);
    mCurrentMessageIndex = 0;
    initializeMessage(mListMessage.at(mCurrentMessageIndex));
    mReaderWin->hasMultiMessages(message.count() > 1);
    mAkonadiStandardActionManager->setItems({});
    updateButtons();
}

void KMReaderMainWin::initializeMessage(const KMime::Message::Ptr &message)
{
    Akonadi::Item item;
    item.setPayload<KMime::Message::Ptr>(message);
    Akonadi::MessageFlags::copyMessageFlags(*message, item);
    item.setMimeType(KMime::Message::mimeType());

    mMsg = item;
    mMsgActions->setCurrentMessage(item);

    mReaderWin->setMessage(message);
    if (auto subject = message->subject(false)) {
        setCaption(subject->asUnicodeString());
    }
    mTrashAction->setEnabled(false);
    mAkonadiStandardActionManager->setItems({mMsg});
    updateActions();
}

void KMReaderMainWin::showMessage(const QString &encoding, const KMime::Message::Ptr &message)
{
    if (!message) {
        return;
    }
    const QList<KMime::Message::Ptr> lst{message};
    showMessage(encoding, lst);
}

void KMReaderMainWin::updateActions()
{
    if (mHideMenuBarAction->isChecked()) {
        menuBar()->show();
    }
    toolBar(QStringLiteral("mainToolBar"))->show();
    if (mMsg.isValid()) {
        mTagActionManager->updateActionStates(1, mMsg);
    }
}

void KMReaderMainWin::slotReplyOrForwardFinished()
{
    if (MessageViewer::MessageViewerSettings::self()->closeAfterReplyOrForward()) {
        close();
    }
}

void KMReaderMainWin::slotSelectMoreMessageTagList()
{
    const Akonadi::Item::List selectedMessages = {mMsg};
    if (selectedMessages.isEmpty()) {
        return;
    }

    QPointer<TagSelectDialog> dlg = new TagSelectDialog(this, selectedMessages.count(), selectedMessages.first());
    dlg->setActionCollection(QList<KActionCollection *>{actionCollection()});
    if (dlg->exec()) {
        const Akonadi::Tag::List lst = dlg->selectedTag();
        KMCommand *command = new KMSetTagCommand(lst, selectedMessages, KMSetTagCommand::CleanExistingAndAddNew);
        command->start();
    }
    delete dlg;
}

void KMReaderMainWin::slotUpdateMessageTagList(const Akonadi::Tag &tag)
{
    // Create a persistent set from the current thread.
    const Akonadi::Item::List selectedMessages = {mMsg};
    if (selectedMessages.isEmpty()) {
        return;
    }
    toggleMessageSetTag(selectedMessages, tag);
}

void KMReaderMainWin::toggleMessageSetTag(const Akonadi::Item::List &select, const Akonadi::Tag &tag)
{
    if (select.isEmpty()) {
        return;
    }
    KMCommand *command = new KMSetTagCommand(Akonadi::Tag::List() << tag, select, KMSetTagCommand::Toggle);
    command->start();
}

Akonadi::Collection KMReaderMainWin::parentCollection() const
{
    if (mParentCollection.isValid()) {
        return mParentCollection;
    } else {
        return mMsg.parentCollection();
    }
}

void KMReaderMainWin::slotTrashMessage()
{
    if (!mMsg.isValid()) {
        return;
    }
    auto command = new KMTrashMsgCommand(parentCollection(), mMsg, -1);
    command->start();
    close();
}

void KMReaderMainWin::slotForwardInlineMsg()
{
    if (!mReaderWin->messageItem().hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    KMCommand *command = nullptr;
    const Akonadi::Collection parentCol = mReaderWin->messageItem().parentCollection();
    if (parentCol.isValid()) {
        QSharedPointer<FolderSettings> fd = FolderSettings::forCollection(parentCol, false);
        if (!fd.isNull()) {
            command = new KMForwardCommand(this, mReaderWin->messageItem(), fd->identity(), QString(), mReaderWin->copyText());
        } else {
            command = new KMForwardCommand(this, mReaderWin->messageItem(), 0, QString(), mReaderWin->copyText());
        }
    } else {
        command = new KMForwardCommand(this, mReaderWin->messageItem(), 0, QString(), mReaderWin->copyText());
    }
    connect(command, &KMTrashMsgCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);
    command->start();
}

void KMReaderMainWin::slotForwardAttachedMessage()
{
    if (!mReaderWin->messageItem().hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    KMCommand *command = nullptr;
    const Akonadi::Collection parentCol = mReaderWin->messageItem().parentCollection();
    if (parentCol.isValid()) {
        QSharedPointer<FolderSettings> fd = FolderSettings::forCollection(parentCol, false);
        if (!fd.isNull()) {
            command = new KMForwardAttachedCommand(this, mReaderWin->messageItem(), fd->identity());
        } else {
            command = new KMForwardAttachedCommand(this, mReaderWin->messageItem());
        }
    } else {
        command = new KMForwardAttachedCommand(this, mReaderWin->messageItem());
    }

    connect(command, &KMForwardAttachedCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);
    command->start();
}

void KMReaderMainWin::slotNewMessageToRecipients()
{
    auto job = new ComposeNewMessageJob;

    const Akonadi::Collection parentCol = mReaderWin->messageItem().parentCollection();
    if (parentCol.isValid()) {
        job->setCurrentCollection(parentCol);

        QSharedPointer<FolderSettings> fd = FolderSettings::forCollection(parentCol, false);
        if (!fd.isNull()) {
            job->setFolderSettings(fd);
        }
    }

    job->setRecipientsFromMessage(mReaderWin->messageItem());
    job->start();
}

void KMReaderMainWin::slotRedirectMessage()
{
    const Akonadi::Item currentItem = mReaderWin->messageItem();
    if (!currentItem.hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    auto command = new KMRedirectCommand(this, currentItem);
    connect(command, &KMRedirectCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);
    command->start();
}

void KMReaderMainWin::slotCustomReplyToMsg(const QString &tmpl)
{
    const Akonadi::Item currentItem = mReaderWin->messageItem();
    if (!currentItem.hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    auto command = new KMReplyCommand(this, currentItem, MessageComposer::ReplySmart, mReaderWin->copyText(), false, tmpl);
    command->setReplyAsHtml(mReaderWin->htmlMail());
    connect(command, &KMReplyCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);
    command->start();
}

void KMReaderMainWin::slotCustomReplyAllToMsg(const QString &tmpl)
{
    const Akonadi::Item currentItem = mReaderWin->messageItem();
    if (!currentItem.hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    auto command = new KMReplyCommand(this, currentItem, MessageComposer::ReplyAll, mReaderWin->copyText(), false, tmpl);
    command->setReplyAsHtml(mReaderWin->htmlMail());
    connect(command, &KMReplyCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);

    command->start();
}

void KMReaderMainWin::slotCustomForwardMsg(const QString &tmpl)
{
    const Akonadi::Item currentItem = mReaderWin->messageItem();
    if (!currentItem.hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    auto command = new KMForwardCommand(this, currentItem, 0, tmpl, mReaderWin->copyText());
    connect(command, &KMForwardCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);

    command->start();
}

void KMReaderMainWin::slotConfigChanged()
{
    // readConfig();
    mMsgActions->setupForwardActions(actionCollection());
    mMsgActions->setupForwardingActionsList(this);
}

void KMReaderMainWin::initializeAkonadiStandardAction()
{
    const auto mailActions = {Akonadi::StandardMailActionManager::MarkAllMailAsRead,
                              Akonadi::StandardMailActionManager::MarkMailAsRead,
                              Akonadi::StandardMailActionManager::MarkMailAsUnread,
                              Akonadi::StandardMailActionManager::MarkMailAsImportant,
                              Akonadi::StandardMailActionManager::MarkMailAsActionItem};

    for (const Akonadi::StandardMailActionManager::Type mailAction : mailActions) {
        QAction *act = mAkonadiStandardActionManager->createAction(mailAction);
        mAkonadiStandardActionManager->interceptAction(mailAction);
        connect(act, &QAction::triggered, this, &KMReaderMainWin::slotMarkMailAs);
    }
}

void KMReaderMainWin::slotMarkMailAs()
{
    const QAction *action = qobject_cast<QAction *>(sender());
    Q_ASSERT(action);

    const QByteArray typeStr = action->data().toByteArray();

    mAkonadiStandardActionManager->markItemsAs(typeStr, {mMsgActions->currentItem()}, false);
}

void KMReaderMainWin::setupAccel()
{
    if (!kmkernel->xmlGuiInstanceName().isEmpty()) {
        setComponentName(kmkernel->xmlGuiInstanceName(), i18n("KMail2"));
    }
    mMsgActions = new KMail::MessageActions(actionCollection(), this);
    mAkonadiStandardActionManager = new Akonadi::StandardMailActionManager(actionCollection(), this);
    initializeAkonadiStandardAction();
    mMsgActions->fillAkonadiStandardAction(mAkonadiStandardActionManager);
    mMsgActions->setMessageView(mReaderWin);
    connect(mMsgActions, &KMail::MessageActions::replyActionFinished, this, &KMReaderMainWin::slotReplyOrForwardFinished);

    //----- File Menu

    mSaveAtmAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-attachment")), i18n("Save A&ttachments…"), actionCollection());
    connect(mSaveAtmAction, &QAction::triggered, mReaderWin->viewer(), &MessageViewer::Viewer::slotAttachmentSaveAll);

    mTrashAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("&Move to Trash"), this);
    mTrashAction->setIconText(i18nc("@action:intoolbar Move to Trash", "Trash"));
    KMail::Util::addQActionHelpText(mTrashAction, i18n("Move message to trashcan"));
    actionCollection()->addAction(QStringLiteral("move_to_trash"), mTrashAction);
    actionCollection()->setDefaultShortcut(mTrashAction, QKeySequence(Qt::Key_Delete));
    connect(mTrashAction, &QAction::triggered, this, &KMReaderMainWin::slotTrashMessage);

    QAction *closeAction = KStandardAction::close(this, &KMReaderMainWin::close, actionCollection());
    QList<QKeySequence> closeShortcut = closeAction->shortcuts();
    closeAction->setShortcuts(closeShortcut << QKeySequence(Qt::Key_Escape));

    mTagActionManager = new KMail::TagActionManager(this, actionCollection(), mMsgActions, this);
    connect(mTagActionManager, &KMail::TagActionManager::tagActionTriggered, this, &KMReaderMainWin::slotUpdateMessageTagList);

    connect(mTagActionManager, &KMail::TagActionManager::tagMoreActionClicked, this, &KMReaderMainWin::slotSelectMoreMessageTagList);

    mTagActionManager->createActions();
    if (mReaderWin->messageItem().isValid()) {
        mTagActionManager->updateActionStates(1, mReaderWin->messageItem());
    }
    mHideMenuBarAction = KStandardAction::showMenubar(this, &KMReaderMainWin::slotToggleMenubar, actionCollection());
    mHideMenuBarAction->setChecked(KMailSettings::self()->readerShowMenuBar());

    //----- Message Menu
    connect(mReaderWin->viewer(), &MessageViewer::Viewer::displayPopupMenu, this, &KMReaderMainWin::slotMessagePopup);

    connect(mReaderWin->viewer(), &MessageViewer::Viewer::itemRemoved, this, &QWidget::close);

    setStandardToolBarMenuEnabled(true);
    KStandardAction::configureToolbars(this, &KMReaderMainWin::slotEditToolbars, actionCollection());
    connect(mReaderWin->viewer(), &MessageViewer::Viewer::moveMessageToTrash, this, &KMReaderMainWin::slotTrashMessage);
}

void KMReaderMainWin::slotToggleMenubar(bool dontShowWarning)
{
    if (!mReaderWin->messageItem().isValid()) {
        return;
    }
    if (menuBar()) {
        if (mHideMenuBarAction->isChecked()) {
            menuBar()->show();
        } else {
            if (!dontShowWarning) {
                const QString accel = mHideMenuBarAction->shortcut().toString(QKeySequence::NativeText);
                KMessageBox::information(this,
                                         i18n("<qt>This will hide the menu bar completely."
                                              " You can show it again by typing %1.</qt>",
                                              accel),
                                         i18n("Hide menu bar"),
                                         QStringLiteral("HideMenuBarWarning"));
            }
            menuBar()->hide();
        }
        KMailSettings::self()->setReaderShowMenuBar(mHideMenuBarAction->isChecked());
    }
}

QAction *KMReaderMainWin::copyActionMenu(QMenu *menu)
{
    KMMainWidget *mainwin = kmkernel->getKMMainWidget();
    if (mainwin) {
        Akonadi::StandardActionManager *manager = mainwin->standardMailActionManager()->standardActionManager();
        const auto mainWinAction = manager->action(Akonadi::StandardActionManager::CopyItemToMenu);
        auto action = new KActionMenu(menu);
        action->setIcon(mainWinAction->icon());
        action->setText(mainWinAction->text());
        manager->createActionFolderMenu(action->menu(), Akonadi::StandardActionManager::CopyItemToMenu);
        connect(action->menu(), &QMenu::triggered, this, &KMReaderMainWin::slotCopyItem);
        return action;
    }
    return nullptr;
}

QAction *KMReaderMainWin::moveActionMenu(QMenu *menu)
{
    KMMainWidget *mainwin = kmkernel->getKMMainWidget();
    if (mainwin) {
        Akonadi::StandardActionManager *manager = mainwin->standardMailActionManager()->standardActionManager();
        const auto mainWinAction = manager->action(Akonadi::StandardActionManager::MoveItemToMenu);
        auto action = new KActionMenu(menu);
        action->setIcon(mainWinAction->icon());
        action->setText(mainWinAction->text());
        manager->createActionFolderMenu(action->menu(), Akonadi::StandardActionManager::MoveItemToMenu);
        connect(action->menu(), &QMenu::triggered, this, &KMReaderMainWin::slotMoveItem);
        return action;
    }
    return nullptr;
}

void KMReaderMainWin::slotMoveItem(QAction *action)
{
    if (action) {
        const QModelIndex index = action->data().toModelIndex();
        const auto collection = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        copyOrMoveItem(collection, true);
    }
}

void KMReaderMainWin::copyOrMoveItem(const Akonadi::Collection &collection, bool move)
{
    if (mMsg.isValid()) {
        if (move) {
            auto job = new Akonadi::ItemMoveJob(mMsg, collection, this);
            connect(job, &KJob::result, this, &KMReaderMainWin::slotCopyMoveResult);
        } else {
            auto job = new Akonadi::ItemCopyJob(mMsg, collection, this);
            connect(job, &KJob::result, this, &KMReaderMainWin::slotCopyMoveResult);
        }
    } else {
        auto job = new Akonadi::ItemCreateJob(mMsg, collection, this);
        connect(job, &KJob::result, this, &KMReaderMainWin::slotCopyMoveResult);
    }

    KMMainWidget *mainwin = kmkernel->getKMMainWidget();
    if (mainwin) {
        mainwin->standardMailActionManager()->standardActionManager()->addRecentCollection(collection.id());
    }
}

void KMReaderMainWin::slotCopyItem(QAction *action)
{
    if (action) {
        const QModelIndex index = action->data().toModelIndex();
        const auto collection = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        copyOrMoveItem(collection, false);
    }
}

void KMReaderMainWin::slotCopyMoveResult(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(this, i18n("Cannot copy item. %1", job->errorString()));
    }
}

void KMReaderMainWin::slotMessagePopup(const Akonadi::Item &aMsg, const WebEngineViewer::WebHitTestResult &result, const QPoint &aPoint)
{
    QUrl aUrl = result.linkUrl();
    QUrl imageUrl = result.imageUrl();
    mMsg = aMsg;

    const QString email = KEmailAddress::firstEmailAddress(aUrl.path()).toLower();
    if (aUrl.scheme() == "mailto"_L1 && !email.isEmpty()) {
        auto job = new Akonadi::ContactSearchJob(this);
        job->setLimit(1);
        job->setQuery(Akonadi::ContactSearchJob::Email, email, Akonadi::ContactSearchJob::ExactMatch);
        job->setProperty("msg", QVariant::fromValue(mMsg));
        job->setProperty("point", aPoint);
        job->setProperty("imageUrl", imageUrl);
        job->setProperty("url", aUrl);
        job->setProperty("webhitresult", QVariant::fromValue(result));
        connect(job, &Akonadi::ItemCopyJob::result, this, &KMReaderMainWin::slotContactSearchJobForMessagePopupDone);
    } else {
        showMessagePopup(mMsg, aUrl, imageUrl, aPoint, false, false, result);
    }
}

void KMReaderMainWin::slotContactSearchJobForMessagePopupDone(KJob *job)
{
    const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob *>(job);
    const bool contactAlreadyExists = !searchJob->contacts().isEmpty();

    const Akonadi::Item::List listContact = searchJob->items();
    const bool uniqueContactFound = (listContact.count() == 1);
    if (uniqueContactFound) {
        mReaderWin->setContactItem(listContact.first(), searchJob->contacts().at(0));
    } else {
        mReaderWin->clearContactItem();
    }

    const auto msg = job->property("msg").value<Akonadi::Item>();
    const QPoint aPoint = job->property("point").toPoint();
    const QUrl imageUrl = job->property("imageUrl").toUrl();
    const QUrl url = job->property("url").toUrl();
    const auto result = job->property("webhitresult").value<WebEngineViewer::WebHitTestResult>();

    showMessagePopup(msg, url, imageUrl, aPoint, contactAlreadyExists, uniqueContactFound, result);
}

void KMReaderMainWin::showMessagePopup(const Akonadi::Item &msg,
                                       const QUrl &url,
                                       const QUrl &imageUrl,
                                       const QPoint &aPoint,
                                       bool contactAlreadyExists,
                                       bool uniqueContactFound,
                                       const WebEngineViewer::WebHitTestResult &result)
{
    QMenu *menu = nullptr;

    bool urlMenuAdded = false;
    bool copyAdded = false;
    const bool messageHasPayload = msg.hasPayload<KMime::Message::Ptr>();
    if (!url.isEmpty()) {
        if (url.scheme() == "mailto"_L1) {
            // popup on a mailto URL
            menu = new QMenu(this);
            menu->addAction(mReaderWin->mailToComposeAction());
            if (messageHasPayload) {
                menu->addAction(mReaderWin->mailToReplyAction());
                menu->addAction(mReaderWin->mailToForwardAction());
                menu->addSeparator();
            }

            if (contactAlreadyExists) {
                if (uniqueContactFound) {
                    menu->addAction(mReaderWin->editContactAction());
                } else {
                    menu->addAction(mReaderWin->openAddrBookAction());
                }
            } else {
                menu->addAction(mReaderWin->addAddrBookAction());
                menu->addAction(mReaderWin->addToExistingContactAction());
            }
            menu->addSeparator();
            menu->addMenu(mReaderWin->viewHtmlOption());
            menu->addSeparator();
            menu->addAction(mReaderWin->copyURLAction());
            copyAdded = true;
            urlMenuAdded = true;
        } else if (url.scheme() != "attachment"_L1) {
            // popup on a not-mailto URL
            menu = new QMenu(this);
            menu->addAction(mReaderWin->urlOpenAction());
            menu->addAction(mReaderWin->addUrlToBookmarkAction());
            menu->addAction(mReaderWin->urlSaveAsAction());
            menu->addAction(mReaderWin->copyURLAction());
            menu->addSeparator();
            menu->addAction(mReaderWin->shareServiceUrlMenu());
            menu->addSeparator();
            menu->addActions(mReaderWin->viewerPluginActionList(MessageViewer::ViewerPluginInterface::NeedUrl));
            if (!imageUrl.isEmpty()) {
                menu->addSeparator();
                mReaderWin->addImageMenuActions(menu);
            }
            urlMenuAdded = true;
        }
    }
    const QString selectedText(mReaderWin->copyText());
    if (!selectedText.isEmpty()) {
        if (!menu) {
            menu = new QMenu(this);
        }
        if (urlMenuAdded) {
            menu->addSeparator();
        }
        if (messageHasPayload) {
            menu->addAction(mMsgActions->replyMenu());
            menu->addSeparator();
            menu->addAction(mMsgActions->mailingListActionMenu());
            menu->addSeparator();
        }
        if (!copyAdded) {
            menu->addAction(mReaderWin->copyAction());
        }
        menu->addAction(mReaderWin->selectAllAction());
        menu->addSeparator();
        mMsgActions->addWebShortcutsMenu(menu, selectedText);
        menu->addSeparator();
        menu->addActions(mReaderWin->viewerPluginActionList(MessageViewer::ViewerPluginInterface::NeedSelection));
#ifdef HAVE_TEXT_TO_SPEECH_SUPPORT
        menu->addSeparator();
        menu->addAction(mReaderWin->speakTextAction());
#endif
        menu->addSeparator();
        menu->addAction(mReaderWin->shareTextAction());
    } else if (!urlMenuAdded) {
        if (!menu) {
            menu = new QMenu(this);
        }

        // popup somewhere else (i.e., not a URL) on the message
        if (messageHasPayload) {
            bool replyForwardMenu = false;
            Akonadi::Collection col = parentCollection();
            if (col.isValid()) {
                if (!(CommonKernel->folderIsSentMailFolder(col) || CommonKernel->folderIsDrafts(col) || CommonKernel->folderIsTemplates(col))) {
                    replyForwardMenu = true;
                }
            } else if (messageHasPayload) {
                replyForwardMenu = true;
            }

            if (replyForwardMenu) {
                // add the reply and forward actions only if we are not in a sent-mail,
                // templates or drafts folder
                menu->addAction(mMsgActions->replyMenu());
                menu->addAction(mMsgActions->forwardMenu());
                menu->addSeparator();
            } else if (col.isValid() && CommonKernel->folderIsTemplates(col)) {
                menu->addAction(mMsgActions->newMessageFromTemplateAction());
            }

            if (col.isValid() && CommonKernel->folderIsSentMailFolder(col)) {
                menu->addAction(mMsgActions->sendAgainAction());
                menu->addSeparator();
            }

            menu->addAction(copyActionMenu(menu));
            if (col.isValid()) {
                menu->addAction(moveActionMenu(menu));
            }
            menu->addSeparator();
            menu->addAction(mMsgActions->mailingListActionMenu());

            menu->addSeparator();
            if (!imageUrl.isEmpty()) {
                menu->addSeparator();
                mReaderWin->addImageMenuActions(menu);
                menu->addSeparator();
            }

            menu->addAction(mMsgActions->printPreviewAction());
            menu->addAction(mMsgActions->printAction());
            menu->addSeparator();
            menu->addAction(mReaderWin->saveAsAction());
            menu->addSeparator();
            menu->addAction(mMsgActions->exportToPdfAction());
            menu->addSeparator();
            menu->addAction(mSaveAtmAction);
            if (msg.isValid()) {
                if (mReaderWin->dkimViewerMenu()) {
                    menu->addSeparator();
                    menu->addMenu(mReaderWin->dkimViewerMenu()->menu());
                }
                menu->addSeparator();
                if (mReaderWin->remoteContentMenu()) {
                    menu->addMenu(mReaderWin->remoteContentMenu());
                    menu->addSeparator();
                }
                menu->addActions(mReaderWin->viewerPluginActionList(MessageViewer::ViewerPluginInterface::NeedMessage));
            }
        } else {
            menu->addAction(mReaderWin->toggleFixFontAction());
            if (!mReaderWin->mimePartTreeIsEmpty()) {
                menu->addAction(mReaderWin->toggleMimePartTreeAction());
            }
        }
        if (msg.isValid()) {
            menu->addSeparator();
            menu->addAction(mMsgActions->addFollowupReminderAction());
        }
        if (kmkernel->allowToDebug()) {
            menu->addSeparator();
            menu->addAction(mMsgActions->debugAkonadiSearchAction());
            menu->addSeparator();
            menu->addAction(mReaderWin->developmentToolsAction());
        }
    }
    const QList<QAction *> interceptorUrlActions = mReaderWin->interceptorUrlActions(result);
    if (!interceptorUrlActions.isEmpty()) {
        menu->addSeparator();
        menu->addActions(interceptorUrlActions);
    }

    if (menu) {
        KAcceleratorManager::manage(menu);
        menu->exec(aPoint, nullptr);
        delete menu;
    }
}

void KMReaderMainWin::showAndActivateWindow()
{
    show();
    raise();
    activateWindow();
}

void KMReaderMainWin::slotEditToolbars()
{
    KConfigGroup grp(KMKernel::self()->config(), QStringLiteral("ReaderWindow"));
    saveMainWindowSettings(grp);
    QPointer<KEditToolBar> dlg = new KEditToolBar(guiFactory(), this);
    connect(dlg.data(), &KEditToolBar::newToolBarConfig, this, &KMReaderMainWin::slotUpdateToolbars);
    dlg->exec();
    delete dlg;
}

void KMReaderMainWin::slotUpdateToolbars()
{
    createGUI(QStringLiteral("kmreadermainwin.rc"));
    applyMainWindowSettings(stateConfigGroup());
}

#include "moc_kmreadermainwin.cpp"
