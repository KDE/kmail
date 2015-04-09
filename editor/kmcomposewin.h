/*
 * This file is part of KMail.
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
#include "messagecomposer/recipient/recipientseditor.h"

// Qt includes
#include <QFont>
#include <QList>

// KDE includes
#include <kglobalsettings.h>

// LIBKDEPIM includes
#include <messagecomposer/composer/kmeditor.h>

#include "messagecomposer/sender/messagesender.h"

// KDEPIMLIBS includes
#include <kmime/kmime_message.h>
#include <kmime/kmime_headers.h>

// Other includes
#include "kleo/enum.h"
#include <composer/composerviewbase.h>

class QByteArray;
class QCheckBox;
class QGridLayout;
class QLabel;
class QPushButton;
class QSplitter;

class CodecAction;
class KActionMenuChangeCase;
class KMComposeWin;
class KMComposerEditor;
class KSelectAction;
class KAction;
class KJob;
class KToggleAction;
class KUrl;
class KRecentFilesAction;
class SnippetWidget;
class SelectSpecialChar;
class AttachmentMissingWarning;
class ExternalEditorWarning;
class KActionMenu;
class CryptoStateIndicatorWidget;
class StatusBarLabelToggledState;
class PotentialPhishingEmailWarning;
class KMComposerGlobalAction;
namespace boost {
template <typename T> class shared_ptr;
}
namespace MailTransport {
class Transport;
}
namespace KPIMIdentities {
class Identity;
}

namespace KPIMTextEdit {
class SelectSpecialChar;
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
class CustomToolsWidget;
class LineEditWithAutoCorrection;
class SplitterCollapser;
}
class KMStorageService;
//-----------------------------------------------------------------------------
class KMComposeWin : public KMail::Composer
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kmail.mailcomposer")

    friend class ::KMComposerEditor;

private: // mailserviceimpl, kmkernel, kmcommands, callback, kmmainwidget
    explicit KMComposeWin(const KMime::Message::Ptr &msg, bool lastSignState, bool lastEncryptState, TemplateContext context = NoTemplate,
                          uint identity = 0, const QString & textSelection = QString(),
                          const QString & customTemplate = QString() );
    ~KMComposeWin();

public:
    static Composer *create( const KMime::Message::Ptr &msg, bool lastSignState, bool lastEncryptState, TemplateContext context = NoTemplate,
                             uint identity = 0, const QString & textSelection = QString(),
                             const QString & customTemplate = QString() );

    QString dbusObjectPath() const;
    QString smartQuote( const QString & msg );

    /**
   * Start of D-Bus callable stuff. The D-Bus methods need to be public slots,
   * otherwise they can't be accessed.
   */
    // TODO clean-up dbus stuff; make the adaptor a friend; etc.
public slots:

    Q_SCRIPTABLE void send( int how );

    Q_SCRIPTABLE void addAttachmentsAndSend( const KUrl::List & urls,
                                             const QString & comment,
                                             int how );

    Q_SCRIPTABLE void addAttachment( const KUrl & url,
                                     const QString & comment );

    Q_SCRIPTABLE void addAttachment( const QString & name,
                                     KMime::Headers::contentEncoding cte,
                                     const QString& charset,
                                     const QByteArray & data,
                                     const QByteArray & mimeType );

    /**
   * End of D-Bus callable stuff
   */

signals:
    void identityChanged( const KPIMIdentities::Identity &identity );


