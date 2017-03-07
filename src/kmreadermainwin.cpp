/*
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Don Sanders <sanders@kde.org>
    Copyright (c) 2011-2017 Montel Laurent <montel@kde.org>

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
*/
//
// A toplevel KMainWindow derived class for displaying
// single messages or single message parts.
//
// Could be extended to include support for normal main window
// widgets like a toolbar.

#include "kmreadermainwin.h"
#include "kmreaderwin.h"

#include <kactionmenu.h>
#include <kedittoolbar.h>
#include <KLocalizedString>
#include <kstandardshortcut.h>
#include <QAction>
#include <kfontaction.h>
#include <kstandardaction.h>
#include <ktoggleaction.h>
#include <ktoolbar.h>
#include "kmail_debug.h"
#include <KFontAction>
#include <KFontSizeAction>
#include <QStatusBar>
#include <KMessageBox>
#include <KAcceleratorManager>
#include "kmcommands.h"
#include <QMenuBar>
#include <qmenu.h>
#include "kmmainwidget.h"
#include <MessageViewer/CSSHelper>
#include <TemplateParser/CustomTemplatesMenu>
#include "messageactions.h"
#include "util.h"
#include "mailcommon/mailkernel.h"
#include <MailCommon/FolderCollection>
#include "messageviewer/headerstyleplugin.h"
#include "messageviewer/headerstyle.h"

#include <KActionCollection>
#include <Akonadi/Contact/ContactSearchJob>
#include <KEmailAddress>
#include <WebEngineViewer/WebHitTestResult>
#include <kmime/kmime_message.h>

#include <messageviewer/viewer.h>
#include <AkonadiCore/item.h>
#include <AkonadiCore/itemcopyjob.h>
#include <AkonadiCore/itemcreatejob.h>
#include <AkonadiCore/itemmovejob.h>
#include <Akonadi/KMime/MessageFlags>
#include "kpimtextedit/texttospeech.h"
#include "messagecore/messagehelpers.h"
#include <mailcommon/mailutil.h>

using namespace MailCommon;

KMReaderMainWin::KMReaderMainWin(MessageViewer::Viewer::DisplayFormatMessage format, bool htmlLoadExtOverride,
                                 const QString &name)
    : KMail::SecondaryWindow(!name.isEmpty() ? name : QStringLiteral("readerwindow#"))
{
    mReaderWin = new KMReaderWin(this, this, actionCollection());
    mReaderWin->setDisplayFormatMessageOverwrite(format);
    mReaderWin->setHtmlLoadExtOverride(htmlLoadExtOverride);
    mReaderWin->setDecryptMessageOverwrite(true);
    initKMReaderMainWin();
}

KMReaderMainWin::KMReaderMainWin(const QString &name)
    : KMail::SecondaryWindow(!name.isEmpty() ? name : QStringLiteral("readerwindow#"))
{
    mReaderWin = new KMReaderWin(this, this, actionCollection());
    initKMReaderMainWin();
}

KMReaderMainWin::KMReaderMainWin(KMime::Content *aMsgPart, MessageViewer::Viewer::DisplayFormatMessage format, const QString &encoding, const QString &name)
    : KMail::SecondaryWindow(!name.isEmpty() ? name : QStringLiteral("readerwindow#"))
{
    mReaderWin = new KMReaderWin(this, this, actionCollection());
    mReaderWin->setOverrideEncoding(encoding);
    mReaderWin->setDisplayFormatMessageOverwrite(format);
    mReaderWin->setMsgPart(aMsgPart);
    initKMReaderMainWin();
}

void KMReaderMainWin::initKMReaderMainWin()
{
    setCentralWidget(mReaderWin);
    setupAccel();
    setupGUI(Keys | StatusBar | Create, QStringLiteral("kmreadermainwin.rc"));
    mMsgActions->setupForwardingActionsList(this);
    applyMainWindowSettings(KMKernel::self()->config()->group("Separate Reader Window"));
    if (! mReaderWin->message().isValid()) {
        menuBar()->hide();
        toolBar(QStringLiteral("mainToolBar"))->hide();
    }
    connect(kmkernel, &KMKernel::configChanged, this, &KMReaderMainWin::slotConfigChanged);
    connect(mReaderWin, &KMReaderWin::showStatusBarMessage, this, &KMReaderMainWin::slotShowMessageStatusBar);
}

KMReaderMainWin::~KMReaderMainWin()
{
    KConfigGroup grp(KMKernel::self()->config()->group("Separate Reader Window"));
    saveMainWindowSettings(grp);
}

void KMReaderMainWin::slotShowMessageStatusBar(const QString &msg)
{
    statusBar()->showMessage(msg);
}

