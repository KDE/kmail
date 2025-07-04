/*
  This file is part of KMail, the KDE mail client.
  SPDX-FileCopyrightText: 1997 Markus Wuebben <markus.wuebben@kde.org>
  SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_export.h"

#include <Akonadi/Item>
#include <KContacts/Addressee>
#include <MessageViewer/MDNWarningWidget>
#include <MessageViewer/Viewer>
#include <MimeTreeParser/BodyPart>
#include <QUrl>
#include <QWidget>
class KActionCollection;
class QAction;
class KToggleAction;
class QMenu;
namespace MessageViewer
{
class CSSHelper;
class DKIMViewerMenu;
}

namespace MimeTreeParser
{
class AttachmentStrategy;
}

class KJob;

/**
   This class implements a "reader window", that is a window
   used for reading or viewing messages.
*/

class KMAIL_EXPORT KMReaderWin : public QWidget
{
    Q_OBJECT

public:
    explicit KMReaderWin(QWidget *parent, QWidget *mainWindow, KActionCollection *actionCollection);
    ~KMReaderWin() override;

    /** Read settings from app's config file. */
    void readConfig();

    /** Get/set the message attachment strategy. */
    const MessageViewer::AttachmentStrategy *attachmentStrategy() const;

    void setAttachmentStrategy(const MessageViewer::AttachmentStrategy *strategy);

    /** Get selected override character encoding.
      @return The encoding selected by the user or an empty string if auto-detection
      is selected. */
    [[nodiscard]] QString overrideEncoding() const;
    /** Set the override character encoding. */
    void setOverrideEncoding(const QString &encoding);
    void setPrinting(bool enable);

    void setMessage(const Akonadi::Item &item, MimeTreeParser::UpdateMode updateMode = MimeTreeParser::Delayed);

    void setMessage(const KMime::Message::Ptr &message);

    /** Instead of settings a message to be shown sets a message part
      to be shown */
    void setMsgPart(KMime::Content *aMsgPart);

    /** Clear the reader and discard the current message. */
    void clear(bool force = false);

    void update(bool force = false);

    /** Return selected text */
    [[nodiscard]] QString copyText() const;

    /** Override default html mail setting */
    [[nodiscard]] MessageViewer::Viewer::DisplayFormatMessage displayFormatMessageOverwrite() const;
    void setDisplayFormatMessageOverwrite(MessageViewer::Viewer::DisplayFormatMessage format);

    /** Override default load external references setting */
    [[nodiscard]] bool htmlLoadExtOverride() const;
    void setHtmlLoadExtDefault(bool loadExtDefault);
    void setHtmlLoadExtOverride(bool loadExtOverride);

    /** Is html mail to be supported? Takes into account override */
    [[nodiscard]] bool htmlMail() const;

    /** Is loading ext. references to be supported? Takes into account override */
    [[nodiscard]] bool htmlLoadExternal();

    /** Display a generic HTML splash page instead of a message */
    void displaySplashPage(const QString &templateName, const QVariantHash &data);

    /** Display the 'please wait' page instead of a message */
    void displayBusyPage();
    /** Display the 'we are currently in offline mode' page instead of a message */
    void displayOfflinePage();

    void displayResourceOfflinePage();

    [[nodiscard]] bool isFixedFont() const;
    void setUseFixedFont(bool useFixedFont);
    [[nodiscard]] MessageViewer::Viewer *viewer() const;
    [[nodiscard]] KToggleAction *toggleFixFontAction() const;
    [[nodiscard]] QAction *mailToComposeAction() const;
    [[nodiscard]] QAction *mailToReplyAction() const;
    [[nodiscard]] QAction *mailToForwardAction() const;
    [[nodiscard]] QAction *addAddrBookAction() const;
    [[nodiscard]] QAction *openAddrBookAction() const;
    [[nodiscard]] QAction *copyAction() const;
    [[nodiscard]] QAction *selectAllAction() const;
    [[nodiscard]] QAction *copyURLAction() const;
    [[nodiscard]] QAction *copyImageLocation() const;
    [[nodiscard]] QAction *urlOpenAction() const;
    [[nodiscard]] QAction *urlSaveAsAction() const;
    [[nodiscard]] QAction *addUrlToBookmarkAction() const;
    [[nodiscard]] QAction *toggleMimePartTreeAction() const;
    [[nodiscard]] QAction *speakTextAction() const;
    [[nodiscard]] QAction *downloadImageToDiskAction() const;
    [[nodiscard]] QAction *viewSourceAction() const;
    [[nodiscard]] QAction *findInMessageAction() const;
    [[nodiscard]] QAction *saveAsAction() const;
    [[nodiscard]] QAction *saveMessageDisplayFormatAction() const;
    [[nodiscard]] QAction *resetMessageDisplayFormatAction() const;
    [[nodiscard]] QAction *editContactAction() const;
    [[nodiscard]] QAction *developmentToolsAction() const;
    [[nodiscard]] QAction *shareTextAction() const;

