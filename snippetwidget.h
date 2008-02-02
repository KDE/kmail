/***************************************************************************
 *   snippet feature from kdevelop/plugins/snippet/                        *
 *                                                                         * 
 *   Copyright (C) 2007 by Robert Gruber                                   *
 *   rgruber@users.sourceforge.net                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
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
class KActionCollection;

/**
This is the widget which gets added to the right TreeToolView.
It inherits KListView and QToolTip which is needed for showing the
tooltips which contains the text of the snippet
@author Robert Gruber
*/
class SnippetWidget : public KListView, public QToolTip
{
  Q_OBJECT

public:
    SnippetWidget(KMEdit* editor, KActionCollection* actionCollection, QWidget* parent = 0);
    ~SnippetWidget();
    QPtrList<SnippetItem> * getList() { return (&_list); }
    void writeConfig();
    SnippetConfig *  getSnippetConfig() { return (&_SnippetConfig); }


private slots:
    void initConfig();

protected:
    void maybeTip( const QPoint & );
    bool acceptDrag (QDropEvent *event) const;

private:
    void insertIntoActiveView( const QString &text );
    QString parseText(QString text, QString del="$");
    bool showMultiVarDialog(QMap<QString, QString> * map, QMap<QString, QString> * mapSave,
                            int & iWidth, int & iBasicHeight, int & iOneHeight);
    QString showSingleVarDialog(QString var, QMap<QString, QString> * mapSave, QRect & dlgSize);
    SnippetItem* makeItem( SnippetItem* parent, const QString& name, const QString& text, const KShortcut& shortcut );

    QPtrList<SnippetItem> _list;
    QMap<QString, QString> _mapSaved;
    KConfig * _cfg;
    SnippetConfig _SnippetConfig;
    KMEdit* mEditor;
    KActionCollection* mActionCollection;

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
