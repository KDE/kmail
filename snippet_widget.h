/*
 *  File : snippet_widget.h
 *
 *  Author: Robert Gruber <rgruber@users.sourceforge.net>
 *
 *  Copyright: See COPYING file that comes with this distribution
 */

#ifndef __SNIPPET_WIDGET_H__
#define __SNIPPET_WIDGET_H__

#include <qwidget.h>
#include <qstring.h>
#include <klistview.h>
#include <qtooltip.h>
#include <qrect.h>

#include <ktexteditor/editinterface.h>
#include <ktexteditor/view.h>
#include "snippetconfig.h"

class KDevProject;
class SnippetPart;
class QPushButton;
class KListView;
class QListViewItem;
class QPoint;
class SnippetDlg;
class SnippetItem;
class KTextEdit;
class KConfig;
class KMEdit;

/**
This is the widget which gets added to the right TreeToolView.
It inherits KListView and QToolTip which is needed for showing the
tooltips which contains the text of the snippet
@author Robert Gruber
*/
class SnippetWidget : public KListView, public QToolTip
{
  Q_OBJECT

  friend class SnippetSettings; //to allow SnippetSettings to call languageChanged()
  
public:
    SnippetWidget(KMEdit* editor, QWidget* parent = 0);
    ~SnippetWidget();
    QPtrList<SnippetItem> * getList() { return (&_list); }
    void writeConfig();
    SnippetConfig *  getSnippetConfig() { return (&_SnippetConfig); }


private slots:
    void initConfig();
    void languageChanged();

protected:
    void maybeTip( const QPoint & );
    bool acceptDrag (QDropEvent *event) const;

private:
    void insertIntoActiveView(QString text);
    QString parseText(QString text, QString del="$");
    bool showMultiVarDialog(QMap<QString, QString> * map, QMap<QString, QString> * mapSave,
                            int & iWidth, int & iBasicHeight, int & iOneHeight);
    QString showSingleVarDialog(QString var, QMap<QString, QString> * mapSave, QRect & dlgSize);

    QPtrList<SnippetItem> _list;
    QMap<QString, QString> _mapSaved;
    KConfig * _cfg;
    SnippetConfig _SnippetConfig;
    KMEdit* mEditor;

public slots:
    void slotRemove();
    void slotEdit( QListViewItem* item_ = 0 );
    void slotEditGroup();
    void slotAdd();
    void slotAddGroup();
    void slotExecute();

protected slots:
    void showPopupMenu( QListViewItem * item, const QPoint & p, int );
    void slotExecuted(QListViewItem * item =  0);
    void slotDropped(QDropEvent *e, QListViewItem *after);
    void startDrag();
};


#endif
