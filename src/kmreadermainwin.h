//

#ifndef KMReaderMainWin_h
#define KMReaderMainWin_h

#include "kmail_export.h"

#include "secondarywindow.h"

#include <QUrl>
#include <KMime/Message>

#include <AkonadiCore/item.h>
#include <AkonadiCore/collection.h>
#include <QModelIndex>
#include <messageviewer/viewer.h>
class KMReaderWin;
class QAction;
class KJob;
class ZoomLabelWidget;

namespace KMail {
class MessageActions;
class TagActionManager;
}

namespace KMime {
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

    MessageViewer::Viewer *viewer() const;
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

    void showMessage(const QString &encoding, const KMime::Message::Ptr &message);
    void showMessagePopup(const Akonadi::Item &msg, const QUrl &aUrl, const QUrl &imageUrl, const QPoint &aPoint, bool contactAlreadyExists, bool uniqueContactFound,
                          const WebEngineViewer::WebHitTestResult &result);
public Q_SLOTS:
    void slotForwardInlineMsg();
    void slotForwardAttachedMessage();
    void slotRedirectMessage();
    void slotCustomReplyToMsg(const QString &tmpl);
    void slotCustomReplyAllToMsg(const QString &tmpl);
    void slotCustomForwardMsg(const QString &tmpl);

private:
    void slotMessagePopup(const Akonadi::Item &aMsg, const WebEngineViewer::WebHitTestResult &result, const QPoint &aPoint);
    void slotContactSearchJobForMessagePopupDone(KJob *);
    void slotTrashMessage();

    void slotEditToolbars();
    void slotConfigChanged();
    void slotUpdateToolbars();

    /// This closes the window if the setting to close the window after replying or
    /// forwarding is set.
    void slotReplyOrForwardFinished();
    void slotCopyItem(QAction *);
    void slotCopyMoveResult(KJob *job);
    void slotMoveItem(QAction *action);

    void slotShowMessageStatusBar(const QString &msg);

    void copyOrMoveItem(const Akonadi::Collection &collection, bool move);
    Akonadi::Collection parentCollection() const;
    void initKMReaderMainWin();
    void setupAccel();
    QAction *copyActionMenu(QMenu *menu);
    QAction *moveActionMenu(QMenu *menu);
    void setZoomChanged(qreal zoomFactor);
    void updateActions();
    void slotSelectMoreMessageTagList();
    void toggleMessageSetTag(const Akonadi::Item::List &select, const Akonadi::Tag &tag);
    void slotUpdateMessageTagList(const Akonadi::Tag &tag);

    Akonadi::Collection mParentCollection;
    Akonadi::Item mMsg;
    // a few actions duplicated from kmmainwidget
    QAction *mTrashAction = nullptr;
    QAction *mSaveAtmAction = nullptr;
    KMail::MessageActions *mMsgActions = nullptr;
    KMReaderWin *mReaderWin = nullptr;
    ZoomLabelWidget *mZoomLabelIndicator = nullptr;
    KMail::TagActionManager *mTagActionManager = nullptr;
};

#endif /*KMReaderMainWin_h*/
