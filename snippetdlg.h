/*
  Originally created by uic, now hand-edited
*/

#ifndef SNIPPETDLG_H
#define SNIPPETDLG_H

#include "ui_snippetdlgbase.h"
using Ui::SnippetDlgBase;

class KActionCollection;
class KShortcut;

class SnippetDlg : public QDialog, public SnippetDlgBase
{
    Q_OBJECT

public:
    explicit SnippetDlg( KActionCollection *ac, QWidget *parent = 0,
                         bool modal = false, Qt::WindowFlags f = 0 );
    ~SnippetDlg();

    void setGroupMode( bool groupMode );

    KActionCollection* actionCollection;
};

#endif // SNIPPETDLG_H
