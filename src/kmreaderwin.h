/*
  This file is part of KMail, the KDE mail client.
  SPDX-FileCopyrightText: 1997 Markus Wuebben <markus.wuebben@kde.org>
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_export.h"

#include <AkonadiCore/Item>
#include <KContacts/Addressee>
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
    Q_REQUIRED_RESULT QString overrideEncoding() const;
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
    Q_REQUIRED_RESULT QString copyText() const;

    /** Override default html mail setting */
    Q_REQUIRED_RESULT MessageViewer::Viewer::DisplayFormatMessage displayFormatMessageOverwrite() const;
    void setDisplayFormatMessageOverwrite(MessageViewer::Viewer::DisplayFormatMessage format);

    /** Override default load external references setting */
    Q_REQUIRED_RESULT bool htmlLoadExtOverride() const;
    void setHtmlLoadExtDefault(bool loadExtDefault);
    void setHtmlLoadExtOverride(bool loadExtOverride);

    /** Is html mail to be supported? Takes into account override */
    Q_REQUIRED_RESULT bool htmlMail() const;

    /** Is loading ext. references to be supported? Takes into account override */
    Q_REQUIRED_RESULT bool htmlLoadExternal();

    /** Returns the MD5 hash for the list of new features */
    static Q_REQUIRED_RESULT QString newFeaturesMD5();

    /** Display a generic HTML splash page instead of a message */
    void displaySplashPage(const QString &templateName, const QVariantHash &data);

    /** Display the about page instead of a message */
    void displayAboutPage();

    /** Display the 'please wait' page instead of a message */
    void displayBusyPage();
    /** Display the 'we are currently in offline mode' page instead of a message */
    void displayOfflinePage();

    void displayResourceOfflinePage();

    Q_REQUIRED_RESULT bool isFixedFont() const;
    void setUseFixedFont(bool useFixedFont);
    MessageViewer::Viewer *viewer() const;
    KToggleAction *toggleFixFontAction() const;
    QAction *mailToComposeAction() const;
    QAction *mailToReplyAction() const;
    QAction *mailToForwardAction() const;
    QAction *addAddrBookAction() const;
    QAction *openAddrBookAction() const;
    QAction *copyAction() const;
    QAction *selectAllAction() const;
    QAction *copyURLAction() const;
    QAction *copyImageLocation() const;
    QAction *urlOpenAction() const;
    QAction *urlSaveAsAction() const;
    QAction *addBookmarksAction() const;
    QAction *toggleMimePartTreeAction() const;
    QAction *speakTextAction() const;
    QAction *downloadImageToDiskAction() const;
    QAction *viewSourceAction() const;
    QAction *findInMessageAction() const;
    QAction *saveAsAction() const;
    QAction *saveMessageDisplayFormatAction() const;
    QAction *resetMessageDisplayFormatAction() const;
    QAction *editContactAction() const;
    QAction *developmentToolsAction() const;
    QAction *shareTextAction() const;

    QMenu *viewHtmlOption() const;
    QAction *shareImage() const;

    QAction *addToExistingContactAction() const;

    Akonadi::Item messageItem() const;

    QWidget *mainWindow() const;

    /** Enforce message decryption. */
    void setDecryptMessageOverwrite(bool overwrite = true);

    MessageViewer::CSSHelper *cssHelper() const;

    Q_REQUIRED_RESULT bool printSelectedText(bool preview);

    void setContactItem(const Akonadi::Item &contact, const KContacts::Addressee &address);
    void clearContactItem();

    Q_REQUIRED_RESULT bool mimePartTreeIsEmpty() const;
    KActionMenu *shareServiceUrlMenu() const;
    MessageViewer::DKIMViewerMenu *dkimViewerMenu() const;
    Q_REQUIRED_RESULT QList<QAction *> viewerPluginActionList(MessageViewer::ViewerPluginInterface::SpecificFeatureTypes features);

    Q_REQUIRED_RESULT QList<QAction *> interceptorUrlActions(const WebEngineViewer::WebHitTestResult &result) const;

    void setPrintElementBackground(bool printElementBackground);
    /** Force update even if message is the same */
    void clearCache();

    void hasMultiMessages(bool multi);

    void updateShowMultiMessagesButton(bool enablePreviousButton, bool enableNextButton);
    MessageViewer::RemoteContentMenu *remoteContentMenu() const;
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
    void slotAddBookmarks();
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
    Q_REQUIRED_RESULT QUrl urlClicked() const;
    Q_REQUIRED_RESULT QUrl imageUrlClicked() const;

private:
    void createActions();
    void updateHtmlActions();
    void slotContactHtmlPreferencesUpdated(const Akonadi::Item &contact, Akonadi::Item::Id id, bool showAsHTML, bool remoteContent);

private:
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
    QAction *mAddBookmarksAction = nullptr;
    QAction *mImageUrlSaveAsAction = nullptr;
    QAction *mEditContactAction = nullptr;
    QAction *mViewAsHtml = nullptr;
    QAction *mLoadExternalReference = nullptr;
    QAction *mShareImage = nullptr;
    QAction *mAddEmailToExistingContactAction = nullptr;

    QMenu *mViewHtmlOptions = nullptr;

    MessageViewer::Viewer *mViewer = nullptr;
};

