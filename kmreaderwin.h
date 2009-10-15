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

#include "kmail-akonadi.h"
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
#include "kmmimeparttree.h" // Needed for friend declaration.
#include "interfaces/observer.h"
#include <map>
#include <messageviewer/viewer.h>
class QSplitter;
class KHBox;
class QTreeWidgetItem;
class QString;
class QTextCodec;


class KActionCollection;
class KAction;
class KSelectAction;
class KToggleAction;
class KToggleAction;
class KHTMLPart;
class KUrl;
class KMFolder;
class KMMessage;
class KMMessagePart;
//Remove it when we remove old code
class HtmlStatusBar;
namespace KMail {
  namespace Interface {
    class Observable;
    class BodyPartMemento;
  }
  class ObjectTreeParser;
}
namespace MessageViewer {
  class HeaderStrategy;
  class HeaderStyle;
}
class KHtmlPartHtmlWriter;
class partNode; // might be removed when KMime is used instead of mimelib
                //                                      (khz, 29.11.2001)

namespace KParts {
  struct BrowserArguments;
  class OpenUrlArguments;
}

namespace DOM {
  class HTMLElement;
}

namespace MessageViewer {
   class Viewer;
}

namespace Akonadi {
  class Item;
}

namespace MessageViewer {
  class CSSHelper;
  class AttachmentStrategy;
}

/**
   This class implements a "reader window", that is a window
   used for reading or viewing messages.
*/

class KMReaderWin: public QWidget, public KMail::Interface::Observer {
  Q_OBJECT

  friend void KMMimePartTree::slotItemClicked( QTreeWidgetItem* );
  friend void KMMimePartTree::slotContextMenuRequested( const QPoint & );
  friend void KMMimePartTree::slotSaveAs();
  friend void KMMimePartTree::startDrag( Qt::DropActions actions );

  friend class KMail::ObjectTreeParser;
  friend class KHtmlPartHtmlWriter;

public:
  KMReaderWin( QWidget *parent, QWidget *mainWindow,
               KActionCollection *actionCollection, Qt::WindowFlags f = 0 );
  virtual ~KMReaderWin();

  /**
     \reimp from Interface::Observer
     Updates the current message
   */
  void update( KMail::Interface::Observable * );

  /** Read settings from app's config file. */
  void readConfig();

#ifndef USE_AKONADI_VIEWER
  /** Write settings to app's config file. Calls sync() if withSync is true. */
  void writeConfig( bool withSync=true ) const;
#endif

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
#ifndef USE_AKONADI_VIEWER
  /** Get codec corresponding to the currently selected override character encoding.
      @return The override codec or 0 if auto-detection is selected. */
  const QTextCodec * overrideCodec() const;
  /** Set printing mode */
#endif
  virtual void setPrinting(bool enable );

  /** Set the message that shall be shown. If msg is 0, an empty page is
      displayed. */
  virtual void setMsg( KMMessage* msg, bool force = false );

  void setMessage( Akonadi::Item item, MessageViewer::Viewer::UpdateMode updateMode = MessageViewer::Viewer::Delayed);

  /**
   * This should be called when setting a message that was constructed from another message, which
   * is the case when viewing encapsulated messages in the separate reader window.
   * We need to know the serial number of the original message, and at which part index the encapsulated
   * message was at that original message, so that deleting and editing attachments can work on the
   * original message.
   *
   * This is a HACK. There really shouldn't be a copy of the original mail.
   *
   * @see slotDeleteAttachment, slotEditAttachment, fillCommandInfo
   */
  void setOriginalMsg( unsigned long serNumOfOriginalMessage, int nodeIdOffset );

  /** Instead of settings a message to be shown sets a message part
      to be shown */
  void setMsgPart( KMMessagePart* aMsgPart, bool aHTML,
                   const QString& aFileName, const QString& pname );

  void setMsgPart( partNode * node );


  /** Store message id of last viewed message,
      normally no need to call this function directly,
      since correct value is set automatically in
      parseMsg(KMMessage* aMsg, bool onlyProcessHeaders). */
  void setIdOfLastViewedMessage( const QString & msgId )
    { mIdOfLastViewedMessage = msgId; }

  /** Clear the reader and discard the current message. */
  void clear(bool force = false);

  /** Saves the relative position of the scroll view. Call this before calling update()
      if you want to preserve the current view. */
  void saveRelativePosition();

  /** Re-parse the current message. */
  void update(bool force = false);
#ifndef USE_AKONADI_VIEWER
  /** Print message. */
  virtual void printMsg(  KMMessage* aMsg );
#endif
  /** Return selected text */
  QString copyText();

  /** Get/set auto-delete msg flag. */
  bool autoDelete(void) const;
  void setAutoDelete(bool f);

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
  /** Returns message part from given URL or null if invalid. */
  partNode* partNodeFromUrl(const KUrl &url);

  partNode * partNodeForId( int id );

  KUrl tempFileUrlFromPartNode( const partNode *node );

  /** Returns id of message part from given URL or -1 if invalid. */
  static int msgPartFromUrl(const KUrl &url);

  void setUpdateAttachment( bool update = true ) { mAtmUpdate = update; }
  /** Access to the KHTMLPart used for the viewer. Use with
      care! */
  KHTMLPart * htmlPart() const;

  /** Returns the current message or 0 if none. */
  KMMessage* message(KMFolder** folder=0) const;
#ifndef USE_AKONADI_VIEWER

