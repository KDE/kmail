/**
 * Copyright (c) 2004 Carsten Burghardt <burghardt@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#ifndef folderrequester_h
#define folderrequester_h

#include <qwidget.h>
#include <klineedit.h>

class KMFolder;
class KMFolderTree;

namespace KMail {

  /**
   * A widget that contains a KLineEdit which shows the current folder
   * and a button that fires a KMFolderSelDlg
   * The dialog is set to disable readonly folders by default
   * Search folders are excluded
   */ 
  class FolderRequester: public QWidget
  {
    Q_OBJECT

    public:
      /** 
       * Constructor
       * @param parent the parent widget
       * @param tree the KMFolderTree to get the folders
       */
      FolderRequester( QWidget *parent, KMFolderTree* tree );
      virtual ~FolderRequester();

      /** Returns selected folder */
      KMFolder* folder( void ) const;

      /** Returns current text */
      QString text() const { return edit->originalText(); } 

      /** Preset the folder */
      void setFolder( KMFolder* );
      void setFolder( const QString& idString );

      /** 
       * Set if readonly folders should be disabled
       * Be aware that if you disable this the user can also select the 
       * 'Local Folders' folder which has no valid folder associated
       */
      void setMustBeReadWrite( bool readwrite ) 
      { mMustBeReadWrite = readwrite; }

      /** Set if the outbox should be shown */
      void setShowOutbox( bool show )
      { mShowOutbox = show; }

      /** Set if the imap folders should be shown */
      void setShowImapFolders( bool show )
      { mShowImapFolders = show; }

    protected slots:
      /** Open the folder dialog */
      void slotOpenDialog();

    signals:
      /** Emitted when the folder changed */
      void folderChanged( KMFolder* );

    protected:
      KLineEdit* edit;
      KMFolder* mFolder;
      KMFolderTree* mFolderTree;
      bool mMustBeReadWrite;
      bool mShowOutbox;
      bool mShowImapFolders;
  };

} // namespace KMail

#endif /*folderrequester_h*/
