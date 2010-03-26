/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>

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

#ifndef KMREADERWIN_H
#define KMREADERWIN_H

#include <QWidget>
#include <QTimer>
#include <QStringList>
#include <QCloseEvent>
#include <QEvent>
#include <QList>
#include <QResizeEvent>
#include <kurl.h>
#include <kservice.h>
#include <messagecore/messagestatus.h>
#include <kvbox.h>
using KPIM::MessageStatus;
#include "interfaces/observer.h"
#include <map>
#include <messageviewer/viewer.h>
#include <messageviewer/interfaces/bodypart.h>
class QString;


class KActionCollection;
class KAction;
class KToggleAction;
class KToggleAction;
class KWebView;
class KUrl;
namespace MessageViewer {
  namespace Interface {
    class BodyPartMemento;
  }
  class HeaderStrategy;
  class HeaderStyle;
  class Viewer;
  class CSSHelper;
  class AttachmentStrategy;
}

namespace KParts {
  struct BrowserArguments;
}

namespace Akonadi {
  class Item;
}

/**
   This class implements a "reader window", that is a window
   used for reading or viewing messages.
*/

class KMReaderWin: public QWidget {
  Q_OBJECT

public:
  KMReaderWin( QWidget *parent, QWidget *mainWindow,
               KActionCollection *actionCollection, Qt::WindowFlags f = 0 );
  virtual ~KMReaderWin();

  /** Read settings from app's config file. */
  void readConfig();

  const MessageViewer::HeaderStyle * headerStyle() const;

  /** Set the header style and strategy. We only want them to be set
      together. */
  void setHeaderStyleAndStrategy( const MessageViewer::HeaderStyle * style,
                                  const MessageViewer::HeaderStrategy * strategy );
  /** Getthe message header strategy. */
  const MessageViewer::HeaderStrategy * headerStrategy() const;

  /** Get/set the message attachment strategy. */
  const MessageViewer::AttachmentStrategy * attachmentStrategy() const;

  void setAttachmentStrategy( const MessageViewer::AttachmentStrategy * strategy );

  /** Get selected override character encoding.
      @return The encoding selected by the user or an empty string if auto-detection
      is selected. */
  QString overrideEncoding() const;
  /** Set the override character encoding. */
  void setOverrideEncoding( const QString & encoding );
  virtual void setPrinting(bool enable );

  void setMessage( const Akonadi::Item& item, MessageViewer::Viewer::UpdateMode updateMode = MessageViewer::Viewer::Delayed);


  /** Instead of settings a message to be shown sets a message part
      to be shown */
  void setMsgPart( KMime::Content* aMsgPart, bool aHTML,
                   const QString& aFileName, const QString& pname );


  /** Store message id of last viewed message,
      normally no need to call this function directly,
      since correct value is set automatically in
      parseMsg(KMMessage* aMsg, bool onlyProcessHeaders). */
  void setIdOfLastViewedMessage( const QString & msgId )
    { mIdOfLastViewedMessage = msgId; }

  /** Clear the reader and discard the current message. */
  void clear(bool force = false);

  void update(bool force = false);

  /** Saves the relative position of the scroll view. Call this before calling update()
      if you want to preserve the current view. */
  void saveRelativePosition();

  /** Return selected text */
  QString copyText() const;

  /** Override default html mail setting */
  bool htmlOverride() const;
  void setHtmlOverride( bool override );

  /** Override default load external references setting */
  bool htmlLoadExtOverride() const;
  void setHtmlLoadExtOverride( bool override );

  /** Is html mail to be supported? Takes into account override */
  bool htmlMail();

  /** Is loading ext. references to be supported? Takes into account override */
  bool htmlLoadExternal();

  /** Returns the MD5 hash for the list of new features */
  static QString newFeaturesMD5();

  /** Display a generic HTML splash page instead of a message */
  void displaySplashPage( const QString &info );

  /** Display the about page instead of a message */
  void displayAboutPage();

  /** Display the 'please wait' page instead of a message */
  void displayBusyPage();
  /** Display the 'we are currently in offline mode' page instead of a message */
  void displayOfflinePage();

