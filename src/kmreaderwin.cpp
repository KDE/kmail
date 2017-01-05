/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>
  Copyright (c) 2009-2017 Laurent Montel <montel@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

// define this to copy all html that is written to the readerwindow to
// filehtmlwriter.out in the current working directory
#include "kmreaderwin.h"

#include "settings/kmailsettings.h"
#include "kmmainwidget.h"
#include "kmreadermainwin.h"
#include "mailcommon/mailkernel.h"
#include "dialog/addemailtoexistingcontactdialog.h"
#include "job/addemailtoexistingcontactjob.h"

#include "kmail-version.h"
#include <KEmailAddress>
#include <Libkdepim/AddEmailAddressJob>
#include <Libkdepim/OpenEmailAddressJob>
#include <Libkdepim/AddEmailDisplayJob>
#include <Libkdepim/BroadcastStatus>
#include "kmcommands.h"
#include "MailCommon/SendMdnHandler"
#include <QVBoxLayout>
#include "messageviewer/headerstrategy.h"
#include "messageviewer/markmessagereadhandler.h"
#include "messageviewer/messageviewersettings.h"
#include <MessageViewer/CSSHelper>
using MessageViewer::CSSHelper;
#include "util.h"
#include <MessageCore/StringUtil>
#include <QCryptographicHash>
#include <kmime/kmime_mdn.h>
#include <akonadi/kmime/messageflags.h>

#include "messageviewer/viewer.h"
using namespace MessageViewer;
#include <MessageCore/MessageCoreSettings>

#include <MimeTreeParser/AttachmentStrategy>
#include <MessageComposer/MessageSender>
#include <MessageComposer/MessageFactory>
#include "MessageComposer/Composer"
#include "MessageComposer/TextPart"
#include "MessageComposer/InfoPart"

#include <KIO/JobUiDelegate>
using MessageComposer::MessageFactory;

#include "messagecore/messagehelpers.h"

#include <Akonadi/Contact/ContactEditorDialog>

#include "kmail_debug.h"
#include <KLocalizedString>
#include <QAction>
#include <kcodecs.h>
#include <ktoggleaction.h>
#include <kservice.h>
#include <KActionCollection>
#include <KMessageBox>
#include <QMenu>

#include <QClipboard>

// X headers...
#undef Never
#undef Always

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <mailcommon/mailutil.h>

using namespace KMail;
using namespace MailCommon;

