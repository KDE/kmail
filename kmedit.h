/* -*- mode: C++; c-file-style: "gnu" -*-
 * KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMAIL_KMEDIT_H__
#define __KMAIL_KMEDIT_H__

#include <kdeversion.h>
#include <keditcl.h>
#include <qmap.h>
#include <qstringlist.h>
#include <qclipboard.h>

class KMComposeWin;
class KSpellConfig;
class KSpell;
class SpellingFilter;
class KTempFile;
class KDictSpellingHighlighter;
class KDirWatch;
class KProcess;
class QPopupMenu;


class KMEdit : public KEdit {
  Q_OBJECT
public:
  KMEdit(QWidget *parent=0,KMComposeWin* composer=0,
         KSpellConfig* spellConfig = 0,
	 const char *name=0);
  ~KMEdit();

  /**
   * Start the spell checker.
   */
  void spellcheck();

  /**
   * Text with lines breaks inserted after every row
   */
  QString brokenText();

   /**
   * Toggle automatic spellchecking
   */
  int autoSpellChecking( bool );

  /**
   * For the external editor
   */
  void setUseExternalEditor( bool use ) { mUseExtEditor = use; }
  void setExternalEditorPath( const QString & path ) { mExtEditor = path; }

  /**
   * Check that the external editor has finished and output a warning
   * if it hasn't.
   * @return false if the user chose to cancel whatever operation
   * called this method.
   */
  bool checkExternalEditorFinished();

  QPopupMenu* createPopupMenu(const QPoint&);
  void setSpellCheckingActive(bool spellCheckingActive);

  /** Drag and drop methods */
  void contentsDragEnterEvent(QDragEnterEvent *e);
  void contentsDragMoveEvent(QDragMoveEvent *e);
  void contentsDropEvent(QDropEvent *e);

  void deleteAutoSpellChecking();

  unsigned int lineBreakColumn() const;
  
  /** set cursor to absolute position pos */
  void setCursorPositionFromStart(unsigned int pos);

signals:
  void spellcheck_done(int result);
  void attachPNGImageData(const QByteArray &image);
  void pasteImage();
  void focusUp();
  void focusChanged( bool );
  void selectionAvailable( bool );
  void insertSnippet();
public slots:
  void initializeAutoSpellChecking();
  void slotSpellcheck2(KSpell*);
  void slotSpellResult(const QString&);
  void slotSpellDone();
  void slotExternalEditorDone(KProcess*);
  void slotMisspelling(const QString &, const QStringList &, unsigned int);
  void slotCorrected (const QString &, const QString &, unsigned int);
  void addSuggestion(const QString& text, const QStringList& lst, unsigned int );
  void cut();
  void clear();
  void del();
  void paste();
protected:
  /**
   * Event filter that does Tab-key handling.
   */
  bool eventFilter(QObject*, QEvent*);
  void keyPressEvent( QKeyEvent* );
  
  void contentsMouseReleaseEvent( QMouseEvent * e );

private slots:
  void slotExternalEditorTempFileChanged( const QString & fileName );
  void slotSelectionChanged() {
    // use !text.isEmpty() here, as null-selections exist, but make no sense
    emit selectionAvailable( !selectedText().isEmpty() );
  }

private:
  void killExternalEditor();

private:
  KMComposeWin* mComposer;

  KSpell *mKSpell;
  KSpellConfig *mSpellConfig;
  QMap<QString,QStringList> mReplacements;
  SpellingFilter* mSpellingFilter;
  KTempFile *mExtEditorTempFile;
  KDirWatch *mExtEditorTempFileWatcher;
  KProcess  *mExtEditorProcess;
  bool      mUseExtEditor;
  QString   mExtEditor;
  bool      mWasModifiedBeforeSpellCheck;
  KDictSpellingHighlighter *mSpellChecker;
  bool mSpellLineEdit;
  QClipboard::Mode mPasteMode;
};

#endif // __KMAIL_KMEDIT_H__

