/* KMail Folder Selection Dialog
 * Pops up a small window with a list of folders and Ok/Cancel buttons.
 * Author: Stefan Taferner <taferner@kde.org>
 *         Carsten Burghardt <burghardt@kde.org>
 */
#ifndef kmfolderseldlg_h
#define kmfolderseldlg_h

#include <kdialogbase.h>
#include <kfoldertree.h>

class KMFolder;
class KMFolderTree;
class KMMainWidget;

namespace KMail {

  class SimpleFolderTree : public KFolderTree
  {
    Q_OBJECT

    public:
      SimpleFolderTree( QWidget * parent, KMFolderTree * folderTree,
                        const QString & preSelection, bool mustBeReadWrite );

      /** Reload the tree and select what folders to show and what not */
      void reload( bool mustBeReadWrite, bool showOutbox, bool showImapFolders,
                   const QString& preSelection = QString::null );

      /** Return the current folder */
      const KMFolder * folder() const;

      /** Set the current folder */
      void setFolder( KMFolder* );
      void setFolder( const QString& idString );

    public slots:
      void addChildFolder();

    protected slots:
      void slotContextMenuRequested( QListViewItem *, const QPoint & );

    private:
      KMFolderTree* mFolderTree;
      int mFolderColumn;
      bool mLastMustBeReadWrite;
      bool mLastShowOutbox;
      bool mLastShowImapFolders;
};

  //-----------------------------------------------------------------------------
  class KMFolderSelDlg: public KDialogBase
  {
    Q_OBJECT

    public:
      /** 
       * Constructor with KMMainWidget
       * @p parent @em must be a @see KMMainWin, because we
       *    need its foldertree.
       * @param mustBeReadWrite if true, readonly folders are disabled
       * @param useGlobalSettings if true, the current folder is read and 
       *        written to GlobalSettings
       */
      KMFolderSelDlg( KMMainWidget * parent, const QString& caption, 
          bool mustBeReadWrite, bool useGlobalSettings = true );
      /**
       * Constructor with separate KMFolderTree
       * @param mustBeReadWrite if true, readonly folders are disabled
       * @param useGlobalSettings if true, the current folder is read and 
       *        written to GlobalSettings
       */ 
      KMFolderSelDlg( QWidget * parent, KMFolderTree * tree,
          const QString& caption, bool mustBeReadWrite, 
          bool useGlobalSettings = true );

      virtual ~KMFolderSelDlg();

      /** Returns selected folder */
      virtual KMFolder* folder( void );

      /** Set selected folder */
      void setFolder( KMFolder* folder );

      /** Set some flags what folders to show and what not */
      void setFlags( bool mustBeReadWrite, bool showOutbox, bool showImapFolders );

    protected slots:
      void slotSelect();
      void slotUser1();
      void slotUpdateBtnStatus();

    protected:
      void readConfig();
      void writeConfig();
      /** Init the dialog */
      void init();

      SimpleFolderTree * mTreeView;
      bool mUseGlobalSettings;
  };

} // namespace KMail

#endif /*kmfolderseldlg_h*/