KMReaderWin::KMReaderWin(QWidget *aParent,
                         QWidget *mainWindow,
                         KActionCollection *actionCollection)
    : QWidget(aParent),
      mMainWindow(mainWindow),
      mActionCollection(actionCollection),
      mMailToComposeAction(nullptr),
      mMailToReplyAction(nullptr),
      mMailToForwardAction(nullptr),
      mAddAddrBookAction(nullptr),
      mOpenAddrBookAction(nullptr),
      mUrlSaveAsAction(nullptr),
      mAddBookmarksAction(nullptr),
      mAddEmailToExistingContactAction(nullptr)
{
    createActions();
    QVBoxLayout *vlay = new QVBoxLayout(this);
    vlay->setMargin(0);
    mViewer = new Viewer(this, mainWindow, mActionCollection);
    connect(mViewer, SIGNAL(urlClicked(Akonadi::Item,QUrl)), this, SLOT(slotUrlClicked(Akonadi::Item,QUrl)));
    connect(mViewer, &Viewer::requestConfigSync, kmkernel, &KMKernel::slotRequestConfigSync, Qt::QueuedConnection);   // happens anyway on shutdown, so we can skip it there with using a queued connection
    connect(mViewer, &Viewer::makeResourceOnline, kmkernel, &KMKernel::makeResourceOnline);
    connect(mViewer, &MessageViewer::Viewer::showReader, this, &KMReaderWin::slotShowReader);
    connect(mViewer, &MessageViewer::Viewer::showMessage, this, &KMReaderWin::slotShowMessage);
    connect(mViewer, &MessageViewer::Viewer::showStatusBarMessage, this, &KMReaderWin::showStatusBarMessage);
    connect(mViewer, &MessageViewer::Viewer::printingFinished, this, &KMReaderWin::slotPrintingFinished);
    connect(mViewer, SIGNAL(deleteMessage(Akonadi::Item)), this, SLOT(slotDeleteMessage(Akonadi::Item)));

    mViewer->addMessageLoadedHandler(new MessageViewer::MarkMessageReadHandler(this));
    mViewer->addMessageLoadedHandler(new MailCommon::SendMdnHandler(kmkernel, this));

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
    mMailToComposeAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-message-new")),
                                       i18n("New Message To..."), this);
    ac->addAction(QStringLiteral("mail_new"), mMailToComposeAction);
    ac->setShortcutsConfigurable(mMailToComposeAction, false);
    connect(mMailToComposeAction, &QAction::triggered, this, &KMReaderWin::slotMailtoCompose);

    // reply to
    mMailToReplyAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-reply-sender")),
                                     i18n("Reply To..."), this);
    ac->addAction(QStringLiteral("mailto_reply"), mMailToReplyAction);
    ac->setShortcutsConfigurable(mMailToReplyAction, false);
    connect(mMailToReplyAction, &QAction::triggered, this, &KMReaderWin::slotMailtoReply);

    // forward to
    mMailToForwardAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-forward")),
                                       i18n("Forward To..."), this);
    ac->setShortcutsConfigurable(mMailToForwardAction, false);
    ac->addAction(QStringLiteral("mailto_forward"), mMailToForwardAction);
    connect(mMailToForwardAction, &QAction::triggered, this, &KMReaderWin::slotMailtoForward);

    // add to addressbook
    mAddAddrBookAction = new QAction(QIcon::fromTheme(QStringLiteral("contact-new")),
                                     i18n("Add to Address Book"), this);
    ac->setShortcutsConfigurable(mAddAddrBookAction, false);
    ac->addAction(QStringLiteral("add_addr_book"), mAddAddrBookAction);
    connect(mAddAddrBookAction, &QAction::triggered, this, &KMReaderWin::slotMailtoAddAddrBook);

    mAddEmailToExistingContactAction = new QAction(QIcon::fromTheme(QStringLiteral("contact-new")),
            i18n("Add to Existing Contact"), this);
    ac->setShortcutsConfigurable(mAddEmailToExistingContactAction, false);
    ac->addAction(QStringLiteral("add_to_existing_contact"), mAddAddrBookAction);
    connect(mAddEmailToExistingContactAction, &QAction::triggered, this, &KMReaderWin::slotMailToAddToExistingContact);

    // open in addressbook
    mOpenAddrBookAction = new QAction(QIcon::fromTheme(QStringLiteral("view-pim-contacts")),
                                      i18n("Open in Address Book"), this);
    ac->setShortcutsConfigurable(mOpenAddrBookAction, false);
    ac->addAction(QStringLiteral("openin_addr_book"), mOpenAddrBookAction);
    connect(mOpenAddrBookAction, &QAction::triggered, this, &KMReaderWin::slotMailtoOpenAddrBook);
    // bookmark message
    mAddBookmarksAction = new QAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Bookmark This Link"), this);
    ac->setShortcutsConfigurable(mAddBookmarksAction, false);
    ac->addAction(QStringLiteral("add_bookmarks"), mAddBookmarksAction);
    connect(mAddBookmarksAction, &QAction::triggered, this, &KMReaderWin::slotAddBookmarks);

    mEditContactAction = new QAction(QIcon::fromTheme(QStringLiteral("view-pim-contacts")),
                                     i18n("Edit contact..."), this);
    ac->setShortcutsConfigurable(mEditContactAction, false);
    ac->addAction(QStringLiteral("edit_contact"), mOpenAddrBookAction);
    connect(mEditContactAction, &QAction::triggered, this, &KMReaderWin::slotEditContact);

    // save URL as
    mUrlSaveAsAction = new QAction(i18n("Save Link As..."), this);
    ac->addAction(QStringLiteral("saveas_url"), mUrlSaveAsAction);
    ac->setShortcutsConfigurable(mUrlSaveAsAction, false);
    connect(mUrlSaveAsAction, &QAction::triggered, this, &KMReaderWin::slotUrlSave);

    // find text
    QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("edit-find")), i18n("&Find in Message..."), this);
    ac->addAction(QStringLiteral("find_in_messages"), action);
    connect(action, &QAction::triggered, this, &KMReaderWin::slotFind);
    ac->setDefaultShortcut(action, KStandardShortcut::find().first());

    // save Image On Disk
    mImageUrlSaveAsAction = new QAction(i18n("Save Image On Disk..."), this);
    ac->addAction(QStringLiteral("saveas_imageurl"), mImageUrlSaveAsAction);
    ac->setShortcutsConfigurable(mImageUrlSaveAsAction, false);
    connect(mImageUrlSaveAsAction, &QAction::triggered, this, &KMReaderWin::slotSaveImageOnDisk);

    // View html options
    mViewHtmlOptions = new QMenu(i18n("Show HTML Format"));
    mViewAsHtml = new QAction(i18n("Show HTML format when mail comes from this contact"), mViewHtmlOptions);
    ac->setShortcutsConfigurable(mViewAsHtml, false);
    connect(mViewAsHtml, &QAction::triggered, this, &KMReaderWin::slotContactHtmlOptions);
    mViewAsHtml->setCheckable(true);
    mViewHtmlOptions->addAction(mViewAsHtml);

    mLoadExternalReference = new QAction(i18n("Load external reference when mail comes for this contact"), mViewHtmlOptions);
    ac->setShortcutsConfigurable(mLoadExternalReference, false);
    connect(mLoadExternalReference, &QAction::triggered, this, &KMReaderWin::slotContactHtmlOptions);
    mLoadExternalReference->setCheckable(true);
    mViewHtmlOptions->addAction(mLoadExternalReference);

    mShareImage = new QAction(i18n("Share image..."), this);
    ac->addAction(QStringLiteral("share_imageurl"), mShareImage);
    ac->setShortcutsConfigurable(mShareImage, false);
    connect(mShareImage, &QAction::triggered, this, &KMReaderWin::slotShareImage);

}