    [[nodiscard]] QMenu *viewHtmlOption() const;
    [[nodiscard]] QAction *shareImage() const;

    [[nodiscard]] QAction *addToExistingContactAction() const;

    [[nodiscard]] Akonadi::Item messageItem() const;

    [[nodiscard]] QWidget *mainWindow() const;
    [[nodiscard]] QAction *openImageAction() const;

    /** Enforce message decryption. */
    void setDecryptMessageOverwrite(bool overwrite = true);

    [[nodiscard]] MessageViewer::CSSHelper *cssHelper() const;

    [[nodiscard]] bool printSelectedText(bool preview);

    void setContactItem(const Akonadi::Item &contact, const KContacts::Addressee &address);
    void clearContactItem();

    [[nodiscard]] bool mimePartTreeIsEmpty() const;
    [[nodiscard]] KActionMenu *shareServiceUrlMenu() const;
    [[nodiscard]] MessageViewer::DKIMViewerMenu *dkimViewerMenu() const;
    [[nodiscard]] QList<QAction *> viewerPluginActionList(MessageViewer::ViewerPluginInterface::SpecificFeatureTypes features);

    [[nodiscard]] QList<QAction *> interceptorUrlActions(const WebEngineViewer::WebHitTestResult &result) const;

    void setPrintElementBackground(bool printElementBackground);
    /** Force update even if message is the same */
    void clearCache();

    void hasMultiMessages(bool multi);

    void updateShowMultiMessagesButton(bool enablePreviousButton, bool enableNextButton);
    [[nodiscard]] MessageViewer::RemoteContentMenu *remoteContentMenu() const;
    void addImageMenuActions(QMenu *menu);
Q_SIGNALS:
    void showStatusBarMessage(const QString &message);
    void zoomChanged(qreal factor);
    void showPreviousMessage();
    void showNextMessage();

public Q_SLOTS:

    /** The user selected "Find" from the menu. */
    void slotFind();
    /** Copy the selected text to the clipboard */
    void slotCopySelectedText();
    /** Operations on mailto: URLs. */
    void slotMailtoReply();
    void slotMailtoCompose();
    void slotMailtoForward();
    void slotMailtoAddAddrBook();
    void slotMailtoOpenAddrBook();
    /** Save the page to a file */
    void slotUrlSave();
    void slotAddUrlToBookmark();
    void slotUrlClicked(const Akonadi::Item &, const QUrl &);
    void slotShowReader(KMime::Content *, bool html, const QString &);
    void slotShowMessage(const KMime::Message::Ptr &message, const QString &encoding);
    void slotDeleteMessage(const Akonadi::Item &);
    void slotSaveImageOnDisk();

    void slotPrintComposeResult(KJob *job);
    void slotEditContact();
    void contactStored(const Akonadi::Item &item);
    void slotContactEditorError(const QString &error);

    void slotContactHtmlOptions();
    void slotShareImage();
    void slotMailToAddToExistingContact();
    void slotPrintingFinished();

protected:
    [[nodiscard]] QUrl urlClicked() const;
    [[nodiscard]] QUrl imageUrlClicked() const;

private:
    KMAIL_NO_EXPORT void createActions();
    KMAIL_NO_EXPORT void updateHtmlActions();
    KMAIL_NO_EXPORT void slotContactHtmlPreferencesUpdated(const Akonadi::Item &contact, Akonadi::Item::Id id, bool showAsHTML, bool remoteContent);
    KMAIL_NO_EXPORT void slotSendMdnResponse(MessageViewer::MDNWarningWidget::ResponseType type, KMime::MDN::SendingMode sendingMode);
    KMAIL_NO_EXPORT void sendMdnInfo(const Akonadi::Item &item);
    KMAIL_NO_EXPORT void slotShowMdnInfo(const QPair<QString, bool> &mdnInfo);
    KMAIL_NO_EXPORT void slotItemModified(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers);
    KMAIL_NO_EXPORT void slotOpenImage();

    KContacts::Addressee mSearchedAddress;
    Akonadi::Item mSearchedContact;
    QWidget *const mMainWindow;
    KActionCollection *const mActionCollection;

    QAction *mMailToComposeAction = nullptr;
    QAction *mMailToReplyAction = nullptr;
    QAction *mMailToForwardAction = nullptr;
    QAction *mAddAddrBookAction = nullptr;
    QAction *mOpenAddrBookAction = nullptr;
    QAction *mUrlSaveAsAction = nullptr;
    QAction *mAddUrlToBookmarkAction = nullptr;
    QAction *mImageUrlSaveAsAction = nullptr;
    QAction *mOpenImageAction = nullptr;
    QAction *mEditContactAction = nullptr;
    QAction *mViewAsHtml = nullptr;
    QAction *mLoadExternalReference = nullptr;
    QAction *mShareImage = nullptr;
    QAction *mAddEmailToExistingContactAction = nullptr;

    QMenu *mViewHtmlOptions = nullptr;

    MessageViewer::Viewer *mViewer = nullptr;
};
