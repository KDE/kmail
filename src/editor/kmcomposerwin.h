/*
 * This file is part of KMail.
 * Copyright (c) 2011-2017 Laurent Montel <montel@kde.org>
 *
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Based on KMail code by:
 * Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __KMComposeWin
#define __KMComposeWin

// KMail includes
#include "editor/composer.h"
#include "MessageComposer/RecipientsEditor"

// Qt includes
#include <QFont>
#include <QList>

// LIBKDEPIM includes
#include "MessageComposer/RichTextComposerNg"

#include <MessageComposer/MessageSender>

// KDEPIMLIBS includes
#include <kmime/kmime_message.h>
#include <kmime/kmime_headers.h>

// Other includes
#include "Libkleo/Enum"
#include <messagecomposer/composerviewbase.h>

class QUrl;

class QByteArray;
class QGridLayout;
class QLabel;
class QSplitter;
class KSplitterCollapserButton;
class CodecAction;
class KMComposerWin;
class KMComposerEditor;
class KSelectAction;
class QAction;
class KJob;
class KToggleAction;
class QUrl;
class KRecentFilesAction;
class SnippetWidget;
class AttachmentMissingWarning;
class ExternalEditorWarning;
class CryptoStateIndicatorWidget;
class StatusBarLabelToggledState;
class PotentialPhishingEmailWarning;
class KMComposerGlobalAction;
class KMailPluginEditorManagerInterface;
class KMailPluginEditorCheckBeforeSendManagerInterface;
class KMailPluginEditorInitManagerInterface;
class IncorrectIdentityFolderWarning;
namespace MailTransport {
class Transport;
}

namespace KIdentityManagement {
class Identity;
}

namespace KPIMTextEdit {
class RichTextEditorWidget;
}

namespace KIO {
class Job;
}

namespace MessageComposer {
class ComposerLineEdit;
class Composer;
}

namespace MailCommon {
class FolderRequester;
}

namespace PimCommon {
class CustomToolsWidgetNg;
class LineEditWithAutoCorrection;
}

namespace GpgME {
class KeyListResult;
class Key;
class UserID;
}

//-----------------------------------------------------------------------------
class KMComposerWin : public KMail::Composer
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kmail.mailcomposer")

    friend class ::KMComposerEditor;

private: // mailserviceimpl, kmkernel, kmcommands, callback, kmmainwidget
    explicit KMComposerWin(const KMime::Message::Ptr &msg, bool lastSignState, bool lastEncryptState, TemplateContext context = NoTemplate, uint identity = 0,
                           const QString &textSelection = QString(), const QString &customTemplate = QString());
    ~KMComposerWin();

public:
    static Composer *create(const KMime::Message::Ptr &msg, bool lastSignState, bool lastEncryptState, TemplateContext context = NoTemplate, uint identity = 0,
                            const QString &textSelection = QString(), const QString &customTemplate = QString());

    QString dbusObjectPath() const override;
    QString smartQuote(const QString &msg);

    /**
    * Start of D-Bus callable stuff. The D-Bus methods need to be public slots,
    * otherwise they can't be accessed.
    */
    // TODO clean-up dbus stuff; make the adaptor a friend; etc.
public Q_SLOTS:

    Q_SCRIPTABLE void send(int how) override;

    Q_SCRIPTABLE void addAttachmentsAndSend(const QList<QUrl> &urls, const QString &comment, int how) override;

    Q_SCRIPTABLE void addAttachment(const QUrl &url, const QString &comment) override;

    Q_SCRIPTABLE void addAttachment(const QString &name, KMime::Headers::contentEncoding cte, const QString &charset, const QByteArray &data, const QByteArray &mimeType) override;

    /**
    * End of D-Bus callable stuff
    */

Q_SIGNALS:
    void identityChanged(const KIdentityManagement::Identity &identity);

