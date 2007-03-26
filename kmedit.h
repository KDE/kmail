/* -*- mode: C++; c-file-style: "gnu" -*-
 * KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMAIL_KMEDIT_H__
#define __KMAIL_KMEDIT_H__

#include <kdeversion.h>
#include <keditcl.h>
#include <QMap>
#include <QStringList>
//Added by qt3to4:
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QKeyEvent>
#include <QEvent>
#include <Q3PopupMenu>
#include <QDropEvent>

class KMComposeWin;
class K3SpellConfig;
class K3Spell;
class SpellingFilter;
class KTemporaryFile;
class K3DictSpellingHighlighter;
class KDirWatch;
class K3Process;


class KMEdit : public KEdit {
  Q_OBJECT
public:
  KMEdit(QWidget *parent=0,KMComposeWin* composer=0,
         K3SpellConfig* spellConfig = 0,
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

  Q3PopupMenu* createPopupMenu(const QPoint&);
  void setSpellCheckingActive(bool spellCheckingActive);

  /** Drag and drop methods */
  void contentsDragEnterEvent(QDragEnterEvent *e);
  void contentsDragMoveEvent(QDragMoveEvent *e);
  void contentsDropEvent(QDropEvent *e);

  void deleteAutoSpellChecking();

  unsigned int lineBreakColumn() const;

signals:
  void spellcheck_done(int result);
  void pasteImage();
  void focusUp();
  void focusChanged( bool );
public slots:
  void initializeAutoSpellChecking();
  void slotSpellcheck2(K3Spell*);
  void slotSpellResult(const QString&);
  void slotSpellDone();
  void slotExternalEditorDone(K3Process*);
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

private slots:
  void slotExternalEditorTempFileChanged( const QString & fileName );

private:
  void killExternalEditor();

private:
  KMComposeWin* mComposer;

  K3Spell *mKSpell;
  K3SpellConfig *mSpellConfig;
  QMap<QString,QStringList> mReplacements;
  SpellingFilter* mSpellingFilter;
  KTemporaryFile *mExtEditorTempFile;
  KDirWatch *mExtEditorTempFileWatcher;
  K3Process  *mExtEditorProcess;
  bool      mUseExtEditor;
  QString   mExtEditor;
  bool      mWasModifiedBeforeSpellCheck;
  K3DictSpellingHighlighter *mSpellChecker;
  bool mSpellLineEdit;
};

#endif // __KMAIL_KMEDIT_H__