  void openAttachment( int id, const QString & name );
  void saveAttachment( const KUrl &tempFileName );
#endif
  //need for urlhandlermanager.cpp
  void emitUrlClicked( const KUrl & url, int button ) {
    emit urlClicked( url, button );
  }

  void emitPopupMenu( const KUrl & url, const QPoint & p ) {
    if ( message() )
      emit popupMenu( *message(), url, p );
  }
#ifndef USE_AKONADI_VIEWER
  /**
   * Sets the current attachment ID and the current attachment temporary filename
   * to the given values.
   * Call this so that slotHandleAttachment() knows which attachment to handle.
   */
  void prepareHandleAttachment( int id, const QString& fileName );

  void showAttachmentPopup( int id, const QString & name, const QPoint & p );
#endif
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

  /* retrieve BodyPartMemento of id \a which for partNode \a node */
  KMail::Interface::BodyPartMemento * bodyPartMemento( const partNode * node, const QByteArray & which ) const;

  /* set/replace BodyPartMemento \a memento of id \a which for
     partNode \a node. If there was a BodyPartMemento registered
     already, replaces (deletes) that one. */
  void setBodyPartMemento( const partNode * node, const QByteArray & which, KMail::Interface::BodyPartMemento * memento );
private:
  /* deletes all BodyPartMementos. Use this when skipping to another
     message (as opposed to re-loading the same one again). */
  void clearBodyPartMementos();

signals:
  /** Emitted after parsing of a message to have it stored
      in unencrypted state in it's folder. */
  void replaceMsgByUnencryptedVersion();

  /** The user presses the right mouse button. 'url' may be 0. */
  void popupMenu(KMMessage &msg, const KUrl &url, const QPoint& mousePos);

  /** The user has clicked onto an URL that is no attachment. */
  void urlClicked(const KUrl &url, int button);

  /** Pgp displays a password dialog */
  void noDrag(void);

public slots:
  /** Force update even if message is the same */
  void clearCache();

  /** Refresh the reader window */
  void updateReaderWin();
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
  /** Copy URL in mUrlCurrent to clipboard. Removes "mailto:" at
      beginning of URL before copying. */
  void slotUrlOpen( const KUrl &url = KUrl() );
  /** Save the page to a file */
  void slotUrlSave();
  void slotAddBookmarks();
  void slotMessageArrived( KMMessage *msg );
  void slotTouchMessage();

  /**
   * Find the node ID and the message of the attachment that should be edited or deleted.
   * This is used when setOriginalMsg() was called before, in that case we want to operate
   * on the original message instead of our copy.
   *
   * @see setOriginalMsg
   */
  void fillCommandInfo( partNode *node, KMMessage **msg, int *nodeId );

  void slotDeleteAttachment( partNode* node );
  void slotEditAttachment( partNode* node );
#ifndef USE_AKONADI_VIEWER
  /**
   * Does an action for the current attachment.
   * The action is defined by the KMHandleAttachmentCommand::AttachmentAction
   * enum.
   * prepareHandleAttachment() needs to be called before calling this to set the
   * correct attachment ID.
   */
  void slotHandleAttachment( int action );
#endif
protected slots:
  /** Some attachment operations. */
  void slotAtmView( int id, const QString& name );
#ifndef USE_AKONADI_VIEWER
  void slotDelayedResize();
  /** Print message. Called on as a response of finished() signal of mPartHtmlWriter
      after rendering is finished.
      In the very end it deletes the KMReaderWin window that was created
      for the purpose of rendering. */
  void slotPrintMsg();
#endif
protected:
  /** reimplemented in order to update the frame width in case of a changed
      GUI style */
  /** Writes the given message part to a temporary file and returns the
      name of this file or QString() if writing failed.
  */
  QString writeMessagePartToTempFile( KMMessagePart* msgPart, int partNumber );

  /**
    Creates a temporary dir for saving attachments, etc.
    Will be automatically deleted when another message is viewed.
    @param param Optional part of the directory name.
  */
  QString createTempDir( const QString &param = QString() );
  /** Cleanup the attachment temp files */
  virtual void removeTempFiles();


  KUrl urlClicked() const;
private:
  void createActions();
private:
  int mAtmCurrent;
  QString mAtmCurrentName;
  KMMessage *mMessage;

  // See setOriginalMsg() for an explaination for those two.
  unsigned long mSerNumOfOriginalMessage;
  int mNodeIdOffset;

  QTimer mDelayedMarkTimer;
  bool mNoMDNsWhenEncrypted;
  unsigned long mLastSerNum;
  QStringList mTempFiles;
  QStringList mTempDirs;
  partNode* mRootNode;
  QString mIdOfLastViewedMessage;
  QWidget *mMainWindow;
  KActionCollection *mActionCollection;

  KAction *mMailToComposeAction, *mMailToReplyAction, *mMailToForwardAction,
    *mAddAddrBookAction, *mOpenAddrBookAction, *mUrlSaveAsAction, *mAddBookmarksAction, *mSelectAllAction;

  MessageViewer::Viewer *mViewer;

  /** Used only to be able to connect and disconnect finished() signal
      in printMsg() and slotPrintMsg() since mHtmlWriter points only to abstract non-QObject class. */
  std::map<QByteArray,KMail::Interface::BodyPartMemento*> mBodyPartMementoMap;
  // an attachment should be updated
  bool mAtmUpdate;
  int mChoice;
  unsigned long mWaitingForSerNum;

  bool mShowFullToAddressList;
  bool mShowFullCcAddressList;
};


#endif