public: // kmkernel, kmcommands, callback
    /**
     * Set the message the composer shall work with. This discards
     * previous messages without calling applyChanges() on them before.
     */
    void setMessage(const KMime::Message::Ptr &newMsg, bool lastSignState = false, bool lastEncryptState = false, bool mayAutoSign = true, bool allowDecryption = false,
                    bool isModified = false) override;

    void setCurrentTransport(int transportId) override;

    /**
     * Use the given folder as sent-mail folder if the given folder exists.
     * Else show an error message and use the default sent-mail folder as
     * sent-mail folder.
     */
    void setFcc(const QString &idString) override;

    /**
      * Disables word wrap completely. No wrapping at all will occur, not even
      * at the right end of the editor.
      * This is useful when sending invitations.
      */
    void disableWordWrap() override;

    /**
      * Disables HTML completely. It disables HTML at the point of calling this and disables it
      * again when sending the message, to be sure. Useful when sending invitations.
      * This will <b>not</b> remove the actions for activating HTML mode again, it is only
      * meant for automatic invitation sending.
      * Also calls @sa disableHtml() internally.
      */
    void forceDisableHtml() override;

    /**
      * Returns @c true while the message composing is in progress.
      */
    bool isComposing() const override;

    /** Disabled signing and encryption completely for this composer window. */
    void setSigningAndEncryptionDisabled(bool v) override;
    /**
     * If this folder is set, the original message is inserted back after
     * canceling
     */
    void setFolder(const Akonadi::Collection &aFolder) override;
    /**
     * Sets the focus to the edit-widget.
     */
    void setFocusToEditor() override;

    /**
     * Sets the focus to the subject line edit. For use when creating a
     * message to a known recipient.
     */
    void setFocusToSubject() override;

    bool insertFromMimeData(const QMimeData *source, bool forceAttachment = false);

    void setCurrentReplyTo(const QString &) override;
    void setCollectionForNewMessage(const Akonadi::Collection &folder) override;

    void addExtraCustomHeaders(const QMap<QByteArray, QString> &header) override;

private:
    /**
    * Write settings to app's config file.
    */
    void writeConfig(void);

    /**
     * Returns true if the message was modified by the user.
     */
    bool isModified() const;
    bool isComposerModified() const;
    void changeModifiedState(bool modified);

public Q_SLOTS: // kmkernel, callback
    void slotSendNow() override;
    /**
     * Switch wordWrap on/off
     */
    void slotWordWrapToggled(bool);

    void slotToggleMarkup();
    void slotTextModeChanged(MessageComposer::RichTextComposerNg::Mode mode);
    void htmlToolBarVisibilityChanged(bool visible);
    void slotSpellcheckDoneClearStatus();
    void autoSaveMessage(bool force = false) override;
    /**
     * Set whether the message should be treated as modified or not.
     */
    void setModified(bool modified) override;
    void slotFetchJob(KJob *);