void KMReaderWin::setUseFixedFont(bool useFixedFont)
{
    mViewer->setUseFixedFont(useFixedFont);
}

Viewer *KMReaderWin::viewer()
{
    return mViewer;
}

bool KMReaderWin::isFixedFont() const
{
    return mViewer->isFixedFont();
}

KMReaderWin::~KMReaderWin()
{
}

void KMReaderWin::readConfig(void)
{
    mViewer->readConfig();
}

void KMReaderWin::setAttachmentStrategy(const MimeTreeParser::AttachmentStrategy *strategy)
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

// enter items for the "Important changes" list here:
static const char *const kmailChanges[] = {
    I18N_NOOP("KMail is now based on the Akonadi Personal Information Management framework, which brings many "
    "changes all around.")
};
static const int numKMailChanges =
    sizeof kmailChanges / sizeof * kmailChanges;

// enter items for the "new features" list here, so the main body of
// the welcome page can be left untouched (probably much easier for
// the translators). Note that the <li>...</li> tags are added
// automatically below:
static const char *const kmailNewFeatures[] = {
    I18N_NOOP("Push email (IMAP IDLE)"),
    I18N_NOOP("Improved searches"),
    I18N_NOOP("Support for adding notes (annotations) to mails"),
    I18N_NOOP("Less GUI freezes, mail checks happen in the background"),
    I18N_NOOP("Plugins support"),
    I18N_NOOP("New HTML renderer (QtWebEngine)"),
    I18N_NOOP("Added Check for Phishing URL"),
};
static const int numKMailNewFeatures =
    sizeof kmailNewFeatures / sizeof * kmailNewFeatures;

//static
QString KMReaderWin::newFeaturesMD5()
{
    QByteArray str;
    for (int i = 0; i < numKMailChanges; ++i) {
        str += kmailChanges[i];
    }
    for (int i = 0; i < numKMailNewFeatures; ++i) {
        str += kmailNewFeatures[i];
    }
    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(str);
    return QLatin1String(md5.result().toBase64());
}

void KMReaderWin::displaySplashPage(const QString &templateName, const QVariantHash &_data)
{
    QVariantHash data = _data;
    if (!data.contains(QStringLiteral("icon"))) {
        data[QStringLiteral("icon")] = QStringLiteral("kmail");
    }
    if (!data.contains(QStringLiteral("name"))) {
        data[QStringLiteral("name")] = i18n("KMail");
    }
    if (!data.contains(QStringLiteral("subtitle"))) {
        data[QStringLiteral("subtitle")] = i18n("The KDE Mail Client");
    }

    mViewer->displaySplashPage(templateName, data, QByteArrayLiteral("kmail"));
}

void KMReaderWin::displayBusyPage()
{
    displaySplashPage(QStringLiteral("status.html"), {
        { QStringLiteral("title"), i18n("Retrieving Folder Contents") },
        { QStringLiteral("subtext"), i18n("Please wait . . .") }
    });
}

void KMReaderWin::displayOfflinePage()
{
    displaySplashPage(QStringLiteral("status.html"), {
        { QStringLiteral("title"), i18n("Offline") },
        {
            QStringLiteral("subtext"), i18n("KMail is currently in offline mode. "
            "Click <a href=\"kmail:goOnline\">here</a> to go online . . .</p>")
        }
    });
}

