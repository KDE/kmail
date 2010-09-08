/*
  Originally created by uic, now hand-edited
*/

#ifndef SNIPPETDLG_H
#define SNIPPETDLG_H

#include "snippetdlgbase.h"

class KKeyButton;
class KActionCollection;
class KShortcut;

class SnippetDlg : public SnippetDlgBase
{
  Q_OBJECT
  public:
    SnippetDlg( KActionCollection *ac, QWidget *parent = 0, const char *name = 0,
                bool modal = FALSE, WFlags fl = 0 );
    ~SnippetDlg();

    void setGroupMode( bool groupMode );
    void setShowShortcut( bool show );

    QLabel *shortcutLabel;
    KKeyButton *shortcutButton;
    KActionCollection *actionCollection;

  protected slots:
    void slotTextChanged( const QString & );
    void slotReturnPressed();
    virtual void languageChange();

  private slots:
    void slotCapturedShortcut( const KShortcut & );
};

#endif // SNIPPETDLG_H