private Q_SLOTS:
    /**
      * Disables the HTML mode, by hiding the HTML toolbar and unchecking the
      * "Formatting" action. Also, removes all rich-text formatting.
      */
    void disableHtml(MessageComposer::ComposerViewBase::Confirmation confirmation);
    /**
     * Enables HTML mode, by showing the HTML toolbar and checking the
     * "Formatting" action
     */
    void enableHtml();

    /**
     * Actions:
     */
    void slotPrint();
    void slotPrintPreview();

    void slotInsertRecentFile(const QUrl &);
    void slotRecentListFileClear();

    void slotSendNowVia(MailTransport::Transport *transport);
    void slotSendLater();
    void slotSendLaterVia(MailTransport::Transport *transport);

    /**
     * Returns true when saving was successful.
     */
    void slotSaveDraft();
    void slotSaveTemplate();
    void slotNewComposer();
    void slotClose();
    void slotPasteAsAttachment();
    void slotFolderRemoved(const Akonadi::Collection &);
    void slotDictionaryLanguageChanged(const QString &language);
    void slotFccFolderChanged(const Akonadi::Collection &);
    void slotEditorTextChanged();
    void slotOverwriteModeChanged();
    /**
     * toggle fixed width font.
     */
    void slotUpdateFont();

    /**
     * Open addressbook editor dialog.
     */
    void slotAddressBook();

    /**
     * Insert a file to the end of the text in the editor.
     */
    void slotInsertFile();

    /**
     * Check spelling of text.
     */
    void slotSpellcheckConfig();

    /**
     * Change crypto plugin to be used for signing/encrypting messages,
     * or switch to built-in OpenPGP code.
     */
    void slotSelectCryptoModule(bool init);

    /**
     * XML-GUI stuff
     */
    void slotEditToolbars();
    void slotUpdateToolbars();
    void slotEditKeys();

    /**
     * Change window title to given string.
     */
    void slotUpdateWindowTitle();

    /**
     * Switch the icon to lock or unlock respectivly.
     * Change states of all encrypt check boxes in the attachments listview
     */
    void slotEncryptToggled(bool);

    /**
     * Change states of all sign check boxes in the attachments listview
     */
    void slotSignToggled(bool);

    void slotUpdateView();

    /**
     * Update composer field to reflect new identity
     * @param initalChange if true, don't apply the template. This is useful when calling
     *                     this function from setMsg(), because there, the message already has the
     *                     template, and we want to avoid calling the template parser unnecessarily.
     */
    void slotIdentityChanged(uint uoid, bool initalChange = false);

    void slotCursorPositionChanged();

    void slotSpellCheckingStatus(const QString &status);

    void slotDelayedApplyTemplate(KJob *);

    void recipientEditorSizeHintChanged();
    void setMaximumHeaderSize();
    void slotDoDelayedSend(KJob *);

    void slotCompletionModeChanged(KCompletion::CompletionMode);
    void slotConfigChanged();

    void slotPrintComposeResult(KJob *job);

    void slotSendFailed(const QString &msg, MessageComposer::ComposerViewBase::FailedType type);
    void slotSendSuccessful();

    /**
     *  toggle automatic spellchecking
     */
    void slotAutoSpellCheckingToggled(bool);

    void setAutoSaveFileName(const QString &fileName) override;
    void slotSpellCheckingLanguage(const QString &language);
    void forceAutoSaveMessage();
    void slotSaveAsFile();

    void slotAttachMissingFile();
    void slotExplicitClosedMissingAttachment();
    void slotVerifyMissingAttachmentTimeout();
    void slotCheckSendNow();

    void slotExternalEditorStarted();
    void slotExternalEditorClosed();

    void slotInsertShortUrl(const QString &url);

    void slotTransportChanged();
    void slotFollowUpMail(bool toggled);
    void slotSendNowByShortcut();
    void slotSnippetWidgetVisibilityChanged(bool b);
    void slotOverwriteModeWasChanged(bool state);
    void slotExpandGroupResult(KJob *job);
    void slotCheckSendNowStep2();
    void slotPotentialPhishingEmailsFound(const QStringList &list);
    void slotInsertTextFile(KJob *job);

    void slotRecipientEditorLineAdded(KPIM::MultiplyingLine *line);
    void slotRecipientEditorFocusChanged();
    void slotRecipientAdded(MessageComposer::RecipientLineNG *line);
    void slotRecipientLineIconClicked(MessageComposer::RecipientLineNG *line);
    void slotRecipientFocusLost(MessageComposer::RecipientLineNG *line);
    void slotKeyForMailBoxResult(const GpgME::KeyListResult &result, const GpgME::Key &key, const GpgME::UserID &userID);

    void slotDelayedCheckSendNow();
    void slotUpdateComposer(const KIdentityManagement::Identity &ident, const KMime::Message::Ptr &msg, uint uoid, uint uoldId, bool wasModified);
public: // kmcommand
    void addAttach(KMime::Content *msgPart) override;

    const KIdentityManagement::Identity &identity() const;

    /** Don't check for forgotten attachments for a mail, eg. when sending out invitations. */
    void disableForgottenAttachmentsCheck() override;

    uint currentIdentity() const;
    QList<KToggleAction *> customToolsList() const;
    QList<QAction *> pluginToolsActionListForPopupMenu() const;