  bool isFixedFont() const;
  void setUseFixedFont( bool useFixedFont );
  MessageViewer::Viewer *viewer() { return mViewer; }
  // Action to reply to a message
  // but action( "some_name" ) some name could be used instead.
  KToggleAction *toggleFixFontAction();
  KAction *mailToComposeAction() { return mMailToComposeAction; }
  KAction *mailToReplyAction() { return mMailToReplyAction; }
  KAction *mailToForwardAction() { return mMailToForwardAction; }
  KAction *addAddrBookAction() { return mAddAddrBookAction; }
  KAction *openAddrBookAction() { return mOpenAddrBookAction; }
  KAction *copyAction();
  KAction *selectAllAction();
  KAction *copyURLAction();
  KAction *urlOpenAction();
  KAction *urlSaveAsAction() { return mUrlSaveAsAction; }
  KAction *addBookmarksAction() { return mAddBookmarksAction;}
  KAction *toggleMimePartTreeAction();

  void setUpdateAttachment( bool update = true ) { mAtmUpdate = update; }
  /** Access to the KWebKitView used for the viewer. Use with
      care! */
  KWebView * htmlPart() const;

  Akonadi::Item message() const;
  /** Set the serial number of the message this reader window is currently
   *  waiting for. Used to discard updates for already deselected messages. */
  void setWaitingForSerNum( unsigned long serNum ) { mWaitingForSerNum = serNum; }

  QWidget* mainWindow() { return mMainWindow; }

  /** Returns whether the message should be decryted. */
  bool decryptMessage() const;

  /** Enforce message decryption. */
  void setDecryptMessageOverwrite( bool overwrite = true );

  /** Show signature details. */
  bool showSignatureDetails() const;

  /** Show signature details. */
  void setShowSignatureDetails( bool showDetails = true );

  MessageViewer::CSSHelper* cssHelper() const;

  /* show or hide the list that points to the attachments */
  bool showAttachmentQuicklist() const;

  /* show or hide the list that points to the attachments */
  void setShowAttachmentQuicklist( bool showAttachmentQuicklist = true );
  /** Return weather to show or hide the full list of "To" addresses */
  bool showFullToAddressList() const;

  /** Show or hide the full list of "To" addresses */
  void setShowFullToAddressList( bool showFullToAddressList = true );

  /** Return weather to show or hide the full list of "To" addresses */
  bool showFullCcAddressList() const;

  /** Show or hide the full list of "To" addresses */
  void setShowFullCcAddressList( bool showFullCcAddressList = true );

private:

signals:
  /** Emitted after parsing of a message to have it stored
      in unencrypted state in it's folder. */
  void replaceMsgByUnencryptedVersion();

  /** Pgp displays a password dialog */
  void noDrag(void);

public slots:
  /** Force update even if message is the same */
  void clearCache();

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
  void slotTouchMessage();
  void slotUrlClicked( const Akonadi::Item &,  const KUrl& );
  void slotRequestConfigSync();
  void slotShowReader( KMime::Content* , bool, const QString&, const QString&, const QString &);
protected:

  KUrl urlClicked() const;
private:
  void createActions();
private:
  Akonadi::Item messageItem() const;

  // See setOriginalMsg() for an explaination for those two.
  QTimer mDelayedMarkTimer;
  bool mNoMDNsWhenEncrypted;
  unsigned long mLastSerNum;
  QStringList mTempFiles;
  QStringList mTempDirs;
  QString mIdOfLastViewedMessage;
  QWidget *mMainWindow;
  KActionCollection *mActionCollection;

  KAction *mMailToComposeAction, *mMailToReplyAction, *mMailToForwardAction,
    *mAddAddrBookAction, *mOpenAddrBookAction, *mUrlSaveAsAction, *mAddBookmarksAction, *mSelectAllAction;

  MessageViewer::Viewer *mViewer;

  /** Used only to be able to connect and disconnect finished() signal
      in printMsg() and slotPrintMsg() since mHtmlWriter points only to abstract non-QObject class. */
  std::map<QByteArray, MessageViewer::Interface::BodyPartMemento*> mBodyPartMementoMap;
  // an attachment should be updated
  bool mAtmUpdate;
  int mChoice;
  unsigned long mWaitingForSerNum;

  bool mShowFullToAddressList;
  bool mShowFullCcAddressList;
};


#endif

