/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>
  Copyright (c) 2013 Laurent Montel <montel@kde.org>

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
#include <kurl.h>
#include <messageviewer/viewer/viewer.h>
#include <messageviewer/interfaces/bodypart.h>
#include <Akonadi/Item>
#include <KABC/Addressee>
class KActionCollection;
class KAction;
class KToggleAction;
class KMenu;
namespace MessageViewer {
  class HeaderStrategy;
  class HeaderStyle;
  class Viewer;
  class CSSHelper;
  class AttachmentStrategy;
}


class KJob;

/**
   This class implements a "reader window", that is a window
   used for reading or viewing messages.
*/

class KMReaderWin: public QWidget {
  Q_OBJECT

public:
  explicit KMReaderWin( QWidget *parent, QWidget *mainWindow,
               KActionCollection *actionCollection, Qt::WindowFlags f = 0 );
  virtual ~KMReaderWin();

  /** Read settings from app's config file. */
  void readConfig();

  MessageViewer::HeaderStyle * headerStyle() const;

  /** Set the header style and strategy. We only want them to be set
      together. */
  void setHeaderStyleAndStrategy( MessageViewer::HeaderStyle * style,
                                  MessageViewer::HeaderStrategy * strategy );
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

  void setMessage( KMime::Message::Ptr message );

  /** Instead of settings a message to be shown sets a message part
      to be shown */
  void setMsgPart( KMime::Content* aMsgPart );

  /** Clear the reader and discard the current message. */
  void clear(bool force = false);

  void update(bool force = false);

  /** Return selected text */
  QString copyText() const;

  /** Override default html mail setting */
  bool htmlOverride() const;
  void setHtmlOverride( bool override );

  /** Override default load external references setting */
  bool htmlLoadExtOverride() const;
  void setHtmlLoadExtOverride( bool override );

  /** Is html mail to be supported? Takes into account override */
  bool htmlMail() const;

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

  void displayResourceOfflinePage();

  bool isFixedFont() const;
  void setUseFixedFont( bool useFixedFont );
  MessageViewer::Viewer *viewer() { return mViewer; }
  KToggleAction *toggleFixFontAction();
  KAction *mailToComposeAction() { return mMailToComposeAction; }
  KAction *mailToReplyAction() { return mMailToReplyAction; }
  KAction *mailToForwardAction() { return mMailToForwardAction; }
  KAction *addAddrBookAction() { return mAddAddrBookAction; }
  KAction *openAddrBookAction() { return mOpenAddrBookAction; }
  KAction *copyAction();
  KAction *selectAllAction();
  KAction *copyURLAction();
  KAction *copyImageLocation();
  KAction *urlOpenAction();
  KAction *urlSaveAsAction() { return mUrlSaveAsAction; }
  KAction *addBookmarksAction() { return mAddBookmarksAction;}
  KAction *toggleMimePartTreeAction();
  KAction *speakTextAction();
  KAction* translateAction();
  KAction* downloadImageToDiskAction() const;
  KAction *viewSourceAction();
  KAction *findInMessageAction();
  KAction *saveAsAction();
  KAction *saveMessageDisplayFormatAction();
  KAction *resetMessageDisplayFormatAction();
  KAction *blockImage();
  KAction *openBlockableItems();

  KAction *editContactAction() const { return mEditContactAction; }

  KMenu *viewHtmlOption() const { return mViewHtmlOptions; }
  KAction *shareImage() const { return mShareImage; }

  Akonadi::Item message() const;

  QWidget* mainWindow() { return mMainWindow; }

  /** Enforce message decryption. */
  void setDecryptMessageOverwrite( bool overwrite = true );

  MessageViewer::CSSHelper* cssHelper() const;

  bool printSelectedText(bool preview);

  void setContactItem(const Akonadi::Item& contact, const KABC::Addressee &address);
  void clearContactItem();
  bool adblockEnabled() const;

signals:
  /** Emitted after parsing of a message to have it stored
      in unencrypted state in it's folder. */
  void replaceMsgByUnencryptedVersion();

  void showStatusBarMessage( const QString &message );

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
  void slotUrlClicked( const Akonadi::Item &,  const KUrl& );
  void slotShowReader( KMime::Content* , bool, const QString& );
  void slotShowMessage( KMime::Message::Ptr message, const QString& encoding );
  void slotDeleteMessage( const Akonadi::Item& );
  void slotSaveImageOnDisk();

  void slotPrintComposeResult( KJob *job );
  void slotEditContact();
  void contactStored( const Akonadi::Item &item );
  void slotContactEditorError(const QString &error);

  void slotContactHtmlOptions();
  void slotShareImage();

protected:
  KUrl urlClicked() const;
  KUrl imageUrlClicked() const;

private:
  void createActions();
  void updateHtmlActions();

private:
  KABC::Addressee mSearchedAddress;
  Akonadi::Item mSearchedContact;
  QWidget *mMainWindow;
  KActionCollection *mActionCollection;

  KAction *mMailToComposeAction;
  KAction *mMailToReplyAction;
  KAction *mMailToForwardAction;
  KAction *mAddAddrBookAction;
  KAction *mOpenAddrBookAction;
  KAction *mUrlSaveAsAction;
  KAction *mAddBookmarksAction;
  KAction *mImageUrlSaveAsAction;
  KAction *mEditContactAction;
  KAction *mViewAsHtml;
  KAction *mLoadExternalReference;
  KAction *mShareImage;

  KMenu *mViewHtmlOptions;

  MessageViewer::Viewer *mViewer;

};


#endif

