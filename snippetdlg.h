/****************************************************************************
** Form interface generated from reading ui file '/Users/till/Documents/Code/kde/enterprise/kdepim/kmail/snippetdlg.ui'
**
** Created: Tue Sep 25 16:03:02 2007
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.8   edited Jan 11 14:47 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

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
    SnippetDlg( KActionCollection* ac, QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~SnippetDlg();

    void setShowShortcut( bool show );

    QLabel* textLabel3;
    QLabel* textLabelGroup;
    KKeyButton* keyButton;
    KActionCollection* actionCollection;

private slots:
    void slotCapturedShortcut( const KShortcut& );

protected slots:
    virtual void languageChange();

};

#endif // SNIPPETDLG_H
