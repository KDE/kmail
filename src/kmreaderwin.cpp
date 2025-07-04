/*
  This file is part of KMail, the KDE mail client.
  SPDX-FileCopyrightText: 1997 Markus Wuebben <markus.wuebben@kde.org>
  SPDX-FileCopyrightText: 2009-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

// define this to copy all html that is written to the readerwindow to
// filehtmlwriter.out in the current working directory
#include "kmreaderwin.h"

#include "dialog/addemailtoexistingcontactdialog.h"
#include "job/addemailtoexistingcontactjob.h"
#include "kmmainwidget.h"
#include "kmreadermainwin.h"
#include <MailCommon/MailKernel>

#include "kmail-version.h"
#include "kmcommands.h"
#include <Akonadi/AddEmailAddressJob>
#include <Akonadi/AddEmailDisplayJob>
#include <Akonadi/OpenEmailAddressJob>
#include <KEmailAddress>
#include <MailCommon/MailUtil>
#include <MessageViewer/CSSHelper>
#include <MessageViewer/HeaderStrategy>
#include <MessageViewer/MarkMessageReadHandler>
#include <MessageViewer/MessageViewerSettings>
#include <PimCommon/BroadcastStatus>
#include <QVBoxLayout>
using MessageViewer::CSSHelper;
#include "util.h"
#include <Akonadi/MessageFlags>
#include <KMime/MDN>
#include <MessageCore/StringUtil>
#include <QCryptographicHash>

using namespace MessageViewer;
#include <MessageCore/MessageCoreSettings>

#include <MessageComposer/ComposerJob>
#include <MessageComposer/InfoPart>
#include <MessageComposer/MDNWarningWidgetJob>
#include <MessageComposer/MessageSender>
#include <MessageComposer/TextPart>
#include <MessageViewer/AttachmentStrategy>

#include <KIO/JobUiDelegate>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/ContactEditorDialog>

#include "kmail_debug.h"
#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <KToggleAction>
#include <QAction>
#include <QDesktopServices>
#include <QMenu>

#include <QClipboard>

// X headers…
#undef Never
#undef Always

#include <MailCommon/MDNWarningJob>
#include <MailCommon/MailUtil>

#include <KLazyLocalizedString>
using namespace Qt::Literals::StringLiterals;

using namespace KMail;
using namespace MailCommon;

KMReaderWin::KMReaderWin(QWidget *aParent, QWidget *mainWindow, KActionCollection *actionCollection)
    : QWidget(aParent)
    , mMainWindow(mainWindow)
    , mActionCollection(actionCollection)
{
    createActions();
    auto vlay = new QVBoxLayout(this);
    vlay->setContentsMargins({});
    mViewer = new Viewer(this, mainWindow, mActionCollection);
    mViewer->setIdentityManager(kmkernel->identityManager());
    connect(mViewer, qOverload<const Akonadi::Item &, const QUrl &>(&Viewer::urlClicked), this, &KMReaderWin::slotUrlClicked);
    connect(mViewer,
            &Viewer::requestConfigSync,
            kmkernel,
            &KMKernel::slotRequestConfigSync,
            Qt::QueuedConnection); // happens anyway on shutdown, so we can skip it there with using a queued connection
    connect(mViewer, &Viewer::makeResourceOnline, kmkernel, &KMKernel::makeResourceOnline);
    connect(mViewer, &MessageViewer::Viewer::showReader, this, &KMReaderWin::slotShowReader);
    connect(mViewer, &MessageViewer::Viewer::showMessage, this, &KMReaderWin::slotShowMessage);
    connect(mViewer, &MessageViewer::Viewer::showStatusBarMessage, this, &KMReaderWin::showStatusBarMessage);
    connect(mViewer, &MessageViewer::Viewer::printingFinished, this, &KMReaderWin::slotPrintingFinished);
    connect(mViewer, &MessageViewer::Viewer::zoomChanged, this, &KMReaderWin::zoomChanged);
    connect(mViewer, qOverload<const Akonadi::Item &>(&Viewer::deleteMessage), this, &KMReaderWin::slotDeleteMessage);
    connect(mViewer, &MessageViewer::Viewer::showNextMessage, this, &KMReaderWin::showNextMessage);
    connect(mViewer, &MessageViewer::Viewer::showPreviousMessage, this, &KMReaderWin::showPreviousMessage);
    connect(mViewer, &MessageViewer::Viewer::sendResponse, this, &KMReaderWin::slotSendMdnResponse);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::itemChanged, this, &KMReaderWin::slotItemModified);

    mViewer->addMessageLoadedHandler(new MessageViewer::MarkMessageReadHandler(this));

    vlay->addWidget(mViewer);
    readConfig();
}

void KMReaderWin::createActions()
{
    KActionCollection *ac = mActionCollection;
    if (!ac) {
        return;
    }
    //
    // Message Menu
    //
    // new message to
    mMailToComposeAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-message-new")), i18n("New Message To…"), this);
    ac->addAction(QStringLiteral("mail_new"), mMailToComposeAction);
    ac->setShortcutsConfigurable(mMailToComposeAction, false);
    connect(mMailToComposeAction, &QAction::triggered, this, &KMReaderWin::slotMailtoCompose);

    // reply to
    mMailToReplyAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-reply-sender")), i18n("Reply To…"), this);
    ac->addAction(QStringLiteral("mailto_reply"), mMailToReplyAction);
    ac->setShortcutsConfigurable(mMailToReplyAction, false);
    connect(mMailToReplyAction, &QAction::triggered, this, &KMReaderWin::slotMailtoReply);

    // forward to
    mMailToForwardAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-forward")), i18n("Forward To…"), this);
    ac->setShortcutsConfigurable(mMailToForwardAction, false);
    ac->addAction(QStringLiteral("mailto_forward"), mMailToForwardAction);
    connect(mMailToForwardAction, &QAction::triggered, this, &KMReaderWin::slotMailtoForward);

    // add to addressbook
    mAddAddrBookAction = new QAction(QIcon::fromTheme(QStringLiteral("contact-new")), i18n("Add to Address Book"), this);
    ac->setShortcutsConfigurable(mAddAddrBookAction, false);
    ac->addAction(QStringLiteral("add_addr_book"), mAddAddrBookAction);
    connect(mAddAddrBookAction, &QAction::triggered, this, &KMReaderWin::slotMailtoAddAddrBook);

    mAddEmailToExistingContactAction = new QAction(QIcon::fromTheme(QStringLiteral("contact-new")), i18n("Add to Existing Contact"), this);
    ac->setShortcutsConfigurable(mAddEmailToExistingContactAction, false);
    ac->addAction(QStringLiteral("add_to_existing_contact"), mAddAddrBookAction);
    connect(mAddEmailToExistingContactAction, &QAction::triggered, this, &KMReaderWin::slotMailToAddToExistingContact);

    // open in addressbook
    mOpenAddrBookAction = new QAction(QIcon::fromTheme(QStringLiteral("view-pim-contacts")), i18n("Open in Address Book"), this);
    ac->setShortcutsConfigurable(mOpenAddrBookAction, false);
    ac->addAction(QStringLiteral("openin_addr_book"), mOpenAddrBookAction);
    connect(mOpenAddrBookAction, &QAction::triggered, this, &KMReaderWin::slotMailtoOpenAddrBook);
    // bookmark message
    mAddUrlToBookmarkAction = new QAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Bookmark This Link"), this);
    ac->setShortcutsConfigurable(mAddUrlToBookmarkAction, false);
    ac->addAction(QStringLiteral("add_bookmarks"), mAddUrlToBookmarkAction);
    connect(mAddUrlToBookmarkAction, &QAction::triggered, this, &KMReaderWin::slotAddUrlToBookmark);

    mEditContactAction = new QAction(QIcon::fromTheme(QStringLiteral("view-pim-contacts")), i18n("Edit contact…"), this);
    ac->setShortcutsConfigurable(mEditContactAction, false);
    ac->addAction(QStringLiteral("edit_contact"), mOpenAddrBookAction);
    connect(mEditContactAction, &QAction::triggered, this, &KMReaderWin::slotEditContact);

    // save URL as
    mUrlSaveAsAction = new QAction(i18nc("@action", "Save Link As…"), this);
    ac->addAction(QStringLiteral("saveas_url"), mUrlSaveAsAction);
    ac->setShortcutsConfigurable(mUrlSaveAsAction, false);
    connect(mUrlSaveAsAction, &QAction::triggered, this, &KMReaderWin::slotUrlSave);

    // find text
    auto action = new QAction(QIcon::fromTheme(QStringLiteral("edit-find")), i18n("&Find in Message…"), this);
    ac->addAction(QStringLiteral("find_in_messages"), action);
    connect(action, &QAction::triggered, this, &KMReaderWin::slotFind);
    ac->setDefaultShortcut(action, KStandardShortcut::find().first());

    // save Image On Disk
    mImageUrlSaveAsAction = new QAction(i18nc("@action", "Save Image On Disk…"), this);
    ac->addAction(QStringLiteral("saveas_imageurl"), mImageUrlSaveAsAction);
    ac->setShortcutsConfigurable(mImageUrlSaveAsAction, false);
    connect(mImageUrlSaveAsAction, &QAction::triggered, this, &KMReaderWin::slotSaveImageOnDisk);

    // save Image On Disk
    mOpenImageAction = new QAction(i18nc("@action", "Open Image…"), this);
    ac->addAction(QStringLiteral("open_image"), mOpenImageAction);
    ac->setShortcutsConfigurable(mOpenImageAction, false);
    connect(mOpenImageAction, &QAction::triggered, this, &KMReaderWin::slotOpenImage);

    // View html options
    mViewHtmlOptions = new QMenu(i18n("Show HTML Format"), this);
    mViewAsHtml = new QAction(i18nc("@action", "Show HTML format when mail comes from this contact"), mViewHtmlOptions);
    ac->setShortcutsConfigurable(mViewAsHtml, false);
    connect(mViewAsHtml, &QAction::triggered, this, &KMReaderWin::slotContactHtmlOptions);
    mViewAsHtml->setCheckable(true);
    mViewHtmlOptions->addAction(mViewAsHtml);

    mLoadExternalReference = new QAction(i18nc("@action", "Load external reference when mail comes for this contact"), mViewHtmlOptions);
    ac->setShortcutsConfigurable(mLoadExternalReference, false);
    connect(mLoadExternalReference, &QAction::triggered, this, &KMReaderWin::slotContactHtmlOptions);
    mLoadExternalReference->setCheckable(true);
    mViewHtmlOptions->addAction(mLoadExternalReference);

    mShareImage = new QAction(i18nc("@action", "Share image…"), this);
    ac->addAction(QStringLiteral("share_imageurl"), mShareImage);
    ac->setShortcutsConfigurable(mShareImage, false);
    connect(mShareImage, &QAction::triggered, this, &KMReaderWin::slotShareImage);
}

void KMReaderWin::setUseFixedFont(bool useFixedFont)
{
    mViewer->setUseFixedFont(useFixedFont);
}

Viewer *KMReaderWin::viewer() const
{
    return mViewer;
}

bool KMReaderWin::isFixedFont() const
{
    return mViewer->isFixedFont();
}

KMReaderWin::~KMReaderWin() = default;

void KMReaderWin::readConfig()
{
    mViewer->readConfig();
}

void KMReaderWin::setAttachmentStrategy(const MessageViewer::AttachmentStrategy *strategy)
{
    mViewer->setAttachmentStrategy(strategy);
}

void KMReaderWin::setOverrideEncoding(const QString &encoding)
{
    mViewer->setOverrideEncoding(encoding);
}

void KMReaderWin::clearCache()
{
    clear();
}

void KMReaderWin::updateShowMultiMessagesButton(bool enablePreviousButton, bool enableNextButton)
{
    mViewer->updateShowMultiMessagesButton(enablePreviousButton, enableNextButton);
}

void KMReaderWin::hasMultiMessages(bool multi)
{
    mViewer->hasMultiMessages(multi);
}

void KMReaderWin::displaySplashPage(const QString &templateName, const QVariantHash &_data)
{
    QVariantHash data = _data;
    if (!data.contains(QStringLiteral("icon"))) {
        data.insert(QStringLiteral("icon"), QStringLiteral("kmail"));
    }
    if (!data.contains(QStringLiteral("name"))) {
        data.insert(QStringLiteral("name"), i18n("KMail"));
    }
    if (!data.contains(QStringLiteral("subtitle"))) {
        data.insert(QStringLiteral("subtitle"), i18n("The KDE Mail Client"));
    }

    mViewer->displaySplashPage(templateName, data, QByteArrayLiteral("kmail"));
}

void KMReaderWin::displayBusyPage()
{
    displaySplashPage(QStringLiteral("status.html"),
                      {{QStringLiteral("title"), i18n("Retrieving Folder Contents")}, {QStringLiteral("subtext"), i18n("Please wait . . .")}});
}

void KMReaderWin::displayOfflinePage()
{
    displaySplashPage(QStringLiteral("status.html"),
                      {{QStringLiteral("title"), i18n("Offline")},
                       {QStringLiteral("subtext"),
                        i18n("KMail is currently in offline mode. "
                             "Click <a href=\"kmail:goOnline\">here</a> to go online . . .</p>")}});
}

void KMReaderWin::displayResourceOfflinePage()
{
    displaySplashPage(QStringLiteral("status.html"),
                      {{QStringLiteral("title"), i18n("Offline")},
                       {QStringLiteral("subtext"),
                        i18n("Account is currently in offline mode. "
                             "Click <a href=\"kmail:goResourceOnline\">here</a> to go online . . .</p>")}});
}

void KMReaderWin::slotFind()
{
    mViewer->slotFind();
}

void KMReaderWin::slotCopySelectedText()
{
    QString selection = mViewer->selectedText();
    selection.replace(QChar::Nbsp, QLatin1Char(' '));
    QApplication::clipboard()->setText(selection);
}

void KMReaderWin::setMsgPart(KMime::Content *aMsgPart)
{
    mViewer->setMessagePart(aMsgPart);
}

QString KMReaderWin::copyText() const
{
    return mViewer->selectedText();
}

MessageViewer::Viewer::DisplayFormatMessage KMReaderWin::displayFormatMessageOverwrite() const
{
    return mViewer->displayFormatMessageOverwrite();
}

void KMReaderWin::setPrintElementBackground(bool printElementBackground)
{
    mViewer->setPrintElementBackground(printElementBackground);
}

void KMReaderWin::setDisplayFormatMessageOverwrite(MessageViewer::Viewer::DisplayFormatMessage format)
{
    mViewer->setDisplayFormatMessageOverwrite(format);
}

void KMReaderWin::setHtmlLoadExtDefault(bool loadExtDefault)
{
    mViewer->setHtmlLoadExtDefault(loadExtDefault);
}

void KMReaderWin::setHtmlLoadExtOverride(bool loadExtOverride)
{
    mViewer->setHtmlLoadExtOverride(loadExtOverride);
}

bool KMReaderWin::htmlMail() const
{
    return mViewer->htmlMail();
}

bool KMReaderWin::htmlLoadExternal()
{
    return mViewer->htmlLoadExternal();
}

Akonadi::Item KMReaderWin::messageItem() const
{
    return mViewer->messageItem();
}

QWidget *KMReaderWin::mainWindow() const
{
    return mMainWindow;
}

void KMReaderWin::slotMailtoCompose()
{
    KMCommand *command = new KMMailtoComposeCommand(urlClicked(), messageItem());
    command->start();
}

void KMReaderWin::slotMailtoForward()
{
    KMCommand *command = new KMMailtoForwardCommand(mMainWindow, urlClicked(), messageItem());
    command->start();
}

void KMReaderWin::slotMailtoAddAddrBook()
{
    const QUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    const QString emailString = KEmailAddress::decodeMailtoUrl(url);

    auto job = new Akonadi::AddEmailAddressJob(emailString, mMainWindow, this);
    job->setInteractive(true);
    connect(job, &Akonadi::AddEmailAddressJob::successMessage, this, [](const QString &message) {
        PimCommon::BroadcastStatus::instance()->setStatusMsg(message);
    });
    job->start();
}

void KMReaderWin::slotMailToAddToExistingContact()
{
    const QUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    const QString emailString = KEmailAddress::decodeMailtoUrl(url);
    QPointer<AddEmailToExistingContactDialog> dlg = new AddEmailToExistingContactDialog(this);
    if (dlg->exec()) {
        Akonadi::Item item = dlg->selectedContact();
        if (item.isValid()) {
            auto job = new AddEmailToExistingContactJob(item, emailString, this);
            job->start();
        }
    }
    delete dlg;
}

void KMReaderWin::slotMailtoOpenAddrBook()
{
    const QUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    const QString emailString = KEmailAddress::decodeMailtoUrl(url).toLower();

    auto job = new Akonadi::OpenEmailAddressJob(emailString, mMainWindow, this);
    job->start();
}

void KMReaderWin::slotAddUrlToBookmark()
{
    const QUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    KMCommand *command = new KMAddBookmarksCommand(url, this);
    command->start();
}

void KMReaderWin::slotUrlSave()
{
    const QUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    KMCommand *command = new KMUrlSaveCommand(url, mMainWindow);
    command->start();
}

void KMReaderWin::slotOpenImage()
{
    const QUrl url = imageUrlClicked();
    if (url.isEmpty()) {
        return;
    }
    QDesktopServices::openUrl(url);
}

void KMReaderWin::slotSaveImageOnDisk()
{
    const QUrl url = imageUrlClicked();
    if (url.isEmpty()) {
        return;
    }
    KMCommand *command = new KMUrlSaveCommand(url, mMainWindow);
    command->start();
}

void KMReaderWin::slotMailtoReply()
{
    auto command = new KMMailtoReplyCommand(mMainWindow, urlClicked(), messageItem(), copyText());
    command->setReplyAsHtml(htmlMail());
    command->start();
}

CSSHelper *KMReaderWin::cssHelper() const
{
    return mViewer->cssHelper();
}

bool KMReaderWin::htmlLoadExtOverride() const
{
    return mViewer->htmlLoadExtOverride();
}

void KMReaderWin::setDecryptMessageOverwrite(bool overwrite)
{
    mViewer->setDecryptMessageOverwrite(overwrite);
}

const MessageViewer::AttachmentStrategy *KMReaderWin::attachmentStrategy() const
{
    return mViewer->attachmentStrategy();
}

QString KMReaderWin::overrideEncoding() const
{
    return mViewer->overrideEncoding();
}

KToggleAction *KMReaderWin::toggleFixFontAction() const
{
    return mViewer->toggleFixFontAction();
}

QAction *KMReaderWin::mailToComposeAction() const
{
    return mMailToComposeAction;
}

QAction *KMReaderWin::mailToReplyAction() const
{
    return mMailToReplyAction;
}

QAction *KMReaderWin::mailToForwardAction() const
{
    return mMailToForwardAction;
}

QAction *KMReaderWin::addAddrBookAction() const
{
    return mAddAddrBookAction;
}

QAction *KMReaderWin::openAddrBookAction() const
{
    return mOpenAddrBookAction;
}

bool KMReaderWin::mimePartTreeIsEmpty() const
{
    return mViewer->mimePartTreeIsEmpty();
}

QAction *KMReaderWin::toggleMimePartTreeAction() const
{
    return mViewer->toggleMimePartTreeAction();
}

KActionMenu *KMReaderWin::shareServiceUrlMenu() const
{
    return mViewer->shareServiceUrlMenu();
}

DKIMViewerMenu *KMReaderWin::dkimViewerMenu() const
{
    return mViewer->dkimViewerMenu();
}

RemoteContentMenu *KMReaderWin::remoteContentMenu() const
{
    return mViewer->remoteContentMenu();
}

QList<QAction *> KMReaderWin::viewerPluginActionList(ViewerPluginInterface::SpecificFeatureTypes features)
{
    return mViewer->viewerPluginActionList(features);
}

QAction *KMReaderWin::selectAllAction() const
{
    return mViewer->selectAllAction();
}

QAction *KMReaderWin::copyURLAction() const
{
    return mViewer->copyURLAction();
}

QAction *KMReaderWin::copyImageLocation() const
{
    return mViewer->copyImageLocation();
}

QAction *KMReaderWin::copyAction() const
{
    return mViewer->copyAction();
}

QAction *KMReaderWin::viewSourceAction() const
{
    return mViewer->viewSourceAction();
}

QAction *KMReaderWin::saveAsAction() const
{
    return mViewer->saveAsAction();
}

QAction *KMReaderWin::findInMessageAction() const
{
    return mViewer->findInMessageAction();
}

QAction *KMReaderWin::urlOpenAction() const
{
    return mViewer->urlOpenAction();
}

QAction *KMReaderWin::urlSaveAsAction() const
{
    return mUrlSaveAsAction;
}

QAction *KMReaderWin::addUrlToBookmarkAction() const
{
    return mAddUrlToBookmarkAction;
}

void KMReaderWin::setPrinting(bool enable)
{
    mViewer->setPrinting(enable);
}

QAction *KMReaderWin::speakTextAction() const
{
    return mViewer->speakTextAction();
}

QAction *KMReaderWin::shareTextAction() const
{
    return mViewer->shareTextAction();
}

QAction *KMReaderWin::downloadImageToDiskAction() const
{
    return mImageUrlSaveAsAction;
}

QAction *KMReaderWin::openImageAction() const
{
    return mOpenImageAction;
}

void KMReaderWin::clear(bool force)
{
    mViewer->clear(force ? MimeTreeParser::Force : MimeTreeParser::Delayed);
}

void KMReaderWin::setMessage(const Akonadi::Item &item, MimeTreeParser::UpdateMode updateMode)
{
    qCDebug(KMAIL_LOG) << Q_FUNC_INFO << parentWidget();
    mViewer->setFolderIdentity(MailCommon::Util::folderIdentity(item));
    mViewer->setMessageItem(item, updateMode);
    if (!item.hasAttribute<Akonadi::MDNStateAttribute>()
        || (item.hasAttribute<Akonadi::MDNStateAttribute>()
            && item.attribute<Akonadi::MDNStateAttribute>()->mdnState() == Akonadi::MDNStateAttribute::MDNStateUnknown)) {
        sendMdnInfo(item);
    } else {
        mViewer->mdnWarningAnimatedHide();
    }
}

void KMReaderWin::setMessage(const KMime::Message::Ptr &message)
{
    mViewer->setFolderIdentity(0);
    mViewer->setMessage(message);
}

QUrl KMReaderWin::urlClicked() const
{
    return mViewer->urlClicked();
}

QUrl KMReaderWin::imageUrlClicked() const
{
    return mViewer->imageUrlClicked();
}

void KMReaderWin::update(bool force)
{
    mViewer->update(force ? MimeTreeParser::Force : MimeTreeParser::Delayed);
}

void KMReaderWin::slotUrlClicked(const Akonadi::Item &item, const QUrl &url)
{
    if (item.isValid() && item.parentCollection().isValid()) {
        const auto col = CommonKernel->collectionFromId(item.parentCollection().id());
        QSharedPointer<FolderSettings> fd = FolderSettings::forCollection(col, false);
        KMail::Util::handleClickedURL(url, fd, item.parentCollection(), this);
        return;
    }
    // No folder so we can't have identity and template.
    KMail::Util::handleClickedURL(url);
}

void KMReaderWin::slotShowReader(KMime::Content *msgPart, bool html, const QString &encoding)
{
    const MessageViewer::Viewer::DisplayFormatMessage format = html ? MessageViewer::Viewer::Html : MessageViewer::Viewer::Text;
    auto win = new KMReaderMainWin(msgPart, format, encoding);
    win->show();
}

void KMReaderWin::slotShowMessage(const KMime::Message::Ptr &message, const QString &encoding)
{
    auto win = new KMReaderMainWin();
    win->showMessage(encoding, message);
    win->show();
}

void KMReaderWin::slotDeleteMessage(const Akonadi::Item &item)
{
    if (!item.isValid()) {
        return;
    }
    auto command = new KMTrashMsgCommand(item.parentCollection(), item, -1);
    command->start();
}

bool KMReaderWin::printSelectedText(bool preview)
{
    const QString str = mViewer->selectedText();
    if (str.isEmpty()) {
        return false;
    }
    auto composer = new ::MessageComposer::ComposerJob;
    composer->textPart()->setCleanPlainText(str);
    composer->textPart()->setWrappedPlainText(str);
    auto messagePtr = messageItem().payload<KMime::Message::Ptr>();
    composer->infoPart()->setFrom(messagePtr->from()->asUnicodeString());
    if (auto to = messagePtr->to(false)) {
        composer->infoPart()->setTo(QStringList() << to->asUnicodeString());
    }
    if (auto cc = messagePtr->cc(false)) {
        composer->infoPart()->setCc(QStringList() << cc->asUnicodeString());
    }
    if (auto subject = messagePtr->subject(false)) {
        composer->infoPart()->setSubject(subject->asUnicodeString());
    }
    composer->setProperty("preview", preview);
    connect(composer, &::MessageComposer::ComposerJob::result, this, &KMReaderWin::slotPrintComposeResult);
    composer->start();
    return true;
}

void KMReaderWin::slotPrintComposeResult(KJob *job)
{
    const bool preview = job->property("preview").toBool();
    auto composer = qobject_cast<::MessageComposer::ComposerJob *>(job);
    Q_ASSERT(composer);
    if (composer->error() == ::MessageComposer::ComposerJob::NoError) {
        Q_ASSERT(composer->resultMessages().size() == 1);
        Akonadi::Item printItem;
        printItem.setPayload<KMime::Message::Ptr>(composer->resultMessages().constFirst());
        Akonadi::MessageFlags::copyMessageFlags(*(composer->resultMessages().constFirst()), printItem);
        const bool useFixedFont = MessageViewer::MessageViewerSettings::self()->useFixedFont();
        const QString overrideEncoding = MessageCore::MessageCoreSettings::self()->overrideCharacterEncoding();

        KMPrintCommandInfo commandInfo;
        commandInfo.mMsg = printItem;
        commandInfo.mHeaderStylePlugin = mViewer->headerStylePlugin();
        commandInfo.mFormat = mViewer->displayFormatMessageOverwrite();
        commandInfo.mHtmlLoadExtOverride = mViewer->htmlLoadExternal();
        commandInfo.mPrintPreview = preview;
        commandInfo.mUseFixedFont = useFixedFont;
        commandInfo.mOverrideFont = overrideEncoding;
        commandInfo.mShowSignatureDetails =
            mViewer->showSignatureDetails() || MessageViewer::MessageViewerSettings::self()->alwaysShowEncryptionSignatureDetails();
        commandInfo.mShowEncryptionDetails =
            mViewer->showEncryptionDetails() || MessageViewer::MessageViewerSettings::self()->alwaysShowEncryptionSignatureDetails();

        auto command = new KMPrintCommand(this, commandInfo);
        command->start();
    } else {
        if (static_cast<KIO::Job *>(job)->uiDelegate()) {
            static_cast<KIO::Job *>(job)->uiDelegate()->showErrorMessage();
        } else {
            qCWarning(KMAIL_LOG) << "Composer for printing failed:" << composer->errorString();
        }
    }
}

void KMReaderWin::clearContactItem()
{
    mSearchedContact = Akonadi::Item();
    mSearchedAddress = KContacts::Addressee();
    mLoadExternalReference->setChecked(false);
    mViewAsHtml->setChecked(false);
}

void KMReaderWin::setContactItem(const Akonadi::Item &contact, const KContacts::Addressee &address)
{
    mSearchedContact = contact;
    mSearchedAddress = address;
    updateHtmlActions();
}

void KMReaderWin::updateHtmlActions()
{
    if (!mSearchedContact.isValid()) {
        mLoadExternalReference->setChecked(false);
        mViewAsHtml->setChecked(false);
    } else {
        const QStringList customs = mSearchedAddress.customs();
        for (const QString &custom : customs) {
            if (custom.contains("MailPreferedFormatting"_L1)) {
                const QString value = mSearchedAddress.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("MailPreferedFormatting"));
                mViewAsHtml->setChecked(value == "HTML"_L1);
            } else if (custom.contains("MailAllowToRemoteContent"_L1)) {
                const QString value = mSearchedAddress.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("MailAllowToRemoteContent"));
                mLoadExternalReference->setChecked((value == "TRUE"_L1));
            }
        }
    }
}

void KMReaderWin::slotContactHtmlOptions()
{
    const QUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    const QString emailString = KEmailAddress::decodeMailtoUrl(url).toLower();

    auto job = new Akonadi::AddEmailDisplayJob(emailString, mMainWindow, this);
    job->setMessageId(mViewer->messageItem().id());
    connect(job, &Akonadi::AddEmailDisplayJob::contactUpdated, this, &KMReaderWin::slotContactHtmlPreferencesUpdated);
    job->setRemoteContent(mLoadExternalReference->isChecked());
    job->setShowAsHTML(mViewAsHtml->isChecked());
    job->setContact(mSearchedContact);
    job->start();
}

void KMReaderWin::slotContactHtmlPreferencesUpdated(const Akonadi::Item &contact, Akonadi::Item::Id id, bool showAsHTML, bool remoteContent)
{
    Q_UNUSED(contact)
    if (mViewer->messageItem().id() == id) {
        mViewer->slotChangeDisplayMail(showAsHTML ? Viewer::Html : Viewer::Text, remoteContent);
    }
}

void KMReaderWin::slotEditContact()
{
    if (mSearchedContact.isValid()) {
        QPointer<Akonadi::ContactEditorDialog> dlg = new Akonadi::ContactEditorDialog(Akonadi::ContactEditorDialog::EditMode, this);
        connect(dlg.data(), &Akonadi::ContactEditorDialog::contactStored, this, &KMReaderWin::contactStored);
        connect(dlg.data(), &Akonadi::ContactEditorDialog::error, this, &KMReaderWin::slotContactEditorError);
        dlg->setContact(mSearchedContact);
        dlg->exec();
        delete dlg;
    }
}

void KMReaderWin::slotContactEditorError(const QString &error)
{
    KMessageBox::error(this, i18n("Contact cannot be stored: %1", error), i18nc("@title:window", "Failed to store contact"));
}

void KMReaderWin::contactStored(const Akonadi::Item &item)
{
    if (item.hasPayload<KContacts::Addressee>()) {
        const auto contact = item.payload<KContacts::Addressee>();
        setContactItem(item, contact);
        mViewer->slotChangeDisplayMail(mViewAsHtml->isChecked() ? Viewer::Html : Viewer::Text, mLoadExternalReference->isChecked());
    }
    PimCommon::BroadcastStatus::instance()->setStatusMsg(i18n("Contact modified successfully"));
}

QAction *KMReaderWin::saveMessageDisplayFormatAction() const
{
    return mViewer->saveMessageDisplayFormatAction();
}

QAction *KMReaderWin::resetMessageDisplayFormatAction() const
{
    return mViewer->resetMessageDisplayFormatAction();
}

QAction *KMReaderWin::editContactAction() const
{
    return mEditContactAction;
}

QAction *KMReaderWin::developmentToolsAction() const
{
    return mViewer->developmentToolsAction();
}

QMenu *KMReaderWin::viewHtmlOption() const
{
    return mViewHtmlOptions;
}

QAction *KMReaderWin::shareImage() const
{
    return mShareImage;
}

QAction *KMReaderWin::addToExistingContactAction() const
{
    return mAddEmailToExistingContactAction;
}

void KMReaderWin::slotShareImage()
{
    KMCommand *command = new KMShareImageCommand(imageUrlClicked(), this);
    command->start();
}

QList<QAction *> KMReaderWin::interceptorUrlActions(const WebEngineViewer::WebHitTestResult &result) const
{
    return mViewer->interceptorUrlActions(result);
}

void KMReaderWin::slotPrintingFinished()
{
    if (mViewer->printingMode()) {
        deleteLater();
    }
}

void KMReaderWin::sendMdnInfo(const Akonadi::Item &item)
{
    auto job = new MessageComposer::MDNWarningWidgetJob(this);
    job->setItem(item);
    connect(job, &MessageComposer::MDNWarningWidgetJob::showMdnInfo, this, &KMReaderWin::slotShowMdnInfo);
    if (!job->start()) {
        qCWarning(KMAIL_LOG) << "Impossible to start MDNWarningWidgetJob";
    }
}

void KMReaderWin::slotShowMdnInfo(const QPair<QString, bool> &mdnInfo)
{
    mViewer->showMdnInformations(mdnInfo);
}

void KMReaderWin::slotSendMdnResponse(MessageViewer::MDNWarningWidget::ResponseType type, KMime::MDN::SendingMode sendingMode)
{
    MailCommon::MDNWarningJob::ResponseMDN response = MailCommon::MDNWarningJob::ResponseMDN::Unknown;
    switch (type) {
    case MessageViewer::MDNWarningWidget::ResponseType::Ignore:
        response = MailCommon::MDNWarningJob::ResponseMDN::MDNIgnore;
        break;
    case MessageViewer::MDNWarningWidget::ResponseType::Send:
        response = MailCommon::MDNWarningJob::ResponseMDN::Send;
        break;
    case MessageViewer::MDNWarningWidget::ResponseType::SendDeny:
        response = MailCommon::MDNWarningJob::ResponseMDN::Denied;
        break;
    }

    auto job = new MailCommon::MDNWarningJob(KMKernel::self(), this);
    job->setItem(mViewer->messageItem());
    job->setResponse(response);
    job->setSendingMode(sendingMode);
    job->start();
    connect(job, &MDNWarningJob::finished, this, [this]() {
        mViewer->mdnWarningAnimatedHide();
    });
}

void KMReaderWin::slotItemModified(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers)
{
    if (mViewer->messageItem().id() == item.id()) {
        if (partIdentifiers.contains("MDNStateAttribute")) {
            mViewer->mdnWarningAnimatedHide();
        }
    }
}

void KMReaderWin::addImageMenuActions(QMenu *menu)
{
    menu->addSeparator();
    menu->addAction(copyImageLocation());
    menu->addAction(downloadImageToDiskAction());
    menu->addAction(shareImage());
    menu->addAction(openImageAction());
}

#include "moc_kmreaderwin.cpp"