void KMReaderMainWin::setUseFixedFont(bool useFixedFont)
{
    mReaderWin->setUseFixedFont(useFixedFont);
}

void KMReaderMainWin::showMessage(const QString &encoding, const Akonadi::Item &msg, const Akonadi::Collection &parentCollection)
{

    mParentCollection = parentCollection;
    mReaderWin->setOverrideEncoding(encoding);
    mReaderWin->setMessage(msg, MimeTreeParser::Force);
    KMime::Message::Ptr message = MessageCore::Util::message(msg);
    QString caption;
    if (message) {
        caption = message->subject()->asUnicodeString();
    }
    if (mParentCollection.isValid()) {
        caption += QLatin1String(" - ");
        caption += MailCommon::Util::fullCollectionPath(mParentCollection);
    }
    if (!caption.isEmpty()) {
        setCaption(caption);
    }
    mMsg = msg;
    mMsgActions->setCurrentMessage(msg);

    const bool canChange = mParentCollection.isValid() ? (bool)(mParentCollection.rights() & Akonadi::Collection::CanDeleteItem) : false;
    mTrashAction->setEnabled(canChange);

    if (mReaderWin->viewer() && mReaderWin->viewer()->headerStylePlugin() && mReaderWin->viewer()->headerStylePlugin()->headerStyle()) {
        mReaderWin->viewer()->headerStylePlugin()->headerStyle()->setReadOnlyMessage(!canChange);
    }

    menuBar()->show();
    toolBar(QStringLiteral("mainToolBar"))->show();
}

void KMReaderMainWin::showMessage(const QString &encoding, const KMime::Message::Ptr &message)
{
    if (!message) {
        return;
    }

    Akonadi::Item item;

    item.setPayload<KMime::Message::Ptr>(message);
    Akonadi::MessageFlags::copyMessageFlags(*message, item);
    item.setMimeType(KMime::Message::mimeType());

    mMsg = item;
    mMsgActions->setCurrentMessage(item);

    mReaderWin->setOverrideEncoding(encoding);
    mReaderWin->setMessage(message);
    setCaption(message->subject()->asUnicodeString());

    mTrashAction->setEnabled(false);

    menuBar()->show();
    toolBar(QStringLiteral("mainToolBar"))->show();
}

void KMReaderMainWin::slotReplyOrForwardFinished()
{
    if (KMailSettings::self()->closeAfterReplyOrForward()) {
        close();
    }
}

Akonadi::Collection KMReaderMainWin::parentCollection() const
{
    if (mParentCollection.isValid()) {
        return mParentCollection;
    } else {
        return mMsg.parentCollection();
    }
}

void KMReaderMainWin::slotExecuteMailAction(MessageViewer::Viewer::MailAction action)
{
    switch (action) {
    case MessageViewer::Viewer::Trash:
        slotTrashMessage();
        break;
    case MessageViewer::Viewer::Reply:
        slotCustomReplyToMsg(QString());
        break;
    case MessageViewer::Viewer::ReplyToAll:
        slotCustomReplyAllToMsg(QString());
        break;
    case MessageViewer::Viewer::Forward:
        slotRedirectMessage();
        break;
    case MessageViewer::Viewer::NewMessage:
        break;
    case MessageViewer::Viewer::Print:
        break;
    case MessageViewer::Viewer::PrintPreview:
        break;
    }
}

void KMReaderMainWin::slotTrashMessage()
{
    if (!mMsg.isValid()) {
        return;
    }
    KMTrashMsgCommand *command = new KMTrashMsgCommand(parentCollection(), mMsg, -1);
    command->start();
    close();
}

