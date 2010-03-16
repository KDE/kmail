/* -*- mode: C++; c-file-style: "gnu" -*-
 * KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMAIL_KMEDIT_H__
#define __KMAIL_KMEDIT_H__

#include <kdeversion.h>
#include <keditcl.h>
#include <kspell.h>
#include <ksyntaxhighlighter.h>
#include <qmap.h>
#include <qstringlist.h>
#include <qclipboard.h>

class KMComposeWin;
class KSpellConfig;
class SpellingFilter;
class KTempFile;
class KDirWatch;
class KProcess;
class QPopupMenu;

/**
 * Reimplemented to make writePersonalDictionary() public, which we call everytime after
 * adding a word to the dictionary (for safety's sake and because the highlighter needs to reload
 * the personal word list, and for that, it needs to be written to disc)
 */
class KMSpell : public KSpell
{
  public:

    KMSpell( QObject *receiver, const char *slot, KSpellConfig *spellConfig );
    using KSpell::writePersonalDictionary;
};

/**
 * Reimplemented to add support for ignored words
 */
class KMSyntaxHighter : public KDictSpellingHighlighter
{
  public:

    KMSyntaxHighter( QTextEdit *textEdit,
                     bool spellCheckingActive = true,
                     bool autoEnable = true,
                     const QColor& spellColor = red,
                     bool colorQuoting = false,
                     const QColor& QuoteColor0 = black,
                     const QColor& QuoteColor1 = QColor( 0x00, 0x80, 0x00 ),
                     const QColor& QuoteColor2 = QColor( 0x00, 0x70, 0x00 ),
                     const QColor& QuoteColor3 = QColor( 0x00, 0x60, 0x00 ),
                     KSpellConfig *spellConfig = 0 );

    /** Reimplemented */
    virtual bool isMisspelled( const QString &word );

    void ignoreWord( const QString &word );

    QStringList ignoredWords() const;

  private:
    QStringList mIgnoredWords;
};

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

  /// Reimplemented to select words under the cursor on double-clicks in our way,
  /// not the broken Qt way (https://issues.kolab.org/issue4089)
  virtual void contentsMouseDoubleClickEvent( QMouseEvent *e );

private slots:
  void slotExternalEditorTempFileChanged( const QString & fileName );
  void slotSelectionChanged() {
    // use !text.isEmpty() here, as null-selections exist, but make no sense
    emit selectionAvailable( !selectedText().isEmpty() );
  }

  /// Called when mSpeller is ready to rumble. Does nothing, but KSpell requires a slot as otherwise
  /// it will show a dialog itself, which we want to avoid.
  void spellerReady( KSpell *speller );

  /// Called when mSpeller died for some reason.
  void spellerDied();

  /// Re-creates the spellers, called when the dictionary is changed
  void createSpellers();

private:
  void killExternalEditor();

private:
  KMComposeWin* mComposer;

  // This is the speller used for the spellcheck dialog. It is only active as long as the spellcheck
  // dialog is shown
  KSpell *mKSpellForDialog;

  // This is the speller used when right-clicking a word and choosing "add to dictionary". It lives
  // as long as the composer lives.
  KMSpell *mSpeller;

  KSpellConfig *mSpellConfig;
  QMap<QString,QStringList> mReplacements;
  SpellingFilter* mSpellingFilter;
  KTempFile *mExtEditorTempFile;
  KDirWatch *mExtEditorTempFileWatcher;
  KProcess  *mExtEditorProcess;
  bool      mUseExtEditor;
  QString   mExtEditor;
  bool      mWasModifiedBeforeSpellCheck;
  KMSyntaxHighter *mHighlighter;
  bool mSpellLineEdit;
  QClipboard::Mode mPasteMode;
};

#endif // __KMAIL_KMEDIT_H__

