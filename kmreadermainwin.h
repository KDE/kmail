// -*- mode: C++; c-file-style: "gnu" -*-

#ifndef KMReaderMainWin_h
#define KMReaderMainWin_h

#include "secondarywindow.h"

#include <kurl.h>
#include <KMime/Message>

#include <boost/scoped_ptr.hpp>
#include <AkonadiCore/item.h>
#include <AkonadiCore/collection.h>
#include <QModelIndex>
#include <messageviewer/viewer/viewer.h>
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
    KMReaderMainWin(MessageViewer::Viewer::DisplayFormatMessage format, bool htmlLoadExtOverride, char *name = 0);
    explicit KMReaderMainWin(char *name = 0);
    KMReaderMainWin(KMime::Content *aMsgPart, MessageViewer::Viewer::DisplayFormatMessage format, const QString &encoding, char *name = 0);
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
    void showMessagePopup(const Akonadi::Item &msg , const KUrl &aUrl, const KUrl &imageUrl, const QPoint &aPoint, bool contactAlreadyExists, bool uniqueContactFound);

private slots:
    void slotMessagePopup(const Akonadi::Item &, const KUrl &, const KUrl &imageUrl, const QPoint &);
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
    void slotCopyResult(KJob *job);

private:
    Akonadi::Collection parentCollection() const;
    void initKMReaderMainWin();
    void setupAccel();
    QAction *copyActionMenu(QMenu *menu);

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