public: // kmkernel, kmcommands, callback
    /**
     * Set the message the composer shall work with. This discards
     * previous messages without calling applyChanges() on them before.
     */
    void setMessage( const KMime::Message::Ptr &newMsg, bool lastSignState = false, bool lastEncryptState = false,
                     bool mayAutoSign=true, bool allowDecryption=false, bool isModified=false );

    void setCurrentTransport( int transportId );

    /**
     * Use the given folder as sent-mail folder if the given folder exists.
     * Else show an error message and use the default sent-mail folder as
     * sent-mail folder.
     */
    void setFcc( const QString &idString );

    /**
      * Disables word wrap completely. No wrapping at all will occur, not even
      * at the right end of the editor.
      * This is useful when sending invitations.
      */
    void disableWordWrap();

    /**
      * Disables HTML completely. It disables HTML at the point of calling this and disables it
      * again when sending the message, to be sure. Useful when sending invitations.
      * This will <b>not</b> remove the actions for activating HTML mode again, it is only
      * meant for automatic invitation sending.
      * Also calls @sa disableHtml() internally.
      */
    void forceDisableHtml();

    /**
      * Returns @c true while the message composing is in progress.
      */
    bool isComposing() const;

    /**
      * Set the text selection the message is a response to.
      */
    void setTextSelection( const QString& selection );

    /**
      * Set custom template to be used for the message.
      */
    void setCustomTemplate( const QString& customTemplate );

    /** Disabled signing and encryption completely for this composer window. */
    void setSigningAndEncryptionDisabled( bool v );
    /**
     * If this folder is set, the original message is inserted back after
     * canceling
     */
    void setFolder(const Akonadi::Collection &aFolder );
    /**
     * Sets the focus to the edit-widget.
     */
    void setFocusToEditor();

    /**
     * Sets the focus to the subject line edit. For use when creating a
     * message to a known recipient.
     */
    void setFocusToSubject();

    bool insertFromMimeData( const QMimeData *source, bool forceAttachment = false );

    void setCurrentReplyTo(const QString&);
    void setCollectionForNewMessage( const Akonadi::Collection& folder);

    void addExtraCustomHeaders( const QMap<QByteArray, QString> &header);

    KToggleAction *translateAction() const { return mTranslateAction; }
    KActionMenuChangeCase *changeCaseMenu() const { return mChangeCaseMenu; }
    KToggleAction *generateShortenUrlAction() const { return mGenerateShortenUrl; }

private:
    /**
   * Write settings to app's config file.
   */
    void writeConfig( void );


    /**
     * Returns message of the composer. To apply the user changes to the
     * message, call applyChanges() first.
     */
    KMime::Message::Ptr msg() const { return mMsg; }
    /**
     * Returns true if the message was modified by the user.
     */
    bool isModified() const;
    bool isComposerModified() const;
    void changeModifiedState( bool modified );



    /**
     * determines whether inline signing/encryption is selected
     */
    bool inlineSigningEncryptionSelected();


public slots: // kmkernel, callback
    void slotSendNow();
    /**
     * Switch wordWrap on/off
     */
    void slotWordWrapToggled( bool );

    void slotToggleMarkup();
    void slotTextModeChanged( KRichTextEdit::Mode );
    void htmlToolBarVisibilityChanged( bool visible );
    void slotSpellcheckDoneClearStatus();
    void autoSaveMessage(bool force = false);
    /**
     * Set whether the message should be treated as modified or not.
     */
    void setModified( bool modified );
    void slotFetchJob(KJob*);