void KMReaderWin::displayResourceOfflinePage()
{
    displaySplashPage(QStringLiteral("status.html"), {
        { QStringLiteral("title"), i18n("Offline") },
        {
            QStringLiteral("subtext"), i18n("Account is currently in offline mode. "
            "Click <a href=\"kmail:goResourceOnline\">here</a> to go online . . .</p>")
        }
    });
}

void KMReaderWin::displayAboutPage()
{
    QVariantHash data;
    data[QStringLiteral("version")] = QStringLiteral(KDEPIM_VERSION);
    data[QStringLiteral("firstStart")] = kmkernel->firstStart();

    QVariantList features;
    features.reserve(numKMailNewFeatures);
    for (int i = 0; i < numKMailNewFeatures; ++i) {
        features.push_back(i18n(kmailNewFeatures[i]));
    }
    data[QStringLiteral("newFeatures")] = features;

    QVariantList changes;
    changes.reserve(numKMailChanges);
    for (int i = 0; i < numKMailChanges; ++i) {
        features.push_back(i18n(kmailChanges[i]));
    }
    data[QStringLiteral("importantChanges")] = changes;

    displaySplashPage(QStringLiteral("introduction_kmail.html"), data);
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

void KMReaderWin::setHtmlLoadExtOverride(bool override)
{
    mViewer->setHtmlLoadExtOverride(override);
}

bool KMReaderWin::htmlMail() const
{
    return mViewer->htmlMail();
}

bool KMReaderWin::htmlLoadExternal()
{
    return mViewer->htmlLoadExternal();
}

Akonadi::Item KMReaderWin::message() const
{
    return mViewer->messageItem();
}

QWidget *KMReaderWin::mainWindow()
{
    return mMainWindow;
}

void KMReaderWin::slotMailtoCompose()
{
    KMCommand *command = new KMMailtoComposeCommand(urlClicked(), message());
    command->start();
}

void KMReaderWin::slotMailtoForward()
{
    KMCommand *command = new KMMailtoForwardCommand(mMainWindow, urlClicked(),
            message());
    command->start();
}

void KMReaderWin::slotMailtoAddAddrBook()
{
    const QUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    const QString emailString = KEmailAddress::decodeMailtoUrl(url);

    KPIM::AddEmailAddressJob *job = new KPIM::AddEmailAddressJob(emailString, mMainWindow, this);
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
            AddEmailToExistingContactJob *job = new AddEmailToExistingContactJob(item, emailString, this);
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

    KPIM::OpenEmailAddressJob *job = new KPIM::OpenEmailAddressJob(emailString, mMainWindow, this);
    job->start();
}

void KMReaderWin::slotAddBookmarks()
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
    KMCommand *command = new KMMailtoReplyCommand(mMainWindow, urlClicked(),
            message(), copyText());
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

const MimeTreeParser::AttachmentStrategy *KMReaderWin::attachmentStrategy() const
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

QAction *KMReaderWin::addBookmarksAction() const
{
    return mAddBookmarksAction;
}
void KMReaderWin::setPrinting(bool enable)
{
    mViewer->setPrinting(enable);
}

QAction *KMReaderWin::speakTextAction() const
{
    return mViewer->speakTextAction();
}

QAction *KMReaderWin::downloadImageToDiskAction() const
{
    return mImageUrlSaveAsAction;
}

void KMReaderWin::clear(bool force)
{
    mViewer->clear(force ? MimeTreeParser::Force : MimeTreeParser::Delayed);
}

void KMReaderWin::setMessage(const Akonadi::Item &item, MimeTreeParser::UpdateMode updateMode)
{
    qCDebug(KMAIL_LOG) << Q_FUNC_INFO << parentWidget();
    mViewer->setMessageItem(item, updateMode);
}

void KMReaderWin::setMessage(const KMime::Message::Ptr &message)
{
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
        QSharedPointer<FolderCollection> fd = FolderCollection::forCollection(
                MailCommon::Util::updatedCollection(item.parentCollection()), false);
        KMail::Util::handleClickedURL(url, fd);
        return;
    }
    //No folder so we can't have identity and template.
    KMail::Util::handleClickedURL(url);
}

void KMReaderWin::slotShowReader(KMime::Content *msgPart, bool html, const QString &encoding)
{
    const MessageViewer::Viewer::DisplayFormatMessage format = html ? MessageViewer::Viewer::Html : MessageViewer::Viewer::Text;
    KMReaderMainWin *win = new KMReaderMainWin(msgPart, format, encoding);
    win->show();
}

void KMReaderWin::slotShowMessage(const KMime::Message::Ptr &message, const QString &encoding)
{
    KMReaderMainWin *win = new KMReaderMainWin();
    win->showMessage(encoding, message);
    win->show();
}