private:
    /**
     * Read settings from app's config file.
     */
    void readConfig(bool reload = false);

    QUrl insertFile();
    /**
     * Updates the visibility and text of the signature and encryption state indicators.
     */
    void updateSignatureAndEncryptionStateIndicators();

    void confirmBeforeSend();
    void sendNow(bool shortcutUsed);

    void updateSignature(uint uoid, uint uOldId);
    Kleo::CryptoMessageFormat cryptoMessageFormat() const;
    void printComposeResult(KJob *job, bool preview);
    void printComposer(bool preview);
    /**
     * Install grid management and header fields. If fields exist that
     * should not be there they are removed. Those that are needed are
     * created if necessary.
     */
    void rethinkFields(bool fromslot = false, bool forceAllHeaders = false);

    /**
      Connect signals for moving focus by arrow keys. Returns next edit.
    */
    QWidget *connectFocusMoving(QWidget *prev, QWidget *next);

    /**
     * Show or hide header lines
     */
    void rethinkHeaderLine(int value, int mask, int &row, QLabel *lbl, QWidget *cbx);  // krazy:exclude=qclasses

    /**
     * Apply template to new or unmodified message.
     */
    void applyTemplate(uint uoid, uint uOldId, const KIdentityManagement::Identity &ident, bool wasModified);

    /**
     * Set the quote prefix according to identity.
     */
    void setQuotePrefix(uint uoid);

    /**
     * Checks how many recipients are and warns if there are too many.
     * @return true, if the user accepted the warning and the message should be sent
    */
    bool checkRecipientNumber() const;

    /**
     * Initialization methods
     */
    void setupActions();
    void setupStatusBar(QWidget *w);
    void setupEditor();

    /**
     * Header fields.
     */
    QString subject() const;
    QString from() const;
    QString replyTo() const;

    /**
     * Ask for confirmation if the message was changed before close.
     */
    bool queryClose() override;

    /**
     * Turn encryption on/off. If setByUser is true then a message box is shown
     * in case encryption isn't possible.
     */
    void setEncryption(bool encrypt, bool setByUser = false);

    /**
     * Turn signing on/off. If setByUser is true then a message box is shown
     * in case signing isn't possible.
     */
    void setSigning(bool sign, bool setByUser = false);

    MessageComposer::ComposerViewBase::MissingAttachment userForgotAttachment();
    /**
     * Send the message.
     */
    void doSend(MessageComposer::MessageSender::SendMethod method = MessageComposer::MessageSender::SendDefault,
                MessageComposer::MessageSender::SaveIn saveIn = MessageComposer::MessageSender::SaveInNone, bool willSendItWithoutReediting = false);

    void doDelayedSend(MessageComposer::MessageSender::SendMethod method, MessageComposer::MessageSender::SaveIn saveIn);

    void changeCryptoAction();
    void applyComposerSetting(MessageComposer::ComposerViewBase *mComposerBase);
    /**
     * Creates a simple composer that creates a KMime::Message out of the composer content.
     * Crypto handling is not done, therefore the name "simple".
     * This is used when autosaving or printing a message.
     *
     * The caller takes ownership of the composer.
     */
    MessageComposer::Composer *createSimpleComposer();

    bool canSignEncryptAttachments() const;

    // helper method for rethinkFields
    int calcColumnWidth(int which, long allShowing, int width) const;

    /** Initialize header fields. Should be called on new messages
      if they are not set manually. E.g. before composing. Calling
      of setAutomaticFields(), see below, is still required. */
    void initHeader(KMime::Message *message, uint identity = 0);

    inline bool encryptToSelf() const;

