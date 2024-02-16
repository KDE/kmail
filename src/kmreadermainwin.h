//

#pragma once

#include "kmail_export.h"

#include "secondarywindow.h"

#include <KMime/Message>
#include <QUrl>

#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <MessageViewer/Viewer>
#include <QModelIndex>
class KMReaderWin;
class QAction;
class KJob;
class ZoomLabelWidget;

namespace KMail
{
class MessageActions;
class TagActionManager;
}
namespace Akonadi
{
class StandardMailActionManager;
}
namespace KMime
{
class Message;
class Content;
}
Q_DECLARE_METATYPE(QModelIndex)

class KMAIL_EXPORT KMReaderMainWin : public KMail::SecondaryWindow
{
    Q_OBJECT

public:
    KMReaderMainWin(MessageViewer::Viewer::DisplayFormatMessage format, bool htmlLoadExtDefault, const QString &name = QString());
    explicit KMReaderMainWin(const QString &name = QString());
    KMReaderMainWin(KMime::Content *aMsgPart, MessageViewer::Viewer::DisplayFormatMessage format, const QString &encoding, const QString &name = QString());
    ~KMReaderMainWin() override;

    void setUseFixedFont(bool useFixedFont);

    [[nodiscard]] MessageViewer::Viewer *viewer() const;
    /**
     * take ownership of and show @p msg
     *
     * The last two parameters, serNumOfOriginalMessage and nodeIdOffset, are needed when @p msg
     * is derived from another message, e.g. the user views an encapsulated message in this window.
     * Then, the reader needs to know about that original message, so those to parameters are passed
     * onto setOriginalMsg() of KMReaderWin.
     *
     * @param encoding The message encoding.
     * @param msg The message.
     * @param parentCollection An Akonadi parent collection.
     */
    void showMessage(const QString &encoding, const Akonadi::Item &msg, const Akonadi::Collection &parentCollection = Akonadi::Collection());

    void showMessage(const QString &encoding, const QList<KMime::Message::Ptr> &message);
    void showMessage(const QString &encoding, const KMime::Message::Ptr &message);
    void showMessagePopup(const Akonadi::Item &msg,
                          const QUrl &aUrl,
                          const QUrl &imageUrl,
                          const QPoint &aPoint,
                          bool contactAlreadyExists,
                          bool uniqueContactFound,
                          const WebEngineViewer::WebHitTestResult &result);
    void showAndActivateWindow();
public Q_SLOTS:
    void slotForwardInlineMsg();
    void slotForwardAttachedMessage();
    void slotRedirectMessage();
    void slotNewMessageToRecipients();
    void slotCustomReplyToMsg(const QString &tmpl);
    void slotCustomReplyAllToMsg(const QString &tmpl);
    void slotCustomForwardMsg(const QString &tmpl);

private:
    KMAIL_NO_EXPORT void slotMessagePopup(const Akonadi::Item &aMsg, const WebEngineViewer::WebHitTestResult &result, const QPoint &aPoint);
    KMAIL_NO_EXPORT void slotContactSearchJobForMessagePopupDone(KJob *);
    KMAIL_NO_EXPORT void slotTrashMessage();

    KMAIL_NO_EXPORT void slotEditToolbars();
    KMAIL_NO_EXPORT void slotConfigChanged();
    KMAIL_NO_EXPORT void slotUpdateToolbars();

    /// This closes the window if the setting to close the window after replying or
    /// forwarding is set.
    KMAIL_NO_EXPORT void slotReplyOrForwardFinished();
    KMAIL_NO_EXPORT void slotCopyItem(QAction *);
    KMAIL_NO_EXPORT void slotCopyMoveResult(KJob *job);
    KMAIL_NO_EXPORT void slotMoveItem(QAction *action);

    KMAIL_NO_EXPORT void slotShowMessageStatusBar(const QString &msg);

    KMAIL_NO_EXPORT void copyOrMoveItem(const Akonadi::Collection &collection, bool move);
    [[nodiscard]] KMAIL_NO_EXPORT Akonadi::Collection parentCollection() const;
    KMAIL_NO_EXPORT void initKMReaderMainWin();
    KMAIL_NO_EXPORT void setupAccel();
    KMAIL_NO_EXPORT QAction *copyActionMenu(QMenu *menu);
    KMAIL_NO_EXPORT QAction *moveActionMenu(QMenu *menu);
    KMAIL_NO_EXPORT void setZoomChanged(qreal zoomFactor);
    KMAIL_NO_EXPORT void updateActions();
    KMAIL_NO_EXPORT void slotSelectMoreMessageTagList();
    KMAIL_NO_EXPORT void toggleMessageSetTag(const Akonadi::Item::List &select, const Akonadi::Tag &tag);
    KMAIL_NO_EXPORT void slotUpdateMessageTagList(const Akonadi::Tag &tag);
    KMAIL_NO_EXPORT void initializeMessage(const KMime::Message::Ptr &message);
    KMAIL_NO_EXPORT void showNextMessage();
    KMAIL_NO_EXPORT void showPreviousMessage();
    KMAIL_NO_EXPORT void updateButtons();
    KMAIL_NO_EXPORT void slotToggleMenubar(bool dontShowWarning);
    KMAIL_NO_EXPORT void initializeAkonadiStandardAction();
    KMAIL_NO_EXPORT void slotMarkMailAs();

    QList<KMime::Message::Ptr> mListMessage;
    int mCurrentMessageIndex = 0;
    Akonadi::Collection mParentCollection;
    Akonadi::Item mMsg;
    // a few actions duplicated from kmmainwidget
    QAction *mTrashAction = nullptr;
    QAction *mSaveAtmAction = nullptr;
    KMail::MessageActions *mMsgActions = nullptr;
    KMReaderWin *mReaderWin = nullptr;
    ZoomLabelWidget *mZoomLabelIndicator = nullptr;
    KMail::TagActionManager *mTagActionManager = nullptr;
    KToggleAction *mHideMenuBarAction = nullptr;
    Akonadi::StandardMailActionManager *mAkonadiStandardActionManager = nullptr;
};