private slots:
    /**
      * Disables the HTML mode, by hiding the HTML toolbar and unchecking the
      * "Formatting" action. Also, removes all rich-text formatting.
      */
    void disableHtml( MessageComposer::ComposerViewBase::Confirmation confirmation );
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

    void slotInsertRecentFile( const KUrl & );
    void slotRecentListFileClear();

    void slotSendNowVia(MailTransport::Transport *transport );
    void slotSendLater();
    void slotSendLaterVia(MailTransport::Transport *transport );

    /**
     * Returns true when saving was successful.
     */
    void slotSaveDraft();
    void slotSaveTemplate();
    void slotNewComposer();
    void slotClose();
    void slotHelp();
    void slotPasteAsAttachment();
    void slotFolderRemoved( const Akonadi::Collection& );
    void slotLanguageChanged( const QString &language );
    void slotFccFolderChanged(const Akonadi::Collection&);
    void slotEditorTextChanged();
    void slotOverwriteModeChanged();
    /**
     * toggle fixed width font.
     */
    void slotUpdateFont();

    /**
     * Open addressbook editor dialog.
     */
    void slotAddrBook();

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
    void slotSelectCryptoModule( bool init = false );

    /**
     * XML-GUI stuff
     */
    void slotEditToolbars();
    void slotUpdateToolbars();
    void slotEditKeys();

    /**
     * Read settings from app's config file.
     */
    void readConfig( bool reload = false );

    /**
     * Change window title to given string.
     */
    void slotUpdWinTitle();

    /**
     * Switch the icon to lock or unlock respectivly.
     * Change states of all encrypt check boxes in the attachments listview
     */
    void slotEncryptToggled( bool );

    /**
     * Change states of all sign check boxes in the attachments listview
     */
    void slotSignToggled( bool );

    void slotUpdateView();

    /**
     * Update composer field to reflect new identity
     * @param initalChange if true, don't apply the template. This is useful when calling
     *                     this function from setMsg(), because there, the message already has the
     *                     template, and we want to avoid calling the template parser unnecessarily.
     */
    void slotIdentityChanged( uint uoid, bool initalChange = false );

    void slotCursorPositionChanged();

    void slotSpellCheckingStatus( const QString & status );

    void slotDelayedApplyTemplate( KJob* );

    void recipientEditorSizeHintChanged();
    void setMaximumHeaderSize();
    void slotDoDelayedSend( KJob* );

    void slotCompletionModeChanged( KGlobalSettings::Completion );
    void slotConfigChanged();

    void slotPrintComposeResult( KJob *job );
    
    void slotSendFailed(const QString& msg , MessageComposer::ComposerViewBase::FailedType type);
    void slotSendSuccessful(const QString &messageId);

    /**
     *  toggle automatic spellchecking
     */
    void slotAutoSpellCheckingToggled( bool );


    virtual void setAutoSaveFileName( const QString& fileName );
    void slotSpellCheckingLanguage(const QString& language);
    void forceAutoSaveMessage();
    void insertSpecialCharacter();
    void charSelected(const QChar& c);
    void slotSaveAsFile();
    void slotCreateAddressBookContact();

    void slotAttachMissingFile();
    void slotExplicitClosedMissingAttachment();
    void slotVerifyMissingAttachmentTimeout();
    void slotCheckSendNow();

    void slotUpperCase();
    void slotLowerCase();
    void slotSentenceCase();

    void slotExternalEditorStarted();
    void slotExternalEditorClosed();

    void slotInsertShortUrl(const QString &url);

    void slotShareLinkDone(const QString &fileName);

    void slotTransportChanged();
    void slotFollowUpMail(bool toggled);
    void slotSendNowByShortcut();
    void slotSnippetWidgetVisibilityChanged(bool b);
    void slotOverwriteModeWasChanged(bool state);
    void slotExpandGroupResult(KJob *job);
    void slotCheckSendNowStep2();
    void slotPotentialPhishingEmailsFound(const QStringList &list);
    void slotInsertTextFile(KJob *job);
public: // kmcommand
    // FIXME we need to remove these, but they're pure virtual in Composer.
    void addAttach( KMime::Content *msgPart );

    const KPIMIdentities::Identity &identity() const;

    /** Don't check for forgotten attachments for a mail, eg. when sending out invitations. */
    void disableForgottenAttachmentsCheck();

    /**
    * Ignore the "sticky" setting of the transport combo box and prefer the X-KMail-Transport
    * header field of the message instead.
    * Do the same for the identity combo box, don't obey the "sticky" setting but use the
    * X-KMail-Identity header field instead.
    *
    * This is useful when sending out invitations, since you don't see the GUI and want the
    * identity and transport to be set to the values stored in the messages.
    */
    void ignoreStickyFields();

