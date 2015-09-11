//

#ifndef KMReaderMainWin_h
#define KMReaderMainWin_h

#include "secondarywindow.h"

#include <QUrl>
#include <KMime/Message>

#include <AkonadiCore/item.h>
#include <AkonadiCore/collection.h>
#include <QModelIndex>
#include <viewer/viewer.h>
class KMReaderWin;
class QAction;
class KFontAction;
class KFontSizeAction;
class KJob;

namespace KMail
{
class MessageActions;
}

namespace KMime
{
class Message;
class Content;
}
Q_DECLARE_METATYPE(QModelIndex)

class KMReaderMainWin : public KMail::SecondaryWindow
{
    Q_OBJECT

public:
    KMReaderMainWin(MessageViewer::Viewer::DisplayFormatMessage format, bool htmlLoadExtOverride, char *name = Q_NULLPTR);
    explicit KMReaderMainWin(char *name = Q_NULLPTR);
    KMReaderMainWin(KMime::Content *aMsgPart, MessageViewer::Viewer::DisplayFormatMessage format, const QString &encoding, char *name = Q_NULLPTR);
    virtual ~KMReaderMainWin();

    void setUseFixedFont(bool useFixedFont);

    /**
    * take ownership of and show @param msg
    *
    * The last two parameters, serNumOfOriginalMessage and nodeIdOffset, are needed when @p msg
    * is derived from another message, e.g. the user views an encapsulated message in this window.
    * Then, the reader needs to know about that original message, so those to parameters are passed
    * onto setOriginalMsg() of KMReaderWin.
    */
    void showMessage(const QString &encoding, const Akonadi::Item &msg, const Akonadi::Collection &parentCollection = Akonadi::Collection());

    void showMessage(const QString &encoding, KMime::Message::Ptr message);
    void showMessagePopup(const Akonadi::Item &msg , const QUrl &aUrl, const QUrl &imageUrl, const QPoint &aPoint, bool contactAlreadyExists, bool uniqueContactFound);

private Q_SLOTS:
    void slotMessagePopup(const Akonadi::Item &, const QUrl &, const QUrl &imageUrl, const QPoint &);
    void slotContactSearchJobForMessagePopupDone(KJob *);
    void slotTrashMsg();
    void slotForwardInlineMsg();
    void slotForwardAttachedMsg();
    void slotRedirectMsg();
    void slotFontAction(const QString &);
    void slotSizeAction(int);
    void slotCustomReplyToMsg(const QString &tmpl);
    void slotCustomReplyAllToMsg(const QString &tmpl);
    void slotCustomForwardMsg(const QString &tmpl);

    void slotEditToolbars();
    void slotConfigChanged();
    void slotUpdateToolbars();

    /// This closes the window if the setting to close the window after replying or
    /// forwarding is set.
    void slotReplyOrForwardFinished();
    void slotCopyItem(QAction *);
    void slotCopyMoveResult(KJob *job);
    void slotMoveItem(QAction *action);

private:
    void copyOrMoveItem(const Akonadi::Collection &collection, bool move);
    Akonadi::Collection parentCollection() const;
    void initKMReaderMainWin();
    void setupAccel();
    QAction *copyActionMenu(QMenu *menu);
    QAction *moveActionMenu(QMenu *menu);
    void changeFont(const QFont &f);

    KMReaderWin *mReaderWin;
    Akonadi::Item mMsg;
    // a few actions duplicated from kmmainwidget
    QAction *mTrashAction, *mSaveAtmAction;
    KFontAction *mFontAction;
    KFontSizeAction *mFontSizeAction;
    KMail::MessageActions *mMsgActions;
    Akonadi::Collection mParentCollection;
};

#endif /*KMReaderMainWin_h*/