private:
    enum CryptoKeyState {
        NoState = 0,
        InProgress,
        KeyOk,
        NoKey
    };
    void slotToggleMenubar(bool dontShowWarning);

    void slotCryptoModuleSelected();
    void slotFccIsInvalid();
    void slotIdentityDeleted(uint uoid);
    void slotTransportRemoved(int id, const QString &name);

    void updateComposerAfterIdentityChanged(const KIdentityManagement::Identity &ident, uint uoid, bool wasModified);

    void insertUrls(const QMimeData *source, const QList<QUrl> &urlList);
    void initializePluginActions();
    bool showErrorMessage(KJob *job);
    int validateLineWrapWidth();
    void slotSelectionChanged();
    void slotMessage(const QString &str);


    Akonadi::Collection mCollectionForNewMessage;
    QMap<QByteArray, QString> mExtraHeaders;

    QWidget *mMainWidget = nullptr;
    MessageComposer::ComposerLineEdit *mEdtFrom = nullptr;
    MessageComposer::ComposerLineEdit *mEdtReplyTo = nullptr;
    PimCommon::LineEditWithAutoCorrection *mEdtSubject = nullptr;
    QLabel *mLblIdentity = nullptr;
    QLabel *mLblTransport = nullptr;
    QLabel *mLblFcc = nullptr;
    QLabel *mLblFrom = nullptr;
    QLabel *mLblReplyTo = nullptr;
    QLabel *mLblSubject = nullptr;
    QLabel *mDictionaryLabel = nullptr;
    QLabel *mCursorLineLabel = nullptr;
    QLabel *mCursorColumnLabel = nullptr;
    QLabel *mStatusbarLabel = nullptr;
    bool mDone = false;

    KMime::Message::Ptr mMsg;
    QGridLayout *mGrid = nullptr;
    QString mTextSelection;
    QString mCustomTemplate;
    bool mLastSignActionState = false;
    bool mLastEncryptActionState = false;
    bool mSigningAndEncryptionExplicitlyDisabled = false;
    bool mLastIdentityHasSigningKey = false;
    bool mLastIdentityHasEncryptionKey = false;
    Akonadi::Collection mFolder;
    long mShowHeaders = 0;
    bool mForceDisableHtml = false;     // Completely disable any HTML. Useful when sending invitations in the
    // mail body.
    int mNumHeaders = 0;
    QFont mBodyFont;
    QFont mFixedFont;
    uint mId = 0;
    TemplateContext mContext;

    KRecentFilesAction *mRecentAction = nullptr;

    KToggleAction *mSignAction = nullptr;
    KToggleAction *mEncryptAction = nullptr;
    KToggleAction *mRequestMDNAction = nullptr;
    KToggleAction *mUrgentAction = nullptr;
    KToggleAction *mAllFieldsAction = nullptr;
    KToggleAction *mFromAction = nullptr;
    KToggleAction *mReplyToAction = nullptr;
    KToggleAction *mSubjectAction = nullptr;
    KToggleAction *mIdentityAction = nullptr;
    KToggleAction *mTransportAction = nullptr;
    KToggleAction *mFccAction = nullptr;
    KToggleAction *mWordWrapAction = nullptr;
    KToggleAction *mFixedFontAction = nullptr;
    KToggleAction *mAutoSpellCheckingAction = nullptr;
    KToggleAction *mDictionaryAction = nullptr;
    KToggleAction *mSnippetAction = nullptr;
    KToggleAction *mHideMenuBarAction = nullptr;
    QAction *mAppendSignature = nullptr;
    QAction *mPrependSignature = nullptr;
    QAction *mInsertSignatureAtCursorPosition = nullptr;

    KToggleAction *mMarkupAction = nullptr;

    CodecAction *mCodecAction = nullptr;
    KSelectAction *mCryptoModuleAction = nullptr;

    QAction *mFindText = nullptr;
    QAction *mFindNextText = nullptr;
    QAction *mReplaceText = nullptr;
    QAction *mSelectAll = nullptr;

    QSplitter *mHeadersToEditorSplitter = nullptr;
    QWidget *mHeadersArea = nullptr;
    QSplitter *mSplitter = nullptr;
    QSplitter *mSnippetSplitter = nullptr;
    QByteArray mOriginalPreferredCharset = nullptr;

    MessageComposer::Composer *mDummyComposer = nullptr;
    // used for auto saving, printing, etc. Not for sending, which happens in ComposerViewBase
    QList< MessageComposer::Composer * > mMiscComposers;

    int mLabelWidth = 0;

    QString mdbusObjectPath;
    static int s_composerNumber;

    MessageComposer::ComposerViewBase *mComposerBase = nullptr;

    SnippetWidget *mSnippetWidget = nullptr;
    PimCommon::CustomToolsWidgetNg *mCustomToolsWidget = nullptr;
    AttachmentMissingWarning *mAttachmentMissing = nullptr;
    ExternalEditorWarning *mExternalEditorWarning = nullptr;
    QTimer *mVerifyMissingAttachment = nullptr;
    MailCommon::FolderRequester *mFccFolder = nullptr;
    bool mPreventFccOverwrite = false;
    bool mCheckForForgottenAttachments = true;
    bool mWasModified = false;
    CryptoStateIndicatorWidget *mCryptoStateIndicatorWidget = nullptr;
    bool mSendNowByShortcutUsed = false;
    KSplitterCollapserButton *mSnippetSplitterCollapser = nullptr;
    KToggleAction *mFollowUpToggleAction = nullptr;
    StatusBarLabelToggledState *mStatusBarLabelToggledOverrideMode = nullptr;
    StatusBarLabelToggledState *mStatusBarLabelSpellCheckingChangeMode = nullptr;
    PotentialPhishingEmailWarning *mPotentialPhishingEmailWarning = nullptr;
    IncorrectIdentityFolderWarning *mIncorrectIdentityFolderWarning = nullptr;
    KMComposerGlobalAction *mGlobalAction = nullptr;
    KPIMTextEdit::RichTextEditorWidget *mRichTextEditorwidget = nullptr;

    KMailPluginEditorManagerInterface *mPluginEditorManagerInterface = nullptr;
    KMailPluginEditorCheckBeforeSendManagerInterface *mPluginEditorCheckBeforeSendManagerInterface = nullptr;
    KMailPluginEditorInitManagerInterface *mPluginEditorInitManagerInterface = nullptr;
};

#endif