private:
    /**
     * Updates the visibility and text of the signature and encryption state indicators.
     */
    void updateSignatureAndEncryptionStateIndicators();

    void addFollowupReminder(const QString &messageId);
    void confirmBeforeSend();
    void sendNow(bool shortcutUsed);

    void updateSignature(uint uoid, uint uOldId);
    Kleo::CryptoMessageFormat cryptoMessageFormat() const;
    void printComposeResult( KJob *job, bool preview );
    void printComposer(bool preview);
    /**
     * Install grid management and header fields. If fields exist that
     * should not be there they are removed. Those that are needed are
     * created if necessary.
     */
    void rethinkFields( bool fromslot=false );

    /**
      Connect signals for moving focus by arrow keys. Returns next edit.
    */
    QWidget *connectFocusMoving( QWidget *prev, QWidget *next );

    /**
     * Show or hide header lines
     */
    void rethinkHeaderLine( int aValue, int aMask, int &aRow,
                            QLabel *aLbl, QWidget *aEdt,
                            QPushButton *aBtn = 0 );

    void rethinkHeaderLine( int value, int mask, int &row,
                            QLabel *lbl, QWidget *cbx, QCheckBox *chk ); // krazy:exclude=qclasses


    /**
     * Apply template to new or unmodified message.
     */
    void applyTemplate(uint uoid , uint uOldId);

    /**
     * Set the quote prefix according to identity.
     */
    void setQuotePrefix( uint uoid );

    /**
     * Checks how many recipients are and warns if there are too many.
     * @return true, if the user accepted the warning and the message should be sent
    */
    bool checkRecipientNumber() const;

    bool checkTransport() const;
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
    virtual bool queryClose();

    /**
     * Turn encryption on/off. If setByUser is true then a message box is shown
     * in case encryption isn't possible.
     */
    void setEncryption( bool encrypt, bool setByUser = false );

    /**
     * Turn signing on/off. If setByUser is true then a message box is shown
     * in case signing isn't possible.
     */
    void setSigning( bool sign, bool setByUser = false );

    MessageComposer::ComposerViewBase::MissingAttachment userForgotAttachment();
#if 0
    /**
     * Decrypt an OpenPGP block or strip off the OpenPGP envelope of a text
     * block with a clear text signature. This is only done if the given
     * string contains exactly one OpenPGP block.
     * This function is for example used to restore the unencrypted/unsigned
     * message text for editting.
     */
    static void decryptOrStripOffCleartextSignature( QByteArray & );
#endif
    /**
     * Send the message.
     */
    void doSend( MessageComposer::MessageSender::SendMethod method=MessageComposer::MessageSender::SendDefault,
                 MessageComposer::MessageSender::SaveIn saveIn = MessageComposer::MessageSender::SaveInNone );

    void doDelayedSend( MessageComposer::MessageSender::SendMethod method, MessageComposer::MessageSender::SaveIn saveIn );

    void changeCryptoAction();
    void applyComposerSetting( MessageComposer::ComposerViewBase* mComposerBase );
    /**
     * Creates a simple composer that creates a KMime::Message out of the composer content.
     * Crypto handling is not done, therefore the name "simple".
     * This is used when autosaving or printing a message.
     *
     * The caller takes ownership of the composer.
     */
    MessageComposer::Composer* createSimpleComposer();

    bool canSignEncryptAttachments() const;

    QString addQuotesToText( const QString &inputText ) const;
    // helper method for rethinkFields
    int calcColumnWidth( int which, long allShowing, int width ) const;


    /** Initialize header fields. Should be called on new messages
      if they are not set manually. E.g. before composing. Calling
      of setAutomaticFields(), see below, is still required. */
    void initHeader( KMime::Message *message, uint identity=0 );

    inline bool encryptToSelf() const;