void KMReaderMainWin::slotForwardInlineMsg()
{
    if (!mReaderWin->message().hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    KMCommand *command = nullptr;
    const Akonadi::Collection parentCol = mReaderWin->message().parentCollection();
    if (parentCol.isValid()) {
        QSharedPointer<FolderCollection> fd = FolderCollection::forCollection(parentCol, false);
        if (fd)
            command = new KMForwardCommand(this, mReaderWin->message(),
                                           fd->identity(), QString(), mReaderWin->copyText());
        else {
            command = new KMForwardCommand(this, mReaderWin->message(), 0, QString(), mReaderWin->copyText());
        }
    } else {
        command = new KMForwardCommand(this, mReaderWin->message(), 0, QString(), mReaderWin->copyText());
    }
    connect(command, &KMTrashMsgCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);
    command->start();
}

void KMReaderMainWin::slotForwardAttachedMessage()
{
    if (!mReaderWin->message().hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    KMCommand *command = nullptr;
    const Akonadi::Collection parentCol = mReaderWin->message().parentCollection();
    if (parentCol.isValid()) {
        QSharedPointer<FolderCollection> fd = FolderCollection::forCollection(parentCol, false);
        if (fd)
            command = new KMForwardAttachedCommand(this, mReaderWin->message(),
                                                   fd->identity());
        else {
            command = new KMForwardAttachedCommand(this, mReaderWin->message());
        }
    } else {
        command = new KMForwardAttachedCommand(this, mReaderWin->message());
    }

    connect(command, &KMTrashMsgCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);
    command->start();
}

void KMReaderMainWin::slotRedirectMessage()
{
    const Akonadi::Item currentItem  = mReaderWin->message();
    if (!currentItem.hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    KMRedirectCommand *command = new KMRedirectCommand(this, currentItem);
    connect(command, &KMRedirectCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);
    command->start();
}

void KMReaderMainWin::slotCustomReplyToMsg(const QString &tmpl)
{
    const Akonadi::Item currentItem  = mReaderWin->message();
    if (!currentItem.hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    KMReplyCommand *command = new KMReplyCommand(this,
                                            currentItem,
                                            MessageComposer::ReplySmart,
                                            mReaderWin->copyText(),
                                            false, tmpl);
    connect(command, &KMReplyCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);
    command->start();
}

void KMReaderMainWin::slotCustomReplyAllToMsg(const QString &tmpl)
{
    const Akonadi::Item currentItem  = mReaderWin->message();
    if (!currentItem.hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    KMReplyCommand *command = new KMReplyCommand(this,
                                            currentItem,
                                            MessageComposer::ReplyAll,
                                            mReaderWin->copyText(),
                                            false, tmpl);
    connect(command, &KMReplyCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);

    command->start();
}

void KMReaderMainWin::slotCustomForwardMsg(const QString &tmpl)
{
    const Akonadi::Item currentItem  = mReaderWin->message();
    if (!currentItem.hasPayload<KMime::Message::Ptr>()) {
        return;
    }
    KMForwardCommand *command = new KMForwardCommand(this,
            currentItem,
            0, tmpl, mReaderWin->copyText());
    connect(command, &KMForwardCommand::completed, this, &KMReaderMainWin::slotReplyOrForwardFinished);

    command->start();
}

void KMReaderMainWin::slotConfigChanged()
{
    //readConfig();
    mMsgActions->setupForwardActions(actionCollection());
    mMsgActions->setupForwardingActionsList(this);
}

void KMReaderMainWin::setupAccel()
{
    if (!kmkernel->xmlGuiInstanceName().isEmpty()) {
        setComponentName(kmkernel->xmlGuiInstanceName(), i18n("kmail2"));
    }
    mMsgActions = new KMail::MessageActions(actionCollection(), this);
    mMsgActions->setMessageView(mReaderWin);
    connect(mMsgActions, &KMail::MessageActions::replyActionFinished, this, &KMReaderMainWin::slotReplyOrForwardFinished);

    //----- File Menu

    mSaveAtmAction  = new QAction(QIcon::fromTheme(QStringLiteral("mail-attachment")), i18n("Save A&ttachments..."), actionCollection());
    connect(mSaveAtmAction, &QAction::triggered, mReaderWin->viewer(), &MessageViewer::Viewer::slotAttachmentSaveAll);

    mTrashAction = new QAction(QIcon::fromTheme(QStringLiteral("user-trash")), i18n("&Move to Trash"), this);
    mTrashAction->setIconText(i18nc("@action:intoolbar Move to Trash", "Trash"));
    KMail::Util::addQActionHelpText(mTrashAction, i18n("Move message to trashcan"));
    actionCollection()->addAction(QStringLiteral("move_to_trash"), mTrashAction);
    actionCollection()->setDefaultShortcut(mTrashAction, QKeySequence(Qt::Key_Delete));
    connect(mTrashAction, &QAction::triggered, this, &KMReaderMainWin::slotTrashMessage);

    QAction *closeAction = KStandardAction::close(this, &KMReaderMainWin::close, actionCollection());
    QList<QKeySequence> closeShortcut = closeAction->shortcuts();
    closeAction->setShortcuts(closeShortcut << QKeySequence(Qt::Key_Escape));

    //----- Message Menu
    connect(mReaderWin->viewer(), &MessageViewer::Viewer::displayPopupMenu, this, &KMReaderMainWin::slotMessagePopup);

    connect(mReaderWin->viewer(), &MessageViewer::Viewer::itemRemoved, this, &QWidget::close);

    setStandardToolBarMenuEnabled(true);
    KStandardAction::configureToolbars(this, &KMReaderMainWin::slotEditToolbars, actionCollection());
    connect(mReaderWin->viewer(), &MessageViewer::Viewer::moveMessageToTrash, this, &KMReaderMainWin::slotTrashMessage);
    connect(mReaderWin->viewer(), &MessageViewer::Viewer::executeMailAction, this, &KMReaderMainWin::slotExecuteMailAction);
}

QAction *KMReaderMainWin::copyActionMenu(QMenu *menu)
{
    KMMainWidget *mainwin = kmkernel->getKMMainWidget();
    if (mainwin) {
        KActionMenu *action = new KActionMenu(menu);
        action->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
        action->setText(i18n("Copy Message To..."));
        mainwin->standardMailActionManager()->standardActionManager()->createActionFolderMenu(action->menu(), Akonadi::StandardActionManager::CopyItemToMenu);
        connect(action->menu(), &QMenu::triggered, this, &KMReaderMainWin::slotCopyItem);
        return action;
    }
    return nullptr;
}

QAction *KMReaderMainWin::moveActionMenu(QMenu *menu)
{
    KMMainWidget *mainwin = kmkernel->getKMMainWidget();
    if (mainwin) {
        KActionMenu *action = new KActionMenu(menu);
        action->setText(i18n("Move Message To..."));
        mainwin->standardMailActionManager()->standardActionManager()->createActionFolderMenu(action->menu(), Akonadi::StandardActionManager::MoveItemToMenu);
        connect(action->menu(), &QMenu::triggered, this, &KMReaderMainWin::slotMoveItem);

        return action;
    }
    return nullptr;

}

void KMReaderMainWin::slotMoveItem(QAction *action)
{
    if (action) {
        const QModelIndex index = action->data().toModelIndex();
        const Akonadi::Collection collection = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        copyOrMoveItem(collection, true);
    }
}

void KMReaderMainWin::copyOrMoveItem(const Akonadi::Collection &collection, bool move)
{
    if (mMsg.isValid()) {
        if (move) {
            Akonadi::ItemMoveJob *job = new Akonadi::ItemMoveJob(mMsg, collection, this);
            connect(job, &KJob::result, this, &KMReaderMainWin::slotCopyMoveResult);
        } else {
            Akonadi::ItemCopyJob *job = new Akonadi::ItemCopyJob(mMsg, collection, this);
            connect(job, &KJob::result, this, &KMReaderMainWin::slotCopyMoveResult);
        }
    } else {
        Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob(mMsg, collection, this);
        connect(job, &KJob::result, this, &KMReaderMainWin::slotCopyMoveResult);
    }
}

void KMReaderMainWin::slotCopyItem(QAction *action)
{
    if (action) {
        const QModelIndex index = action->data().toModelIndex();
        const Akonadi::Collection collection = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        copyOrMoveItem(collection, false);
    }
}

void KMReaderMainWin::slotCopyMoveResult(KJob *job)
{
    if (job->error()) {
        KMessageBox::sorry(this, i18n("Cannot copy item. %1", job->errorString()));
    }
}

void KMReaderMainWin::slotMessagePopup(const Akonadi::Item &aMsg, const WebEngineViewer::WebHitTestResult &result, const QPoint &aPoint)
{
    QUrl aUrl = result.linkUrl();
    QUrl imageUrl = result.imageUrl();
    mMsg = aMsg;

    const QString email =  KEmailAddress::firstEmailAddress(aUrl.path()).toLower();
    if (aUrl.scheme() == QLatin1String("mailto") && !email.isEmpty()) {
        Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob(this);
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

    const Akonadi::Item msg = job->property("msg").value<Akonadi::Item>();
    const QPoint aPoint = job->property("point").toPoint();
    const QUrl imageUrl = job->property("imageUrl").toUrl();
    const QUrl url = job->property("url").toUrl();
    const WebEngineViewer::WebHitTestResult result = job->property("webhitresult").value<WebEngineViewer::WebHitTestResult>();

    showMessagePopup(msg, url, imageUrl, aPoint, contactAlreadyExists, uniqueContactFound, result);
}

void KMReaderMainWin::showMessagePopup(const Akonadi::Item &msg, const QUrl &url, const QUrl &imageUrl, const QPoint &aPoint, bool contactAlreadyExists, bool uniqueContactFound, const WebEngineViewer::WebHitTestResult &result)
{
    QMenu *menu = nullptr;

    bool urlMenuAdded = false;
    bool copyAdded = false;
    const bool messageHasPayload = msg.hasPayload<KMime::Message::Ptr>();
    if (!url.isEmpty()) {
        if (url.scheme() == QLatin1String("mailto")) {
            // popup on a mailto URL
            menu = new QMenu;
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
        } else if (url.scheme() != QLatin1String("attachment")) {
            // popup on a not-mailto URL
            menu = new QMenu;
            menu->addAction(mReaderWin->urlOpenAction());
            menu->addAction(mReaderWin->addBookmarksAction());
            menu->addAction(mReaderWin->urlSaveAsAction());
            menu->addAction(mReaderWin->copyURLAction());
            menu->addSeparator();
            menu->addAction(mReaderWin->shareServiceUrlMenu());
            menu->addSeparator();
            menu->addActions(mReaderWin->viewerPluginActionList(MessageViewer::ViewerPluginInterface::NeedUrl));
            if (!imageUrl.isEmpty()) {
                menu->addSeparator();
                menu->addAction(mReaderWin->copyImageLocation());
                menu->addAction(mReaderWin->downloadImageToDiskAction());
                menu->addAction(mReaderWin->shareImage());
            }
            urlMenuAdded = true;
        }
    }
    const QString selectedText(mReaderWin->copyText());
    if (!selectedText.isEmpty()) {
        if (!menu) {
            menu = new QMenu;
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
        if (KPIMTextEdit::TextToSpeech::self()->isReady()) {
            menu->addSeparator();
            menu->addAction(mReaderWin->speakTextAction());
        }
    } else if (!urlMenuAdded) {
        if (!menu) {
            menu = new QMenu;
        }

        // popup somewhere else (i.e., not a URL) on the message
        if (messageHasPayload) {
            bool replyForwardMenu = false;
            Akonadi::Collection col = parentCollection();
            if (col.isValid()) {
                if (!(CommonKernel->folderIsSentMailFolder(col) ||
                        CommonKernel->folderIsDrafts(col) ||
                        CommonKernel->folderIsTemplates(col))) {
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
            }
            menu->addAction(copyActionMenu(menu));
            menu->addAction(moveActionMenu(menu));
            menu->addSeparator();
            menu->addAction(mMsgActions->mailingListActionMenu());

            menu->addSeparator();
            if (!imageUrl.isEmpty()) {
                menu->addSeparator();
                menu->addAction(mReaderWin->copyImageLocation());
                menu->addAction(mReaderWin->downloadImageToDiskAction());
                menu->addAction(mReaderWin->shareImage());
                menu->addSeparator();
            }

            menu->addAction(mMsgActions->printPreviewAction());
            menu->addAction(mMsgActions->printAction());
            menu->addSeparator();
            menu->addAction(mReaderWin->saveAsAction());
            menu->addAction(mSaveAtmAction);
            if (msg.isValid()) {
                menu->addSeparator();
                menu->addActions(mReaderWin->viewerPluginActionList(MessageViewer::ViewerPluginInterface::NeedMessage));
            }
        } else {
            menu->addAction(mReaderWin->toggleFixFontAction());
            if (!mReaderWin->mimePartTreeIsEmpty()) {
                menu->addAction(mReaderWin->toggleMimePartTreeAction());
            }
        }
        if (msg.isValid()) {
            menu->addAction(mMsgActions->addFollowupReminderAction());
        }
        if (msg.isValid()) {
            menu->addSeparator();
            menu->addAction(mMsgActions->addFollowupReminderAction());
        }
        if (kmkernel->allowToDebugBalooSupport()) {
            menu->addSeparator();
            menu->addAction(mMsgActions->debugBalooAction());
        }
    }
    if (!selectedText.isEmpty()) {
        const QList<QAction *> interceptorUrlActions = mReaderWin->interceptorUrlActions(result);
        if (!interceptorUrlActions.isEmpty()) {
            menu->addSeparator();
            menu->addActions(interceptorUrlActions);
        }
    }

    if (menu) {
        KAcceleratorManager::manage(menu);
        menu->exec(aPoint, nullptr);
        delete menu;
    }
}

void KMReaderMainWin::slotEditToolbars()
{
    KConfigGroup grp(KMKernel::self()->config(), "ReaderWindow");
    saveMainWindowSettings(grp);
    KEditToolBar dlg(guiFactory(), this);
    connect(&dlg, &KEditToolBar::newToolBarConfig, this, &KMReaderMainWin::slotUpdateToolbars);
    dlg.exec();
}

void KMReaderMainWin::slotUpdateToolbars()
{
    createGUI(QStringLiteral("kmreadermainwin.rc"));
    applyMainWindowSettings(KConfigGroup(KMKernel::self()->config(), "ReaderWindow"));
}

