/*
 *  File : snippet_widget.h
 *
 *  Author: Robert Gruber <rgruber@users.sourceforge.net>
 *
 *  Copyright: See COPYING file that comes with this distribution
 */

#ifndef __SNIPPET_WIDGET_H__
#define __SNIPPET_WIDGET_H__

#include <qstring.h>
#include <QTreeWidget>
#include <qrect.h>

#include <libkdepim/kmeditor.h>
#include <ktexteditor/view.h>
#include "snippetconfig.h"

class QPushButton;
class QTreeWidgetItem;
class QContextMenuEvent;
class SnippetDlg;
class SnippetItem;
class KTextEdit;
class KConfig;
class QWidget;
class KActionCollection;
using KPIM::KMeditor;

/**
This is the widget which gets added to the right TreeToolView.
It inherits QTreeWidget which is needed for showing the
tooltips which contains the text of the snippet
@author Robert Gruber
*/
class SnippetWidget : public QTreeWidget
{
  Q_OBJECT

public:
    SnippetWidget(KMeditor *editor, KActionCollection *actionCollection, QWidget *parent = 0);
    ~SnippetWidget();
    QList<SnippetItem * > * getList() { return (&_list); }
    void writeConfig();
    SnippetConfig *  getSnippetConfig() { return (&_SnippetConfig); }

private slots:
    void initConfig();

protected:
    bool acceptDrag(QDropEvent *event) const;
    void startDrag( Qt::DropActions supportedActions );
    bool dropMimeData( QTreeWidgetItem *parent, int index,
                       const QMimeData *data, Qt::DropAction action );
    void contextMenuEvent( QContextMenuEvent *e );
private:
    void insertIntoActiveView( const QString &text );
    QString parseText(QString text, QString del="$");
    bool showMultiVarDialog(QMap<QString, QString> * map, QMap<QString, QString> * mapSave,
                            int & iWidth, int & iBasicHeight, int & iOneHeight);
    QString showSingleVarDialog(QString var, QMap<QString, QString> * mapSave, QRect & dlgSize);
    QTreeWidgetItem *selectedItem() const;
    SnippetItem* makeItem( SnippetItem *parent, const QString &name,
                           const QString &text, const QKeySequence &keySeq );

    QList<SnippetItem * > _list;
    QMap<QString, QString> _mapSaved;
    KConfig * _cfg;
    SnippetConfig _SnippetConfig;
    KMeditor *mEditor;
    KActionCollection* mActionCollection;

public slots:
    void slotRemove();
    void slotEdit( QTreeWidgetItem *item_ = 0 );
    void slotEditGroup();
    void slotAdd();
    void slotAddGroup();
    void slotExecute();

protected slots:
    void slotExecuted( QTreeWidgetItem *item =  0 );
};

#endif