void KMReaderWin::slotDeleteMessage(const Akonadi::Item &item)
{
    if (!item.isValid()) {
        return;
    }
    KMTrashMsgCommand *command = new KMTrashMsgCommand(item.parentCollection(), item, -1);
    command->start();
}

bool KMReaderWin::printSelectedText(bool preview)
{
    const QString str = mViewer->selectedText();
    if (str.isEmpty()) {
        return false;
    }
    ::MessageComposer::Composer *composer = new ::MessageComposer::Composer;
    composer->textPart()->setCleanPlainText(str);
    composer->textPart()->setWrappedPlainText(str);
    KMime::Message::Ptr messagePtr = message().payload<KMime::Message::Ptr>();
    composer->infoPart()->setFrom(messagePtr->from()->asUnicodeString());
    composer->infoPart()->setTo(QStringList() << messagePtr->to()->asUnicodeString());
    composer->infoPart()->setCc(QStringList() << messagePtr->cc()->asUnicodeString());
    composer->infoPart()->setSubject(messagePtr->subject()->asUnicodeString());
    composer->setProperty("preview", preview);
    connect(composer, &::MessageComposer::Composer::result, this, &KMReaderWin::slotPrintComposeResult);
    composer->start();
    return true;
}

void KMReaderWin::slotPrintComposeResult(KJob *job)
{
    const bool preview = job->property("preview").toBool();
    ::MessageComposer::Composer *composer = dynamic_cast< ::MessageComposer::Composer * >(job);
    Q_ASSERT(composer);
    if (composer->error() == ::MessageComposer::Composer::NoError) {

        Q_ASSERT(composer->resultMessages().size() == 1);
        Akonadi::Item printItem;
        printItem.setPayload<KMime::Message::Ptr>(composer->resultMessages().first());
        Akonadi::MessageFlags::copyMessageFlags(*(composer->resultMessages().first()), printItem);
        const bool useFixedFont = MessageViewer::MessageViewerSettings::self()->useFixedFont();
        const QString overrideEncoding = MessageCore::MessageCoreSettings::self()->overrideCharacterEncoding();

        KMPrintCommand *command = new KMPrintCommand(this, printItem, mViewer->headerStylePlugin(),
                mViewer->displayFormatMessageOverwrite(), mViewer->htmlLoadExternal(), useFixedFont, overrideEncoding);
        command->setPrintPreview(preview);
        command->start();
    } else {
        if (static_cast<KIO::Job *>(job)->ui()) {
            static_cast<KIO::Job *>(job)->ui()->showErrorMessage();
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
            if (custom.contains(QStringLiteral("MailPreferedFormatting"))) {
                const QString value = mSearchedAddress.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("MailPreferedFormatting"));
                mViewAsHtml->setChecked(value == QLatin1String("HTML"));
            } else if (custom.contains(QStringLiteral("MailAllowToRemoteContent"))) {
                const QString value = mSearchedAddress.custom(QStringLiteral("KADDRESSBOOK"), QStringLiteral("MailAllowToRemoteContent"));
                mLoadExternalReference->setChecked((value == QLatin1String("TRUE")));
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

    KPIM::AddEmailDiplayJob *job = new KPIM::AddEmailDiplayJob(emailString, mMainWindow, this);
    job->setRemoteContent(mLoadExternalReference->isChecked());
    job->setShowAsHTML(mViewAsHtml->isChecked());
    job->setContact(mSearchedContact);
    job->start();
}

void KMReaderWin::slotEditContact()
{
    if (mSearchedContact.isValid()) {
        QPointer<Akonadi::ContactEditorDialog> dlg =
            new Akonadi::ContactEditorDialog(Akonadi::ContactEditorDialog::EditMode, this);
        connect(dlg.data(), &Akonadi::ContactEditorDialog::contactStored, this, &KMReaderWin::contactStored);
        connect(dlg.data(), &Akonadi::ContactEditorDialog::error, this, &KMReaderWin::slotContactEditorError);
        dlg->setContact(mSearchedContact);
        dlg->exec();
        delete dlg;
    }
}

void KMReaderWin::slotContactEditorError(const QString &error)
{
    KMessageBox::error(this, i18n("Contact cannot be stored: %1", error), i18n("Failed to store contact"));
}

void KMReaderWin::contactStored(const Akonadi::Item &item)
{
    Q_UNUSED(item);
    KPIM::BroadcastStatus::instance()->setStatusMsg(i18n("Contact modified successfully"));
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
