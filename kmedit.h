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

#ifndef __KMAIL_KMEDIT_H__
#define __KMAIL_KMEDIT_H__

#include <kdeversion.h>
#include <keditcl.h>
#include <QMap>
#include <QStringList>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QKeyEvent>
#include <QEvent>
#include <Q3PopupMenu>
#include <QDropEvent>

class KMComposeWin;
class K3SpellConfig;
class K3Spell;
namespace KPIMUtils { class SpellingFilter; }
using KPIMUtils::SpellingFilter;
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

  /** set cursor to absolute position pos */
  void setCursorPositionFromStart(unsigned int pos);

signals:
  void spellcheck_done( int result );
  void attachPNGImageData( const QByteArray &image );
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

