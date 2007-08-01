/* KMail Folder Selection Dialog
 * Pops up a small window with a list of folders and Ok/Cancel buttons.
 * Author: Stefan Taferner <taferner@kde.org>
 */
#ifndef kmfolderseldlg_h
#define kmfolderseldlg_h

#include <kdialogbase.h>
#include <simplefoldertree.h>
#include <qvaluelist.h>
#include <qguardedptr.h>

class KMFolder;
class KMFolderTree;
class KMMainWidget;

class KMFolderSelDlg: public KDialogBase
{
  Q_OBJECT

public:
  /** Constructor. @p parent @em must be a @ref KMMainWin, because we
      need its foldertree.
   * @param mustBeReadWrite if true, readonly folders are disabled
   */
  KMFolderSelDlg( KMMainWidget * parent, const QString& caption, bool mustBeReadWrite );
  virtual ~KMFolderSelDlg();

  /** Returns selected folder */
  virtual KMFolder* folder( void );

protected slots:
  void slotSelect();

protected:
  KMail::SimpleFolderTree * mTreeView;
};

#endif /*kmfolderseldlg_h*/