private:
    bool showErrorMessage(KJob *job);
    int validateLineWrapWidth();
    Akonadi::Collection mCollectionForNewMessage;
    QMap<QByteArray, QString> mExtraHeaders;


    QWidget   *mMainWidget;
    MessageComposer::ComposerLineEdit *mEdtFrom, *mEdtReplyTo;
    PimCommon::LineEditWithAutoCorrection *mEdtSubject;
    QLabel    *mLblIdentity, *mLblTransport, *mLblFcc;
    QLabel    *mLblFrom, *mLblReplyTo;
    QLabel    *mLblSubject;
    QLabel    *mDictionaryLabel;
    QCheckBox *mBtnIdentity, *mBtnDictionary, *mBtnTransport, *mBtnFcc;
    bool mDone;

    KMime::Message::Ptr mMsg;
    QGridLayout *mGrid;
    QString mTextSelection;
    QString mCustomTemplate;
    bool mLastSignActionState, mLastEncryptActionState, mSigningAndEncryptionExplicitlyDisabled;
    bool mLastIdentityHasSigningKey, mLastIdentityHasEncryptionKey;
    Akonadi::Collection mFolder;
    long mShowHeaders;
    bool mForceDisableHtml;     // Completely disable any HTML. Useful when sending invitations in the
    // mail body.
    int mNumHeaders;
    QFont mBodyFont, mFixedFont;
    uint mId;
    TemplateContext mContext;

    KRecentFilesAction *mRecentAction;

    KToggleAction *mSignAction, *mEncryptAction, *mRequestMDNAction;
    KToggleAction *mUrgentAction, *mAllFieldsAction, *mFromAction;
    KToggleAction *mReplyToAction;
    KToggleAction *mSubjectAction;
    KToggleAction *mIdentityAction, *mTransportAction, *mFccAction;
    KToggleAction *mWordWrapAction, *mFixedFontAction, *mAutoSpellCheckingAction;
    KToggleAction *mDictionaryAction, *mSnippetAction, *mTranslateAction;
    KAction *mAppendSignature;
    KAction *mPrependSignature;
    KAction *mInsertSignatureAtCursorPosition;
    KToggleAction *mGenerateShortenUrl;

    KToggleAction *markupAction;

    CodecAction *mCodecAction;
    KSelectAction *mCryptoModuleAction;

    KAction *mFindText, *mFindNextText, *mReplaceText, *mSelectAll;

    QSplitter *mHeadersToEditorSplitter;
    QWidget* mHeadersArea;
    QSplitter *mSplitter;
    QSplitter *mSnippetSplitter;
    QByteArray mOriginalPreferredCharset;

    MessageComposer::Composer *mDummyComposer;
    // used for auto saving, printing, etc. Not for sending, which happens in ComposerViewBase
    QList< MessageComposer::Composer* > mMiscComposers;

    int mLabelWidth;

    KActionMenuChangeCase *mChangeCaseMenu;

    QString mdbusObjectPath;
    static int s_composerNumber;

    MessageComposer::ComposerViewBase* mComposerBase;


    SnippetWidget *mSnippetWidget;
    PimCommon::CustomToolsWidget *mCustomToolsWidget;
    QPointer<KPIMTextEdit::SelectSpecialChar> mSelectSpecialChar;
    AttachmentMissingWarning *mAttachmentMissing;
    ExternalEditorWarning *mExternalEditorWarning;
    QTimer *m_verifyMissingAttachment;
    MailCommon::FolderRequester *mFccFolder;
    bool mPreventFccOverwrite;
    bool mCheckForForgottenAttachments;
    bool mIgnoreStickyFields;
    bool mWasModified;
    CryptoStateIndicatorWidget *mCryptoStateIndicatorWidget;
    KMStorageService *mStorageService;
    bool mSendNowByShortcutUsed;
    PimCommon::SplitterCollapser *mSnippetSplitterCollapser;
    KToggleAction *mFollowUpToggleAction;
    StatusBarLabelToggledState *mStatusBarLabelToggledOverrideMode;
    StatusBarLabelToggledState *mStatusBarLabelSpellCheckingChangeMode;
    PotentialPhishingEmailWarning *mPotentialPhishingEmailWarning;
    KMComposerGlobalAction *mGlobalAction;
};

#endif
